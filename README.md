# ECA: Ethanol Content Analyzer

An Arduino-based ethanol content analyzer that converts a standard automotive
FlexFuel sensor's digital frequency output (50–150 Hz) into a proportional
analog voltage (0.50–4.50 V) suitable for ECU inputs, wideband controllers,
datalogs, and in-dash gauges. An optional CAN bus output mode is also available
for direct integration with CAN-equipped dashboards, loggers, and standalone
ECUs.

---

## Features

- **FlexFuel sensor input** — reads the 50–150 Hz square-wave from any
  standard GM-style FlexFuel sensor via Timer1 input capture.
- **Dual analog output** — simultaneous PWM and 12-bit MCP4725 I2C DAC output
  for maximum compatibility.
- **Configurable voltage range** — default 0.50–4.50 V maps linearly to
  0–100% ethanol.
- **Error/fault signaling** — dedicated out-of-band voltages for sensor
  disconnect, contaminated fuel, and high water content.
- **Optional CAN bus output** — opt-in at compile time; transmits ethanol
  content, frequency, voltage, fuel temperature, and status over CAN using a
  configurable arbitration ID.
- **Fuel temperature** — derived from the sensor's duty cycle, output via
  serial and (optionally) CAN.
- **Serial debug output** — logs ethanol percentage and fuel temperature over
  UART at configurable baud rate.
- **Single config header** — all compile-time options centralized in
  `src/eca_config.h`.

---

## Hardware

### Supported Boards

| Board | MCU | Notes |
|---|---|---|
| Arduino Nano | ATmega328P | Primary target |
| Arduino Uno | ATmega328P | Fully compatible |
| Other ATmega328P boards | ATmega328P | Should work with correct pin mapping |

### Required Components

| Component | Purpose |
|---|---|
| FlexFuel sensor (GM-style) | Ethanol content input (50–150 Hz square-wave) |
| MCP4725 breakout board | 12-bit I2C DAC for precision analog output |
| MCP2515 CAN module _(optional)_ | CAN bus transceiver (only needed if CAN is enabled) |

---

## Pinout

| Type | Pin(s) | Description |
|---|---|---|
| Sensor Input | `D10` | FlexFuel sensor signal (Timer1 / ICP1) |
| PWM Output | `D9` | Built-in PWM analog output |
| DAC Output | `A4 / A5` | MCP4725 I2C (SDA / SCL) |
| CAN SPI _(optional)_ | `D10–D13` | MCP2515 SPI bus (CS configurable in `eca_config.h`) |

> **Note:** When CAN is enabled, the SPI chip-select pin defaults to `D10`. If
> your wiring conflicts with the sensor input pin, change `ECA_CAN_CS_PIN` in
> `eca_config.h` and re-wire accordingly.

---

## Frequency → Ethanol → Voltage Mapping

The FlexFuel sensor outputs a square-wave whose frequency is proportional to
ethanol content. The ECA firmware maps this linearly:

| Sensor Freq | Ethanol | Analog Output |
|:---|:---:|:---|
| `50 Hz` | `0%` | `0.50 V` |
| `100 Hz` | `50%` | `2.50 V` |
| `150 Hz` | `100%` | `4.50 V` |

### Error Conditions (Analog)

Out-of-range frequencies trigger distinct error voltages so downstream devices
can distinguish faults from valid readings:

| Condition | Frequency | Analog Voltage | Description |
|:---|:---|:---|:---|
| **Disconnected** | `≤ 0 Hz` | `0.10 V` | Sensor unplugged or short circuit |
| **Contaminated** | `< 50 Hz` | `4.80 V` | Fuel frequency below valid range |
| **High Water** | `> 150 Hz` | `4.90 V` | Water content above safe threshold |

These error voltages are intentionally placed outside the normal 0.50–4.50 V
operating range so they can be reliably detected by an ECU or gauge.

---

## CAN Bus Output (Optional)

CAN support is **disabled by default** and must be opted into at compile time.
When enabled, the ECA transmits an 8-byte CAN frame at a configurable interval
alongside the normal analog output (analog behavior is always preserved).

### Enabling CAN

In `src/eca_config.h`, set:

```c
#define ECA_ENABLE_CAN  1
```

An MCP2515-based CAN transceiver module connected via SPI is required.

### CAN Configuration Options

All CAN settings are in `src/eca_config.h`:

| Option | Default | Description |
|---|---|---|
| `ECA_ENABLE_CAN` | `0` | Master enable (0 = off, 1 = on) |
| `ECA_CAN_ARBID` | `0xEC` (236) | 11-bit standard arbitration ID |
| `ECA_CAN_BAUDRATE` | `500000` | CAN bus speed in bps |
| `ECA_CAN_CS_PIN` | `10` | MCP2515 SPI chip-select pin |
| `ECA_CAN_TX_INTERVAL_MS` | `100` | Transmission interval (ms) |

### CAN Payload Layout

Each CAN frame is 8 bytes:

| Byte | Signal | Unit | Encoding |
|:---|:---|:---|:---|
| 0 | Ethanol Content | `%` | Raw value 0–100 |
| 1–2 | Sensor Frequency | `Hz` | Big-endian, ×10 (0.1 Hz resolution) |
| 3–4 | Output Voltage | `mV` | Big-endian, millivolts |
| 5 | Fuel Temperature | `°C` | Raw + 40 offset (0 = −40 °C) |
| 6 | Status Code | — | Enumerated (see below) |
| 7 | Status Flags | — | Bit field (see below) |

### CAN Status / Error Codes

The CAN status byte maps 1:1 to the analog error voltages:

