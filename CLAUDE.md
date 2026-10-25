# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project overview

"Météo des Loggias" is a two-part home weather-station project:

1. **ESP8266 firmware** (`.ino` + `.cpp`/`.h` files at repo root) — reads a DS18B20 temperature probe, an AHT10/20 temperature+humidity sensor, and an analog luminosity sensor, then POSTs the readings as JSON over HTTPS to a Google Cloud Function.
2. **Google Cloud Function** (`server/index.js`) — receives the POSTed readings and appends them as rows to a Google Sheet via the Sheets API.

Data flow: sensors → ESP8266 firmware → HTTPS POST (JSON) → Cloud Function → Google Sheet (one row per reading).

## Commands

There is no build system, linter, or test runner configured (no `arduino-cli`, no npm scripts, no Makefile).

- **Firmware**: built/flashed via the Arduino IDE targeting an ESP8266 board. Library/toolchain paths are configured in `.vscode/c_cpp_properties.json` (ESP8266 core 3.1.2, libraries under `~/dev/arduino/libraries` and `~/.arduino15/libraries`).
- **Cloud Function** (`server/`): install deps with `yarn install` (root `package.json` + `yarn.lock` provide `@google-cloud/functions-framework` and `googleapis`). Requires the gitignored credentials `apikey.json` and `server/meteo-des-loggias-*.json` (service account key) to run.
- `test.js` at the repo root is an ad-hoc manual script, not a real test suite — it calls `appendSheetRow`/`getJwt`/`getApiKey` with a hardcoded payload. These now live in `server/index.js`, so paths would need adjusting before it can run as-is.
- **Web configurator**: `tools/configurateur.html` is a standalone WebSerial page (Chrome/Edge, `localhost` or `https`) that connects to the board, identifies it and wraps every serial command in buttons. Serve with `python3 -m http.server` from `tools/`. If you add/rename a serial command or a valid device name, update the buttons and `DEFAULT_DEVICES` there too.
- **Firmware runtime CLI**: over Serial at 9600 baud, the firmware accepts commands to inspect/change config without reflashing (WiFi credentials, POST URL, device name, TLS fingerprint, pause/resume posting). Full list in `serial_cmd.cpp`'s `printHelp()` (`GET_URL`, `SET_URL`, `SET_WIFI`, `SET_DEVICE`, `LIST_VALID_DEVICES`, `SET_FINGERPRINT`, `PAUSE`/`RESUME`, `STATUS`, `HELP`).

## Architecture

### Firmware (root directory)

- `meteo-des-loggias.ino` — main sketch, built around **deep sleep**: `setup()` initializes the sensors, loads `AppConfig` from EEPROM, connects to WiFi, reads all three sensors and POSTs a JSON body to the Cloud Function endpoint using `WiFiClientSecure` with TLS **fingerprint pinning** (not full certificate validation). It then opens a short Serial listening window (`SERIAL_WINDOW_MS`, see `config.h`) before calling `ESP.deepSleep(DEEP_SLEEP_INTERVAL_US)`, which fully resets the chip (all RAM, including `gConfig`/`gPaused`, is lost — `setup()` runs again from scratch on wake). Requires GPIO16 (D0) wired to RST for auto-wake. Typing `PAUSE` during the Serial window (or later, since `loop()` is only reached in this state) skips deep sleep and keeps the board awake indefinitely for configuration; `RESUME` sends one more reading and re-enters the deep-sleep cycle.
- `config.h` — compile-time defaults (WiFi SSID/password, POST URL, device name, TLS fingerprint, timing constants). These are only fallbacks: whatever is persisted in EEPROM takes precedence at boot.
- `storage.h`/`storage.cpp` — hand-rolled EEPROM layout (512 bytes) storing WiFi credentials, POST URL, device name, and TLS fingerprint behind a magic-byte header. The exact byte offsets/lengths are documented in the comment block at the top of `storage.h`; if you change the layout, keep that comment, the `#define` offsets, and the read/write logic in `storage.cpp` all in sync.
- `serial_cmd.h`/`serial_cmd.cpp` — parses newline-terminated Serial commands to read/update `AppConfig` fields at runtime (persisting through `storage.cpp`) and to pause/resume HTTP posting. `SET_DEVICE` is validated against an allow-list (`VALID_DEVICE_NAMES`) tied to actual physical sensor locations — add new physical sensors there.
- Global mutable state (`gConfig: AppConfig`, `gPaused: bool`) is defined in the `.ino` file and shared with `serial_cmd.cpp` via `extern`.
- User-facing Serial output and error messages are in French — match that convention when touching `serial_cmd.cpp`.

### Server (`server/`)

- `server/index.js` — single HTTP Cloud Function, entry point `helloHttp` (exported as `reply`). Accepts either one reading or an array of readings, maps each to a spreadsheet row (`[timestamp, sensorName, temperature1, temperature2, humidity, luminosity]`), and appends them to a hardcoded spreadsheet ID via a JWT service-account auth (`server/meteo-des-loggias-*.json`) plus an API key (`apikey.json`).
- The repo is mid-migration: this Cloud Function source used to live at the repo root (`index.js`, now deleted in the working tree) and is being moved into `server/`.

## Reference docs

- `docs/CN3163.PDF` — datasheet.
- `docs/optimization options.md` — French notes on ESP8266 power-saving techniques (deep sleep, WiFi off, CPU underclocking). Deep sleep + WiFi-off-before-sleep are now implemented (see `meteo-des-loggias.ino`); CPU underclocking and light sleep are not.
