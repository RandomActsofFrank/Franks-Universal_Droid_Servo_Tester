# Frank’s Universal Droid Servo Tester

ESP32 Servo & LED Tester — companion sketch for Franks-Universal_Droid_Control (not public yet due to ongoing development.)

Six-channel servo PWM tester (500–2500 μs) plus WS2811/NeoPixel strip with UDC-style LED effects and a built-in web UI.

## Open in Arduino IDE

1. Install the **esp32** board package by **Espressif Systems** (Arduino IDE → Boards Manager → search `esp32`).
2. Install libraries via **Sketch → Include Library → Manage Libraries…**, then search these **exact** names:

| Library Manager name | Author | Notes |
|----------------------|--------|--------|
| **ArduinoJson** | **Benoit Blanchon** | **Use v6.x** — this sketch uses `StaticJsonDocument` / ArduinoJson v6 API. Do **not** install Arduino_JSON, aJSON, or other JSON libraries. If Library Manager offers v7, pick a **6.x** release (or use the version dropdown). |
| **ESP32Servo** | Kevin Harrington / John K. Bennett | ESP32-oriented servo PWM library (`#include <ESP32Servo.h>`). |
| **Adafruit NeoPixel** | Adafruit | WS2811/WS2812 driver (`#include <Adafruit_NeoPixel.h>`). |

   Built-in with the ESP32 core (no Library Manager install): **WiFi**, **WebServer**.

3. Copy `UDC-ServoTester/secrets.example.h` → `UDC-ServoTester/secrets.h` and fill in WiFi credentials.
4. Open **`UDC-ServoTester/UDC-ServoTester.ino`** (folder name must match the `.ino` for the Arduino IDE).
5. Board: **ESP32 Dev Module** (or your DevKit). Upload, then open the printed IP / hostname `UDC-ServoTester` in a browser.

## Layout

| Path | Notes |
|------|--------|
| `UDC-ServoTester/` | Arduino sketch (open the `.ino` here) |
| `UDC-ServoTester/scripts/` | Helper scripts |
| `UDC-ServoTester/README.md` | Full pin maps, API, and flash steps |
