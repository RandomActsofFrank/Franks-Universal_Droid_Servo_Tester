/*
 * UDC Servo & LED Tester
 * Six-servo PWM tester (500–2500 μs) + WS2811/NeoPixel strip with UDC LedFx effects.
 *
 * Libraries: ESP32Servo, Adafruit_NeoPixel, ArduinoJson, WiFi, WebServer
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>
#include <strings.h>

#include "secrets.h"
#include "UdcLedFx.h"
#include "WebUi.h"
#include "BannerPng.h"
#include "FaviconPng.h"

// ISO-week version: YYYY.MM-WW.NN (WW = ISO week number)
#ifndef UDC_SERVO_TESTER_VERSION
#define UDC_SERVO_TESTER_VERSION "2026.10-42.07"
#endif
static const char *kSketchVersion = UDC_SERVO_TESTER_VERSION;

static const char *kHostname = "UDC-ServoTester";

// ---- Board profile (flip these for 30-pin vs 38-pin) ----
// Set exactly one of these to 1. Default: 30-pin (Frank's handy board).
#define UDC_BOARD_38PIN 0
#define UDC_BOARD_30PIN 1

// Servo PWM refresh (ESP32Servo setPeriodHertz). 300 Hz feels snappy on digital servos;
// drop to 50 if an analog/hobby servo only tracks a classic 20 ms frame.
#ifndef UDC_SERVO_HZ
#define UDC_SERVO_HZ 300
#endif
static const int kServoHz = UDC_SERVO_HZ;

#if (UDC_BOARD_38PIN + UDC_BOARD_30PIN) != 1
#error "Set exactly one of UDC_BOARD_38PIN or UDC_BOARD_30PIN to 1"
#endif

#if UDC_BOARD_38PIN
// 38-pin DevKit-style
static const char *kBoardProfileName = "38-pin";
static const uint8_t kServoPins[6] = {13, 14, 25, 26, 27, 33};
static const uint8_t kLedPin = 18;
#elif UDC_BOARD_30PIN
// 30-pin DevKit-style (default) — safe broken-out set; LED on 23 (no overlap with servos)
static const char *kBoardProfileName = "30-pin";
static const uint8_t kServoPins[6] = {13, 14, 16, 17, 25, 26};
static const uint8_t kLedPin = 23;
#endif

static const uint16_t kDefaultLedCount = 60;
static const uint16_t kMaxLedCount = 300;

static const int kPulseMinUs = 500;
static const int kPulseMaxUs = 2500;
static const uint8_t kResetPercent = 50;

enum class LedColorOrder : uint8_t {
    Rgb = 0,
    Grb = 1,
    Rbg = 2,
    Brg = 3,
    Gbr = 4,
    Bgr = 5,
};

struct LedConfig {
    uint16_t count = kDefaultLedCount;
    uint8_t brightnessPct = 80;
    UdcLedEffect effect = UdcLedEffect::Solid;
    LedColorOrder order = LedColorOrder::Grb;
    uint8_t r = 255;
    uint8_t g = 0;
    uint8_t b = 0;
    uint8_t r2 = 0;
    uint8_t g2 = 0;
    uint8_t b2 = 255;
    uint8_t speedPct = 50;
    uint8_t intensityPct = 50;
    bool invert = false;
};

struct ServoChannel {
    uint8_t percent = 50;
    uint8_t minPercent = 0;
    uint8_t maxPercent = 100;
    bool sweeping = false;
    uint8_t sweepTarget = 100;
    uint8_t sweepStartPercent = 50;
    uint32_t sweepStartMs = 0;
    uint32_t sweepDurationMs = 0;
};

static Servo gServos[6];
static ServoChannel gServoCh[6];

static Adafruit_NeoPixel *gStrip = nullptr;
static LedConfig gLed;
static UdcLedFxState gFxState;
static uint32_t gLastFxMs = 0;
static uint8_t gFxBrightnessPct = 80;
// NeoPixel show() blocks interrupts — cap FPS and pause FX while servos are busy.
static constexpr uint32_t kLedMinFrameMs = 28;     // ~35 FPS hard cap
static constexpr uint32_t kLedBusyFrameMs = 50;    // slower while recent servo activity
static constexpr uint32_t kServoBusyHoldMs = 80;   // treat as "busy" after a write
static constexpr uint32_t kSweepMsPerPercent = 18; // matches prior ~18ms/+1% pace
static uint32_t gLastServoCmdMs = 0;

static WebServer gServer(80);

// ---- Helpers ----
static int percentToPulseUs(uint8_t percent) {
    if (percent > 100) {
        percent = 100;
    }
    return kPulseMinUs + (static_cast<int>(percent) * (kPulseMaxUs - kPulseMinUs)) / 100;
}

static uint8_t clampU8(int v, int lo, int hi) {
    if (v < lo) {
        return static_cast<uint8_t>(lo);
    }
    if (v > hi) {
        return static_cast<uint8_t>(hi);
    }
    return static_cast<uint8_t>(v);
}

static uint8_t stripScale(uint8_t c, uint8_t brightnessPct) {
    // Same math as MpLedEngine / Adafruit_NeoPixel::setBrightness applied per channel.
    const uint8_t scaled = static_cast<uint8_t>((static_cast<uint16_t>(brightnessPct) * 255U) / 100U);
    if (scaled == 255U) {
        return c;
    }
    return static_cast<uint8_t>((static_cast<uint16_t>(c) * (static_cast<uint16_t>(scaled) + 1U)) >> 8);
}

static uint32_t neoPixelFlags(LedColorOrder order) {
    switch (order) {
        case LedColorOrder::Rgb:
            return NEO_RGB + NEO_KHZ800;
        case LedColorOrder::Rbg:
            return NEO_RBG + NEO_KHZ800;
        case LedColorOrder::Brg:
            return NEO_BRG + NEO_KHZ800;
        case LedColorOrder::Gbr:
            return NEO_GBR + NEO_KHZ800;
        case LedColorOrder::Bgr:
            return NEO_BGR + NEO_KHZ800;
        case LedColorOrder::Grb:
        default:
            return NEO_GRB + NEO_KHZ800;
    }
}

static const char *colorOrderLabel(LedColorOrder order) {
    switch (order) {
        case LedColorOrder::Rgb:
            return "RGB";
        case LedColorOrder::Rbg:
            return "RBG";
        case LedColorOrder::Brg:
            return "BRG";
        case LedColorOrder::Gbr:
            return "GBR";
        case LedColorOrder::Bgr:
            return "BGR";
        case LedColorOrder::Grb:
        default:
            return "GRB";
    }
}

static LedColorOrder colorOrderFromString(const char *s) {
    if (s == nullptr) {
        return LedColorOrder::Grb;
    }
    if (strcasecmp(s, "RGB") == 0) {
        return LedColorOrder::Rgb;
    }
    if (strcasecmp(s, "RBG") == 0) {
        return LedColorOrder::Rbg;
    }
    if (strcasecmp(s, "BRG") == 0) {
        return LedColorOrder::Brg;
    }
    if (strcasecmp(s, "GBR") == 0) {
        return LedColorOrder::Gbr;
    }
    if (strcasecmp(s, "BGR") == 0) {
        return LedColorOrder::Bgr;
    }
    return LedColorOrder::Grb;
}

static void stopSweep(uint8_t index0) {
    if (index0 < 6) {
        gServoCh[index0].sweeping = false;
        gServoCh[index0].sweepDurationMs = 0;
    }
}

static bool anyServoSweeping() {
    for (uint8_t i = 0; i < 6; ++i) {
        if (gServoCh[i].sweeping) {
            return true;
        }
    }
    return false;
}

static bool servosRecentlyBusy(uint32_t nowMs) {
    if (anyServoSweeping()) {
        return true;
    }
    return (gLastServoCmdMs != 0) && ((nowMs - gLastServoCmdMs) < kServoBusyHoldMs);
}

static void writeServoPulse(uint8_t index0, uint8_t percent) {
    percent = clampU8(percent, 0, 100);
    gServoCh[index0].percent = percent;
    gServos[index0].writeMicroseconds(percentToPulseUs(percent));
    gLastServoCmdMs = millis();
}

static void setServoPercent(uint8_t index0, uint8_t percent) {
    if (index0 >= 6) {
        return;
    }
    stopSweep(index0);
    writeServoPulse(index0, percent);
}

static void resetServo(uint8_t index0) {
    setServoPercent(index0, kResetPercent);
}

static void normalizeSweepLimits(uint8_t index0) {
    ServoChannel &ch = gServoCh[index0];
    ch.minPercent = clampU8(ch.minPercent, 0, 100);
    ch.maxPercent = clampU8(ch.maxPercent, 0, 100);
    if (ch.minPercent > ch.maxPercent) {
        const uint8_t tmp = ch.minPercent;
        ch.minPercent = ch.maxPercent;
        ch.maxPercent = tmp;
    }
}

static void setSweepMinFromCurrent(uint8_t index0) {
    stopSweep(index0);
    ServoChannel &ch = gServoCh[index0];
    ch.minPercent = ch.percent;
    normalizeSweepLimits(index0);
}

static void setSweepMaxFromCurrent(uint8_t index0) {
    stopSweep(index0);
    ServoChannel &ch = gServoCh[index0];
    ch.maxPercent = ch.percent;
    normalizeSweepLimits(index0);
}

// One-way trip: direction from current % — ≥50 → min, <50 → max; animate and stay.
// Press while already sweeping cancels in place.
static bool startOneWaySweep(uint8_t index0) {
    ServoChannel &ch = gServoCh[index0];
    normalizeSweepLimits(index0);
    if (ch.minPercent >= ch.maxPercent) {
        return false;
    }

    if (ch.sweeping) {
        ch.sweeping = false;
        return true;
    }

    // Exact 50% counts as ≥50 → go to min.
    const bool goToMin = (ch.percent >= 50);
    ch.sweepTarget = goToMin ? ch.minPercent : ch.maxPercent;

    if (ch.percent == ch.sweepTarget) {
        ch.sweeping = false;
        return true;
    }

    // Time-based: elapsed ms → percent so late loops don't bunch or stall at +1/tick.
    const uint32_t now = millis();
    const uint32_t dist = static_cast<uint32_t>(
        ch.percent > ch.sweepTarget ? (ch.percent - ch.sweepTarget)
                                    : (ch.sweepTarget - ch.percent));
    ch.sweepStartPercent = ch.percent;
    ch.sweepStartMs = now;
    ch.sweepDurationMs = dist * kSweepMsPerPercent;
    if (ch.sweepDurationMs == 0) {
        ch.sweepDurationMs = kSweepMsPerPercent;
    }
    ch.sweeping = true;
    gLastServoCmdMs = now;
    return true;
}

static void updateServoSweeps() {
    const uint32_t now = millis();
    for (uint8_t i = 0; i < 6; ++i) {
        ServoChannel &ch = gServoCh[i];
        if (!ch.sweeping) {
            continue;
        }

        const uint32_t elapsed = now - ch.sweepStartMs;
        if (elapsed >= ch.sweepDurationMs) {
            writeServoPulse(i, ch.sweepTarget);
            ch.sweeping = false;
            ch.sweepDurationMs = 0;
            continue;
        }

        const int start = static_cast<int>(ch.sweepStartPercent);
        const int target = static_cast<int>(ch.sweepTarget);
        const int next = start + static_cast<int>(
            (static_cast<int32_t>(target - start) * static_cast<int32_t>(elapsed)) /
            static_cast<int32_t>(ch.sweepDurationMs));
        const uint8_t pct = static_cast<uint8_t>(clampU8(next, 0, 100));
        if (pct != ch.percent) {
            writeServoPulse(i, pct);
        }
    }
}

static void fxSetPixel(uint16_t index, uint8_t r, uint8_t g, uint8_t b, void *ctx) {
    (void)ctx;
    if (gStrip == nullptr || index >= gStrip->numPixels()) {
        return;
    }
    // Mirror strip index when Invert Direction is on (UdcLedFx has no native reverse).
    uint16_t px = index;
    if (gLed.invert) {
        px = static_cast<uint16_t>(gStrip->numPixels() - 1U - index);
    }
    gStrip->setPixelColor(px,
                          Adafruit_NeoPixel::Color(stripScale(r, gFxBrightnessPct),
                                                   stripScale(g, gFxBrightnessPct),
                                                   stripScale(b, gFxBrightnessPct)));
}

static void fxShow(void *ctx) {
    (void)ctx;
    if (gStrip != nullptr) {
        gStrip->show();
    }
}

static void ledBootPulse() {
    if (gStrip == nullptr) {
        return;
    }
    // Brief solid prove-out so a mis-wired DIN is obvious at Serial + eyes.
    const uint16_t n = gStrip->numPixels();
    for (uint16_t i = 0; i < n; ++i) {
        gStrip->setPixelColor(i, Adafruit_NeoPixel::Color(64, 0, 0));
    }
    gStrip->show();
    delay(120);
    gStrip->clear();
    gStrip->show();
}

static void rebuildStrip() {
    uint16_t count = gLed.count;
    if (count < 1) {
        count = 1;
    }
    if (count > kMaxLedCount) {
        count = kMaxLedCount;
    }
    gLed.count = count;

    if (gStrip != nullptr) {
        gStrip->clear();
        gStrip->show();
        delete gStrip;
        gStrip = nullptr;
    }

    // Match UDC MpLedEngine / MultiPurposePort: Adafruit_NeoPixel + NEO_* + NEO_KHZ800.
    // Construct BEFORE ESP32Servo attach so RMT claims the LED pin first (avoids servo/LED
    // peripheral fights seen on some ESP32 cores when NeoPixel is created after many LEDC channels).
    pinMode(kLedPin, OUTPUT);
    digitalWrite(kLedPin, LOW);

    gStrip = new Adafruit_NeoPixel(count, kLedPin, neoPixelFlags(gLed.order));
    if (gStrip == nullptr) {
        Serial.println("[led] NeoPixel alloc failed");
        return;
    }
    gStrip->begin();
    gStrip->clear();
    gStrip->show();
    udcLedFxReset(gFxState);
    gLastFxMs = 0;
    Serial.printf("[led] strip begin pin=%u count=%u flags order=%s\n",
                  kLedPin,
                  gStrip->numPixels(),
                  colorOrderLabel(gLed.order));
}

static void applyLedConfig(const LedConfig &next, bool rebuild) {
    gLed = next;
    gLed.effect = udcLedEffectSanitize(gLed.effect);
    gLed.brightnessPct = clampU8(gLed.brightnessPct, 0, 100);
    gLed.speedPct = udcLedClampParamPct(gLed.speedPct);
    gLed.intensityPct = udcLedClampParamPct(gLed.intensityPct);
    gFxBrightnessPct = gLed.brightnessPct;
    if (rebuild || gStrip == nullptr) {
        rebuildStrip();
    }
}

static void renderLedFrame() {
    if (gStrip == nullptr) {
        return;
    }

    const uint32_t now = millis();

    // Adafruit_NeoPixel::show() blocks interrupts for the whole strip — skip FX frames
    // while any servo is sweeping so ESP32Servo timing stays smooth.
    if (anyServoSweeping()) {
        return;
    }

    uint32_t frameInterval = udcLedFrameIntervalMs(gLed.speedPct);
    if (frameInterval < kLedMinFrameMs) {
        frameInterval = kLedMinFrameMs; // ~35 FPS cap
    }
    if (servosRecentlyBusy(now) && frameInterval < kLedBusyFrameMs) {
        frameInterval = kLedBusyFrameMs; // slower after slider / reset / limits
    }
    if (gLastFxMs != 0 && (now - gLastFxMs) < frameInterval) {
        return;
    }
    gLastFxMs = now;

    if (gLed.effect == UdcLedEffect::RgbCycle) {
        const uint32_t rgbInterval = 600U + ((100U - gLed.speedPct) * 18U);
        if (now - gFxState.lastRgbChangeMs >= rgbInterval) {
            gFxState.lastRgbChangeMs = now;
            gFxState.rgbStepIndex = static_cast<uint8_t>((gFxState.rgbStepIndex + 1U) % 3U);
        }
    }

    UdcLedFxRenderArgs args;
    args.effect = gLed.effect;
    args.state = &gFxState;
    args.pixelCount = gStrip->numPixels();
    args.nowMs = now;
    args.solidR = gLed.r;
    args.solidG = gLed.g;
    args.solidB = gLed.b;
    args.solidR2 = gLed.r2;
    args.solidG2 = gLed.g2;
    args.solidB2 = gLed.b2;
    args.speed255 = udcLedParamPctTo255(gLed.speedPct);
    args.intensity255 = udcLedParamPctTo255(gLed.intensityPct);
    args.setPixel = fxSetPixel;
    args.setPixelCtx = nullptr;
    args.show = fxShow;
    args.showCtx = nullptr;
    udcLedFxRenderFrame(args);
}

// ---- HTTP ----
static void sendJson(int code, const String &body) {
    gServer.sendHeader("Cache-Control", "no-store");
    gServer.send(code, "application/json", body);
}

static void handleRoot() {
    gServer.sendHeader("Cache-Control", "public, max-age=3600");
    gServer.send_P(200, "text/html", kWebUiHtml);
}

static void sendPngAsset(const uint8_t *data, size_t len) {
    // Binary PROGMEM send (do not use strlen-based send_P — PNGs contain NULs).
    gServer.sendHeader("Cache-Control", "public, max-age=604800");
    gServer.setContentLength(len);
    gServer.send(200, "image/png", "");
    WiFiClient client = gServer.client();
    constexpr size_t kChunk = 512;
    uint8_t buf[kChunk];
    size_t sent = 0;
    while (sent < len && client.connected()) {
        size_t n = len - sent;
        if (n > kChunk) {
            n = kChunk;
        }
        memcpy_P(buf, data + sent, n);
        if (client.write(buf, n) != n) {
            break;
        }
        sent += n;
    }
}

static void handleBanner() {
    sendPngAsset(kBannerPng, kBannerPngLen);
}

static void handleFavicon() {
    sendPngAsset(kFaviconPng, kFaviconPngLen);
}

static void handleState() {
    StaticJsonDocument<1536> doc;
    doc["board"] = kBoardProfileName;
    doc["version"] = kSketchVersion;
    JsonArray servos = doc.createNestedArray("servos");
    for (uint8_t i = 0; i < 6; ++i) {
        const ServoChannel &ch = gServoCh[i];
        JsonObject s = servos.createNestedObject();
        s["id"] = i + 1;
        s["gpio"] = kServoPins[i];
        s["percent"] = ch.percent;
        s["us"] = percentToPulseUs(ch.percent);
        s["minPercent"] = ch.minPercent;
        s["maxPercent"] = ch.maxPercent;
        s["sweeping"] = ch.sweeping;
    }
    JsonObject led = doc.createNestedObject("led");
    led["count"] = gLed.count;
    led["brightness"] = gLed.brightnessPct;
    led["speed"] = gLed.speedPct;
    led["invert"] = gLed.invert;
    led["effect"] = udcLedEffectModeString(gLed.effect);
    led["colorOrder"] = colorOrderLabel(gLed.order);
    led["gpio"] = kLedPin;
    JsonObject c1 = led.createNestedObject("color1");
    c1["r"] = gLed.r;
    c1["g"] = gLed.g;
    c1["b"] = gLed.b;
    JsonObject c2 = led.createNestedObject("color2");
    c2["r"] = gLed.r2;
    c2["g"] = gLed.g2;
    c2["b"] = gLed.b2;

    String out;
    serializeJson(doc, out);
    sendJson(200, out);
}

static bool readJsonBody(StaticJsonDocument<768> &doc) {
    if (!gServer.hasArg("plain")) {
        sendJson(400, "{\"ok\":false,\"error\":\"missing body\"}");
        return false;
    }
    const DeserializationError err = deserializeJson(doc, gServer.arg("plain"));
    if (err) {
        sendJson(400, "{\"ok\":false,\"error\":\"bad json\"}");
        return false;
    }
    return true;
}

static void handleServoPost() {
    StaticJsonDocument<768> doc;
    if (!readJsonBody(doc)) {
        return;
    }
    const int id = doc["id"] | 0;
    const int percent = doc["percent"] | -1;
    if (id < 1 || id > 6 || percent < 0 || percent > 100) {
        sendJson(400, "{\"ok\":false,\"error\":\"id 1-6, percent 0-100\"}");
        return;
    }
    setServoPercent(static_cast<uint8_t>(id - 1), static_cast<uint8_t>(percent));
    sendJson(200, "{\"ok\":true}");
}

static void handleServoReset() {
    StaticJsonDocument<768> doc;
    if (!readJsonBody(doc)) {
        return;
    }
    const int id = doc["id"] | 0;
    if (id < 1 || id > 6) {
        sendJson(400, "{\"ok\":false,\"error\":\"id 1-6\"}");
        return;
    }
    resetServo(static_cast<uint8_t>(id - 1));
    sendJson(200, "{\"ok\":true}");
}

static void handleServoLimits() {
    StaticJsonDocument<768> doc;
    if (!readJsonBody(doc)) {
        return;
    }
    const int id = doc["id"] | 0;
    if (id < 1 || id > 6) {
        sendJson(400, "{\"ok\":false,\"error\":\"id 1-6\"}");
        return;
    }
    const uint8_t idx = static_cast<uint8_t>(id - 1);
    ServoChannel &ch = gServoCh[idx];

    if (doc.containsKey("setMin") && doc["setMin"].as<bool>()) {
        setSweepMinFromCurrent(idx);
    }
    if (doc.containsKey("setMax") && doc["setMax"].as<bool>()) {
        setSweepMaxFromCurrent(idx);
    }
    if (doc.containsKey("minPercent") || doc.containsKey("maxPercent")) {
        stopSweep(idx);
    }
    if (doc.containsKey("minPercent")) {
        ch.minPercent = clampU8(doc["minPercent"] | 0, 0, 100);
    }
    if (doc.containsKey("maxPercent")) {
        ch.maxPercent = clampU8(doc["maxPercent"] | 100, 0, 100);
    }
    normalizeSweepLimits(idx);
    if (ch.sweeping && ch.minPercent >= ch.maxPercent) {
        ch.sweeping = false;
    }

    StaticJsonDocument<256> out;
    out["ok"] = true;
    out["id"] = id;
    out["minPercent"] = ch.minPercent;
    out["maxPercent"] = ch.maxPercent;
    out["percent"] = ch.percent;
    out["sweeping"] = ch.sweeping;
    String body;
    serializeJson(out, body);
    sendJson(200, body);
}

static void handleServoSweep() {
    StaticJsonDocument<768> doc;
    if (!readJsonBody(doc)) {
        return;
    }
    const int id = doc["id"] | 0;
    if (id < 1 || id > 6) {
        sendJson(400, "{\"ok\":false,\"error\":\"id 1-6\"}");
        return;
    }
    const uint8_t idx = static_cast<uint8_t>(id - 1);
    ServoChannel &ch = gServoCh[idx];

    // Explicit cancel
    if (doc.containsKey("enabled") && !doc["enabled"].as<bool>()) {
        stopSweep(idx);
    } else if (!startOneWaySweep(idx)) {
        sendJson(400, "{\"ok\":false,\"error\":\"min and max must differ\"}");
        return;
    }

    StaticJsonDocument<256> out;
    out["ok"] = true;
    out["id"] = id;
    out["sweeping"] = ch.sweeping;
    out["targetPercent"] = ch.sweepTarget;
    out["minPercent"] = ch.minPercent;
    out["maxPercent"] = ch.maxPercent;
    out["percent"] = ch.percent;
    String body;
    serializeJson(out, body);
    sendJson(200, body);
}

static void handleLedPost() {
    StaticJsonDocument<768> doc;
    if (!readJsonBody(doc)) {
        return;
    }

    LedConfig next = gLed;
    if (doc.containsKey("count")) {
        int c = doc["count"] | 60;
        if (c < 1) {
            c = 1;
        }
        if (c > static_cast<int>(kMaxLedCount)) {
            c = kMaxLedCount;
        }
        next.count = static_cast<uint16_t>(c);
    }
    if (doc.containsKey("brightness")) {
        next.brightnessPct = clampU8(doc["brightness"] | 80, 0, 100);
    }
    if (doc.containsKey("effect")) {
        next.effect = udcLedEffectFromModeString(doc["effect"] | "solid");
    }
    if (doc.containsKey("colorOrder")) {
        next.order = colorOrderFromString(doc["colorOrder"] | "GRB");
    }
    if (doc["color1"].is<JsonObject>()) {
        JsonObject c1 = doc["color1"];
        next.r = clampU8(c1["r"] | next.r, 0, 255);
        next.g = clampU8(c1["g"] | next.g, 0, 255);
        next.b = clampU8(c1["b"] | next.b, 0, 255);
    }
    if (doc["color2"].is<JsonObject>()) {
        JsonObject c2 = doc["color2"];
        next.r2 = clampU8(c2["r"] | next.r2, 0, 255);
        next.g2 = clampU8(c2["g"] | next.g2, 0, 255);
        next.b2 = clampU8(c2["b"] | next.b2, 0, 255);
    }
    if (doc.containsKey("speed")) {
        next.speedPct = udcLedClampParamPct(doc["speed"] | next.speedPct);
    }
    if (doc.containsKey("invert")) {
        next.invert = doc["invert"].as<bool>();
    } else if (doc.containsKey("reverse")) {
        next.invert = doc["reverse"].as<bool>();
    }
    if (doc.containsKey("intensity")) {
        next.intensityPct = udcLedClampParamPct(doc["intensity"] | next.intensityPct);
    }

    const bool rebuild = (next.count != gLed.count) || (next.order != gLed.order);
    const bool effectChanged = (next.effect != gLed.effect);
    applyLedConfig(next, rebuild);
    if (effectChanged && !rebuild) {
        udcLedFxReset(gFxState);
        gLastFxMs = 0;
    }
    Serial.printf("[led] api pin=%u count=%u bright=%u%% speed=%u%% invert=%u effect=%s order=%s\n",
                  kLedPin,
                  gLed.count,
                  gLed.brightnessPct,
                  gLed.speedPct,
                  gLed.invert ? 1U : 0U,
                  udcLedEffectModeString(gLed.effect),
                  colorOrderLabel(gLed.order));
    sendJson(200, "{\"ok\":true}");
}

static void handleNotFound() {
    sendJson(404, "{\"ok\":false,\"error\":\"not found\"}");
}

static void printPinSummary() {
    Serial.printf("Board profile: %s\n", kBoardProfileName);
    Serial.printf("  Servos 1-6 GPIO: %u, %u, %u, %u, %u, %u\n",
                  kServoPins[0],
                  kServoPins[1],
                  kServoPins[2],
                  kServoPins[3],
                  kServoPins[4],
                  kServoPins[5]);
    Serial.printf("  NeoPixel data GPIO: %u\n", kLedPin);
}

static void setupWifi() {
    Serial.printf("Connecting to WiFi \"%s\" …\n", WIFI_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.setHostname(kHostname);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    const uint32_t start = millis();
    uint8_t dots = 0;
    while (WiFi.status() != WL_CONNECTED && (millis() - start) < 30000U) {
        delay(500);
        Serial.print('.');
        if (++dots % 40 == 0) {
            Serial.println();
        }
    }
    if (dots > 0) {
        Serial.println();
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("Connected to WiFi");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
        Serial.printf("Hostname: %s\n", kHostname);
    } else {
        Serial.println("WiFi connect failed (timeout ~30s).");
        Serial.println("Check SSID/password in secrets.h, then reset the board to retry.");
        Serial.println("HTTP UI will not be available until WiFi is up.");
    }
}

void setup() {
    Serial.begin(115200);
    delay(200);
    Serial.println();
    Serial.println("=== UDC Servo & LED Tester ===");
    Serial.printf("Version %s\n", kSketchVersion);
    printPinSummary();
    Serial.printf("Wire NeoPixel DIN to GPIO %u (not Chopper's classic 18 unless you select 38-pin).\n",
                  kLedPin);

    // LEDs first (UDC-style NeoPixel + RMT), then servos (LEDC) — keeps both working together.
    udcLedFxReset(gFxState);
    applyLedConfig(gLed, true);
    ledBootPulse();
    Serial.printf("NeoPixel ready: pin=%u count=%u effect=%s order=%s brightness=%u%%\n",
                  kLedPin,
                  gLed.count,
                  udcLedEffectModeString(gLed.effect),
                  colorOrderLabel(gLed.order),
                  gLed.brightnessPct);

    ESP32PWM::allocateTimer(0);
    ESP32PWM::allocateTimer(1);
    ESP32PWM::allocateTimer(2);
    ESP32PWM::allocateTimer(3);

    for (uint8_t i = 0; i < 6; ++i) {
        // ESP32Servo: setPeriodHertz before attach so the timer period sticks.
        gServos[i].setPeriodHertz(kServoHz);
        gServos[i].attach(kServoPins[i], kPulseMinUs, kPulseMaxUs);
        resetServo(i);
    }
    Serial.printf("Servos: %d Hz refresh, attached at 50%% / 1500 us (reset/neutral).\n", kServoHz);
    Serial.printf("LED FX: capped ~%lu FPS (min frame %lu ms); paused while servos sweep.\n",
                  1000UL / static_cast<unsigned long>(kLedMinFrameMs),
                  static_cast<unsigned long>(kLedMinFrameMs));

    // Re-show after servo attach so a quiet strip isn't left dark if RMT needed a kick.
    if (gStrip != nullptr) {
        gStrip->show();
    }

    setupWifi();

    gServer.on("/", HTTP_GET, handleRoot);
    gServer.on("/banner.png", HTTP_GET, handleBanner);
    gServer.on("/favicon.png", HTTP_GET, handleFavicon);
    gServer.on("/favicon.ico", HTTP_GET, handleFavicon);
    gServer.on("/api/state", HTTP_GET, handleState);
    gServer.on("/api/servo", HTTP_POST, handleServoPost);
    gServer.on("/api/servo/reset", HTTP_POST, handleServoReset);
    gServer.on("/api/servo/limits", HTTP_POST, handleServoLimits);
    gServer.on("/api/servo/sweep", HTTP_POST, handleServoSweep);
    gServer.on("/api/led", HTTP_POST, handleLedPost);
    gServer.onNotFound(handleNotFound);
    gServer.begin();
    Serial.println("HTTP server listening on port 80");
}

void loop() {
    // Servo motion first so NeoPixel show() / HTTP work never starve sweep updates.
    updateServoSweeps();
    gServer.handleClient();
    updateServoSweeps(); // catch up if a long HTTP body delayed the first pass
    renderLedFrame();
}
