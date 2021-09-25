# `ECA: Ethanol Content Analyzer`

Ethanol Content Analyzer for Arduino Nano or Uno.

Converts digital FlexFuel sensor data (`50~150 Hz`) into analog (`0-5v` or `0.5-4.5v`), for tuners, datalogs, and in-dash gauges.

## `Pins`

| Type       | Pin      | Description         |
| ---------- | -------- | ------------------- |
| Sensor In  | `D8`     | `TIMER1 / ICP1`     |
| PWM Output | `D3/D11` | Built-in PWM driver |
| DAC Output | `A4/A5`  | MCP4725 12bit DAC   |

## `Hz -> E % -> V`

| Input (Hz) | E (%)  | Output (V)                 |
| :--------- | :----: | :------------------------- |
| `50 hz`    | ` 0 %` | `0.50v`                    |
| `100 hz`   | `50 %` | `2.25v`                    |
| `150 hz`   | `100%` | `4.50v`                    |
| **Errors** |        |                            |
| `< 50 hz`  | `---`  | `4.80v` - contaminated     |
| `> 150 hz` | `---`  | `4.90v` - high water level |
| `<= 0 hz`  | `---`  | `0.10v` - disconnected     |

## `License`

[MIT](https://mit-license.org) © [Nicholas Berlette](https://nick.berlette.com)
