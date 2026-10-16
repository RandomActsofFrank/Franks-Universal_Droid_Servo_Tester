#pragma once

#include <Arduino.h>

// MP-port and virtual-button LED patterns. Values 0–3 are legacy (NVS-stable).
enum class UdcLedEffect : uint8_t {
    Solid = 0,
    RgbCycle = 1,
    RainbowChase = 2,
    White = 3,
    Candle = 4,
    Chase = 5,
    DjLight = 6,
    Fade = 7,
    FlowStripe = 8,
    Gradient = 9,
    Pacifica = 10,
    Rainbow = 11,
    Twinkle = 12,
    Fire = 13,
    Breathe = 14,
    Theater = 15,
    Comet = 16,
    Strobe = 17,
    ColorWipe = 18,
    Scanner = 19,
    Police = 20,
};

inline uint8_t udcLedEffectCount() {
    return static_cast<uint8_t>(UdcLedEffect::Police) + 1U;
}

using VirtualButtonLedPattern = UdcLedEffect;

struct UdcLedFxState {
    uint32_t lastFrameMs = 0;
    uint16_t huePhase = 0;
    uint8_t rgbStepIndex = 0;
    uint32_t lastRgbChangeMs = 0;
    uint8_t chaseHead = 0;
    uint8_t fadeLevel = 0;
    int8_t fadeDir = 1;
    uint32_t djLastFlashMs = 0;
    uint8_t djR = 255;
    uint8_t djG = 0;
    uint8_t djB = 255;
    uint32_t candleSeed = 1;
    uint8_t wipePos = 0;
    uint8_t scannerPos = 0;
    int8_t scannerDir = 1;
};

UdcLedEffect udcLedEffectFromIndex(uint8_t index);
uint8_t udcLedEffectToIndex(UdcLedEffect effect);

struct UdcLedFxRenderArgs {
    UdcLedEffect effect = UdcLedEffect::Solid;
    UdcLedFxState *state = nullptr;
    uint16_t pixelCount = 1;
    uint32_t nowMs = 0;
    uint8_t solidR = 255;
    uint8_t solidG = 0;
    uint8_t solidB = 0;
    uint8_t solidR2 = 0;
    uint8_t solidG2 = 0;
    uint8_t solidB2 = 255;
    uint8_t speed255 = 128;
    uint8_t intensity255 = 128;
    void (*setPixel)(uint16_t index, uint8_t r, uint8_t g, uint8_t b, void *ctx) = nullptr;
    void *setPixelCtx = nullptr;
    void (*show)(void *ctx) = nullptr;
    void *showCtx = nullptr;
};

const char *udcLedEffectLabel(UdcLedEffect effect);
const char *udcLedEffectParamHint(UdcLedEffect effect);
const char *udcLedEffectUiLabel(UdcLedEffect effect);
const char *udcLedEffectModeString(UdcLedEffect effect);
UdcLedEffect udcLedEffectFromModeString(const char *mode);
UdcLedEffect clampUdcLedEffect(uint8_t value);

uint8_t udcLedParamPctTo255(uint8_t pct);
uint8_t udcLedClampParamPct(uint8_t pct);
uint8_t udcLedClampColorPct(uint8_t pct);
uint8_t udcLedColorPctTo255(uint8_t pct);
uint8_t udcLedNormalizeColorPct(uint8_t value);
uint32_t udcLedFrameIntervalMs(uint8_t speedPct);

bool udcLedEffectIsPublic(UdcLedEffect effect);
uint8_t udcLedPublicEffectCount();
UdcLedEffect udcLedPublicEffectAt(uint8_t publicIndex);
uint8_t udcLedPublicIndexOf(UdcLedEffect effect);
UdcLedEffect udcLedEffectSanitize(UdcLedEffect effect);
bool udcLedEffectUsesColorA(UdcLedEffect effect);
bool udcLedEffectUsesColorB(UdcLedEffect effect);

void udcLedFxReset(UdcLedFxState &state);
void udcLedFxRenderFrame(UdcLedFxRenderArgs &args);
