# Smart AC Controller

ESP32-based Daikin AC controller with a local web UI, IR transmission, BMP280 room sensing, timers, sleep mode, and automatic temperature control.

## Hardware

- ESP32 Dev Module
- IR LED/transmitter circuit on GPIO 4
- BMP280 temperature/pressure sensor
- I2C wiring:
  - SDA: GPIO 21
  - SCL: GPIO 22
- LittleFS is used to serve the web UI from the ESP32.

## Features

- Local web remote for Daikin AC
- Power ON/OFF
- Temperature up/down
- Fan speed: Auto, 1-5
- Modes: Cool, Fan, Dry
- Advanced controls:
  - Power / Powerchill
  - Economy
  - Quiet fan mode
  - Horizontal swing
  - Vertical swing
  - Coanda / Comfort airflow
- Auto OFF timer
- Sleep mode with temperature ramp-up
- Cyclical ON/OFF timer
- Auto Mode based on room temperature
- NTP clock display
- BMP280 temperature and pressure display

## Control Logic

Some modes are mutually exclusive to match typical Daikin behavior:

- Turning on Powerchill turns off Economy.
- Turning on Economy turns off Powerchill.
- Turning on Coanda turns off Vertical swing.
- Turning on Vertical swing turns off Coanda.
- Turning on Powerchill also turns off Coanda and turns on Vertical swing.
- Choosing a normal fan speed turns off Quiet mode.
- Quiet mode uses Daikin's quiet fan setting.

## Auto Mode

Auto Mode uses the BMP280 room temperature.

- If AC is OFF and room temperature is greater than or equal to the ON limit, the AC turns ON.
- If AC is ON and room temperature is less than or equal to the OFF limit, the AC turns OFF.
- The loop checks the sensor about once per second.
- Auto Mode also checks immediately when enabled from the web UI.
- The OFF limit is forced to stay lower than the ON limit to avoid rapid switching.

Default limits:

- ON at `30.0 C`
- OFF at `27.0 C`

## Project Structure

```text
src/main.cpp       Firmware, WiFi, IR, sensor, web routes, control logic
data/index.html    Web UI layout
data/app.js        Web UI JavaScript and API calls
data/style.css     Web UI styling
platformio.ini     PlatformIO build configuration
```

## Setup

Install PlatformIO, then open this folder as a PlatformIO project.

Before uploading, update the WiFi credentials in `src/main.cpp`:

```cpp
const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";
```

Important: do not push real WiFi credentials to a public GitHub repository.

## Build

```powershell
python -m platformio run
```

## Upload Firmware

```powershell
python -m platformio run -t upload
```

## Upload Web UI

The files in `data/` must be uploaded to LittleFS:

```powershell
python -m platformio run -t uploadfs
```

Run this whenever `index.html`, `app.js`, or `style.css` changes.

## Serial Monitor

```powershell
python -m platformio device monitor
```

Monitor speed is `115200`.

## Build Notes

The project pins ESP32/Arduino and library versions in `platformio.ini` to avoid linker mismatches:

- `espressif32@7.0.1`
- `framework-arduinoespressif32@3.20017.241212`
- `toolchain-xtensa-esp32@8.4.0+2021r2-patch5`
- `IRremoteESP8266@2.9.0`
- `ESPAsyncWebServer@3.11.0`
- `AsyncTCP@3.4.10`

If PlatformIO reports strange linker errors, clean and rebuild:

```powershell
python -m platformio run -t clean
python -m platformio run
```

## Notes

- The current Daikin implementation uses `IRDaikinESP`.
- Remote-specific buttons such as Good Sleep / Display may require capturing and replaying the exact IR packet from the physical remote.
- The web UI is served locally by the ESP32; no cloud service is required.
