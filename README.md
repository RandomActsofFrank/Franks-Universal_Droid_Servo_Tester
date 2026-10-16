# Frank’s Universal Droid Servo Tester

ESP32 Servo & LED Tester — companion sketch for [Franks-Universal_Droid_Control](https://github.com/RandomActsofFrank/Franks-Universal_Droid_Control).

Six-channel servo PWM tester (500–2500 μs) plus WS2811/NeoPixel strip with UDC-style LED effects and a built-in web UI.

## Open in Arduino IDE

1. Copy `UDC-ServoTester/secrets.example.h` → `UDC-ServoTester/secrets.h` and fill in WiFi credentials (`secrets.h` is gitignored).
2. Open **`UDC-ServoTester/UDC-ServoTester.ino`** (folder name must match the `.ino` for the Arduino IDE).
3. Board: ESP32 Dev Module (or your DevKit). Install libraries: **ESP32Servo**, **Adafruit NeoPixel**, **ArduinoJson** (v6).
4. Upload, then open the printed IP / hostname `UDC-ServoTester` in a browser.

## Layout

| Path | Notes |
|------|--------|
| `UDC-ServoTester/` | Arduino sketch (open the `.ino` here) |
| `UDC-ServoTester/scripts/` | `bump-version.sh`, `push.sh` (ISO-week versioning) |
| `UDC-ServoTester/README.md` | Full pin maps, API, versioning rules |

## Versioning

ISO-week scheme `YYYY.MM-WW.NN` — see `UDC-ServoTester/README.md` and run `./scripts/bump-version.sh` from the sketch folder before pushing sketch changes.
