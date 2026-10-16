# UDC Servo & LED Tester

**Version:** `2026.10-42.07`

ESP32 Servo & LED Tester — companion sketch for Franks-Universal_Droid_Control (not public yet due to ongoing development.)

Arduino IDE sketch for ESP32 DevKit boards (30-pin or 38-pin): six-servo PWM tester plus a WS2811/NeoPixel strip tester using the same public LED effects as Universal Droid Control (`UdcLedFx`).

WiFi hostname is **`UDC-ServoTester`** (set before `WiFi.begin`). Serial Monitor: **115200** baud.

Web UI serves `/banner.png` and `/favicon.png` from embedded copies of `Images/large_banner.png` / `Images/fav_icon.png` (also kept as sketch-local `large_banner.png` / `favicon.png` + `BannerPng.h` / `FaviconPng.h`).

## Board profile (30-pin vs 38-pin)

At the top of `UDC-ServoTester.ino`, set **exactly one** of these to `1`. **Default is 30-pin** (Frank’s handy board):

```cpp
#define UDC_BOARD_38PIN 0
#define UDC_BOARD_30PIN 1   // default
```

For a 38-pin board, flip to:

```cpp
#define UDC_BOARD_38PIN 1
#define UDC_BOARD_30PIN 0
```

### 30-pin (default)

| Role | GPIO |
|------|------|
| Servo 1–6 | **13, 14, 16, 17, 25, 26** |
| WS2811 / NeoPixel data | **23** |

**Wiring note:** Chopper / classic UDC often uses GPIO **18** for LED1. This tester’s **30-pin default drives GPIO 23** — connect DIN to **23**, or flip the board profile to 38-pin if you intentionally want pin 18. The web panel title shows the live LED GPIO after `/api/state` sync.

### 38-pin

| Role | GPIO |
|------|------|
| Servo 1–6 | **13, 14, 25, 26, 27, 33** |
| WS2811 / NeoPixel data | **18** |

(30-pin map is a safe broken-out set with no servo/LED overlap. If 16/17 conflict with silkscreen/notes on a specific module, still use this documented set unless you change the profile block in the `.ino`.)

- Servo pulse range: **500–2500 μs** ↔ **0–100%**. Reset / neutral = **50% / 1500 μs**.
- Servo PWM refresh default: **`UDC_SERVO_HZ` 300** (set near the board-profile block in the `.ino`). Feels snappier on digital servos; if an analog/hobby servo only likes classic ~50 Hz frames, lower `UDC_SERVO_HZ` to `50`.
- Default LED count: **60**. Power longer strips from an external **5 V** supply; share ground with the ESP32.
- NeoPixel color order default: **GRB** (changeable in the UI).

## Libraries (Arduino Library Manager)

Install the **esp32** board package by **Espressif Systems** first (Boards Manager → search `esp32`).

Then: **Sketch → Include Library → Manage Libraries…** and search these **exact** names:

| Library Manager name | Author | Notes |
|----------------------|--------|--------|
| **ArduinoJson** | **Benoit Blanchon** | **Use v6.x** — this sketch uses `StaticJsonDocument` / ArduinoJson v6 API. Do **not** install Arduino_JSON, aJSON, or other JSON libraries. If Library Manager offers v7, pick a **6.x** release (version dropdown). |
| **ESP32Servo** | Kevin Harrington / John K. Bennett | ESP32 servo PWM (`#include <ESP32Servo.h>`). |
| **Adafruit NeoPixel** | Adafruit | WS2811/WS2812 (`#include <Adafruit_NeoPixel.h>`). |

Built-in with the ESP32 Arduino core (no Library Manager install): **WiFi**, **WebServer**.

## WiFi secrets

1. Copy `secrets.example.h` → `secrets.h`
2. Set `WIFI_SSID` / `WIFI_PASSWORD`

A local `secrets.h` for the Imperial Senate network may already exist on this machine. Never commit `secrets.h`.

## Board select & flash

1. Open `UDC-ServoTester/UDC-ServoTester.ino` in Arduino IDE
2. Set `UDC_BOARD_38PIN` / `UDC_BOARD_30PIN` for your hardware (see above)
3. Board package: **ESP32 Dev Module** (or matching DevKit)
4. Select the correct serial port
5. Upload
6. Serial Monitor at **115200** baud — note IP / hostname `UDC-ServoTester`
7. Browse to `http://<ip>/` (mDNS may also resolve `http://UDC-ServoTester.local/` on some networks)

## HTTP API

| Method | Path | Notes |
|--------|------|--------|
| GET | `/` | Embedded UI (cacheable) |
| GET | `/api/state` | Servos + LED state (`Cache-Control: no-store`) |
| POST | `/api/servo` | `{ "id": 1-6, "percent": 0-100 }` (cancels sweep) |
| POST | `/api/servo/reset` | `{ "id": 1-6 }` → 50% / 1500 μs (cancels sweep) |
| POST | `/api/servo/limits` | `{ "id", "setMin"/"setMax": true }` from current, or `{ "minPercent", "maxPercent" }` |
| POST | `/api/servo/sweep` | `{ "id" }` starts a **one-way** trip: current % ≥ 50 → min, % < 50 → max; press again while moving cancels. `{ "enabled": false }` cancels. |
| POST | `/api/led` | `count`, `brightness`, `speed` (0–100%), `invert` (bool), `effect`, `colorOrder`, `color1`/`color2` |

`GET /api/state` includes per-servo `minPercent`, `maxPercent`, `sweeping`, and LED `speed` / `invert` / `gpio`.

**Sweep:** not continuous oscillation — each press chooses direction from current percent (≥50 → min, <50 → max), animates to that end, and stops. Slider / Reset / Set Min / Set Max cancel an in-progress move. Invert Direction mirrors pixel indices so chase/wipe/scanner run the other way.

LED frames render in `loop()` so animations keep running while the web UI is open.

## Effects

Public UDC set: Solid, RGB step, Rainbow chase, Chase, DJ Light, Fade, Flow Stripe, Gradient, Pacifica, Rainbow, Twinkle, Fire, Breathe, Theater, Comet, Color wipe, Scanner, Police bar. Legacy White / Candle / Strobe sanitize like UDC (→ Solid / Fire / Chase).

## License

ISC — free and open source. See [LICENSE](../LICENSE).

## Support

Universal Droid Control is donationware. Donations help cover development but are never required:

[Donate via PayPal](https://www.paypal.com/donate/?hosted_button_id=BDQCA9ATGU298)