| Code | Name | Analog Equiv. | Condition |
|:---|:---|:---|:---|
| `0x00` | `OK` | `0.50–4.50 V` | Normal valid reading |
| `0x01` | `Disconnected` | `0.10 V` | Sensor disconnected (≤ 0 Hz) |
| `0x02` | `Contaminated` | `4.80 V` | Contaminated fuel (< 50 Hz) |
| `0x03` | `HighWater` | `4.90 V` | High water content (> 150 Hz) |
| `0xFF` | `Unknown` | — | Initializing / unknown state |

### CAN Status Flags (Byte 7)

Individual bits for downstream fault detection:

| Bit | Mask | Meaning |
|:---|:---|:---|
| 0 | `0x01` | Sensor OK |
| 1 | `0x02` | Sensor disconnected |
| 2 | `0x04` | Contaminated fuel |
| 3 | `0x08` | High water content |
| 7 | `0x80` | Any error present |

During normal operation, only bit 0 (`0x01`) is set. During any fault, bit 7
is always set in addition to the specific fault bit.

---

## DBC File

The included [`eca.dbc`](eca.dbc) file is the canonical CAN message schema for
the ECA output. Load it into your CAN tool of choice to automatically decode
ECA frames:

- **Vector CANdb++ / CANalyzer / CANoe**
- **SavvyCAN**
- **PCAN-Explorer / PCAN-View**
- **BusMaster**
- **python-can** / **cantools** (Python)

The DBC defines a single message `ECA_Data` (ID `0x0EC`) with signals for
ethanol content, sensor frequency, output voltage, fuel temperature, status
code, and status flags. Signal comments describe encoding details and error
condition semantics.

### Example: Decoding with Python cantools

```python
import cantools
db = cantools.database.load_file('eca.dbc')
msg = db.get_message_by_name('ECA_Data')
data = msg.decode(b'\x32\x01\xF4\x08\xCA\x3C\x00\x01')
# {'EthanolContent': 50, 'SensorFrequency': 50.0, 'OutputVoltage': 2250,
#  'FuelTemperature': 20, 'StatusCode': 0, 'StatusFlags': 1}
```

---

## Configuration

All compile-time options are in [`src/eca_config.h`](src/eca_config.h). Edit
this file before uploading to your board.

### General Settings

| Option | Default | Description |
|---|---|---|
| `ECA_VERSION` | `"2.0.0"` | Firmware version string |
| `PIN_INPUT_SENSOR` | `10` | Sensor input pin |
| `PIN_OUTPUT_PWM` | `9` | PWM output pin |
| `ECA_ENABLE_SERIAL` | `1` | Enable serial debug output |
| `ECA_SERIAL_BAUDRATE` | `9600` | Serial baud rate |
| `ECA_ENABLE_PWM_OUT` | `1` | Enable PWM output |
| `ECA_ENABLE_DAC_OUT` | `1` | Enable MCP4725 DAC output |
| `ECA_DAC_I2C_ADDR` | `0x60` | MCP4725 I2C address |
| `ECA_VOLTAGE_MIN` | `0.5` | Voltage at 0% ethanol |
| `ECA_VOLTAGE_MAX` | `4.5` | Voltage at 100% ethanol |
| `ECA_REFRESH_DELAY_MS` | `1000` | Main loop delay (ms) |

### CAN Settings

See the [CAN Bus Output](#can-bus-output-optional) section above.

---

## Integration Examples

### Standalone ECU (Analog)

Wire the PWM or DAC output directly to a spare analog input on your standalone
ECU (Megasquirt, Haltech, AEM, Link, etc.). Configure the input as a
0–5 V sensor and set up a calibration table:

| Voltage | Ethanol % |
|---|---|
| 0.50 V | 0% |
| 2.50 V | 50% |
| 4.50 V | 100% |

Program fault detection thresholds at `< 0.30 V` (disconnected) and
`> 4.60 V` (contaminated or high water).

### CAN Dashboard / Logger

1. Enable CAN in `eca_config.h` (`ECA_ENABLE_CAN 1`).
2. Connect the MCP2515 module's CAN-H / CAN-L to your CAN bus.
3. Import `eca.dbc` into your dashboard or logging software.
4. The ECA will broadcast `ECA_Data` frames at the configured interval
   (default 10 Hz) on arbitration ID `0xEC`.

### AIM / RaceCapture / Custom Display

Use the DBC file to add the ECA signals to your channel list. The
`StatusCode` and `StatusFlags` signals allow you to trigger warnings or
alarms when a sensor fault is detected.

---

## Building & Uploading

1. Open `src/eca.ino` in the [Arduino IDE](https://www.arduino.cc/en/software)
   (1.8+) or [PlatformIO](https://platformio.org/).
2. Edit `src/eca_config.h` to match your hardware and preferences.
3. If using the MCP4725 DAC, ensure the library in `libraries/MCP4725/` is
   available in your Arduino libraries path.
4. If CAN is enabled, install the
   [MCP_CAN library](https://github.com/coryjfowler/MCP_CAN_lib) via the
   Arduino Library Manager.
5. Select your board (Arduino Nano / Uno) and upload.

---

## Repository Structure

```
eca/
├── src/
│   ├── eca.ino          # Main firmware sketch
│   ├── eca_config.h     # Central configuration header
│   └── eca_can.h        # Optional CAN bus module
├── libraries/
│   └── MCP4725/         # Bundled MCP4725 12-bit DAC library
│       ├── MCP4725.h
│       ├── MCP4725.cpp
│       └── examples/
│           └── waveform/
│               └── waveform.ino
├── eca.dbc              # CAN message schema (DBC format)
├── CHANGELOG.md         # Version history
├── LICENSE              # MIT License
└── README.md            # This file
```

---

## License

[MIT](LICENSE) © 2021–2026 [Nicholas Berlette](https://github.com/nberlette)
