# UDC Servo & LED Tester

**Version:** `2026.10-42.05` (ISO-week: `YYYY.MM-WW.NN`)

Arduino IDE sketch for ESP32 DevKit boards (30-pin or 38-pin): six-servo PWM tester plus a WS2811/NeoPixel strip tester using the same public LED effects as Universal Droid Control (`UdcLedFx`).

WiFi hostname is **`UDC-ServoTester`** (set before `WiFi.begin`). Serial Monitor: **115200** baud.

Web UI serves `/banner.png` and `/favicon.png` from embedded copies of `Images/large_banner.png` / `Images/fav_icon.png` (also kept as sketch-local `large_banner.png` / `favicon.png` + `BannerPng.h` / `FaviconPng.h`).

## Versioning

Scheme: **`YYYY.MM-WW.NN`** (example: `2026.10-42.01`).

| Part | Meaning |
|------|---------|
| `YYYY` | Calendar year |
| `MM` | Calendar month |
| `WW` | **ISO week** number (`date +%V`) |
| `NN` | Build within that week |

| Rule | Behavior |
|------|----------|
| Week change | If today’s `YYYY.MM-WW` ≠ version prefix → set `YYYY.MM-WW.01` |
| Each push | Same ISO week → increment `.NN` (`…01` → `…02`). Week rollover already sets `.01` — no double bump |
| By request | Explicit bump / set via the script below |

Source of truth: `#define UDC_SERVO_TESTER_VERSION` in `UDC-ServoTester.ino`. The script also updates the web UI labels in `WebUi.h` and this README. Serial boot uses `kSketchVersion` (same define).

```bash
cd "UDC-ServoTester"

./scripts/bump-version.sh show          # print current
./scripts/bump-version.sh auto          # new week → .01, else +1
./scripts/bump-version.sh push          # same as auto (run before git push)
./scripts/bump-version.sh date          # force today's YYYY.MM-WW.01
./scripts/bump-version.sh set 2026.10-42.03
```

**Before pushing sketch changes**, bump so the commit includes the new version:

```bash
./scripts/bump-version.sh push
# then stage/commit the version files with your other changes, and git push
```

Or use the wrapper (bumps, commits **only** the version files if they changed, then `git push`):

```bash
./scripts/push.sh
# optional: ./scripts/push.sh origin main
```

No repo-wide git hook is installed (UDC root may already have hooks). An optional reminder sample lives at `hooks/pre-push.sample` — prefer `scripts/push.sh` or a manual `bump-version.sh push` instead.

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

- **ESP32Servo**
- **Adafruit NeoPixel**
- **ArduinoJson** (v6 recommended)

WiFi and WebServer come with the ESP32 Arduino core.

## WiFi secrets

1. Copy `secrets.example.h` → `secrets.h`
2. Set `WIFI_SSID` / `WIFI_PASSWORD`

A local `secrets.h` for the Imperial Senate network may already exist on this machine. **`secrets.h` is gitignored — never commit it.**

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
