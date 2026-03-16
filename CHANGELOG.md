# Changelog

All notable changes to **ECA (Ethanol Content Analyzer)** are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

---

## [2.0.0] — 2026-03-16

### Added
- **Optional CAN bus output** — opt-in via `ECA_ENABLE_CAN` in `eca_config.h`. Requires an MCP2515 CAN transceiver module. When disabled (default), behavior is identical to v1.0.x.
- **Configurable CAN arbitration ID** — `ECA_CAN_ARBID` (default `0xEC`).
- **Configurable CAN payload scaling** — frequency transmitted at 0.1 Hz resolution, voltage in millivolts, temperature with +40 °C offset.
- **CAN error/status support** — CAN frames include both an enumerated status code and individual bit flags that map 1:1 to the existing analog error voltages:
  - `0x01` Disconnected (analog `0.10 V`)
  - `0x02` Contaminated (analog `4.80 V`)
  - `0x03` High Water (analog `4.90 V`)
- **DBC schema file** (`eca.dbc`) — canonical CAN message definition for use with CAN tools, dashboards, and loggers.
- **Central configuration header** (`src/eca_config.h`) — all compile-time settings in one place, replacing scattered `#define` directives in the main sketch.
- **CAN module header** (`src/eca_can.h`) — self-contained CAN abstraction, compiled out cleanly when CAN is disabled.

### Changed
- **README.md** — significantly expanded with hardware pinout, voltage/frequency mapping tables, error condition documentation, CAN configuration guide, DBC usage instructions, and integration examples.
- **Copyright years** updated to 2021–2026 across LICENSE, source files, and documentation.
- **Serial output** now uses `F()` macro for flash-string storage to reduce SRAM usage.
- Firmware version bumped from `1.0.1` to `2.0.0`.
- Analog output `#ifdef` guards replaced with proper `#if` preprocessor checks.
- Error voltage constants centralized in config header (`ECA_ERROR_V_DISCONNECTED`, `ECA_ERROR_V_CONTAMINATED`, `ECA_ERROR_V_HIGH_WATER`).

### Fixed
- `setVoltage()` DAC address now uses configurable `ECA_DAC_I2C_ADDR` instead of a hardcoded value.

---

## [1.0.0] — 2021-09-24

_Initial release._

### Added
- FlexFuel sensor frequency measurement using Timer1 input capture (50–150 Hz → 0–100% ethanol).
- PWM analog output (0–5 V) on configurable pin.
- MCP4725 12-bit I2C DAC output support.
- Analog error voltage signaling: `0.10 V` (disconnected), `4.80 V` (contaminated), `4.90 V` (high water).
- Fuel temperature calculation from duty cycle.
- Serial logging of ethanol content and fuel temperature.
- Bundled MCP4725 library with waveform example.

> **Note:** This entry was reconstructed from the initial commit (`cc2caf3`, 2021-09-24). Intermediate changes between v1.0.0 and v2.0.0, if any, were not tagged and cannot be reliably reconstructed from the repository history.
