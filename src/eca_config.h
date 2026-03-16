/**
 * ECA: Ethanol Content Analyzer - Configuration
 *
 * Central configuration header for compile-time options.
 * Edit these values to match your hardware setup and preferences.
 * -------------------------------------------------------------------------
 *   MIT (c) 2021-2026 Nicholas Berlette <https://github.com/nberlette/eca>
 */

#ifndef ECA_CONFIG_H
#define ECA_CONFIG_H

// -------------------------------------------------------------------------
//  Firmware version
// -------------------------------------------------------------------------
#define ECA_VERSION           "2.0.0"

// -------------------------------------------------------------------------
//  Pin assignments
// -------------------------------------------------------------------------
#define PIN_INPUT_SENSOR      10     // FlexFuel sensor input (Timer1 / ICP1)
#define PIN_OUTPUT_PWM        9      // PWM analog output

// -------------------------------------------------------------------------
//  Serial / debug output
// -------------------------------------------------------------------------
#define ECA_ENABLE_SERIAL     1      // 1 = enable serial logging, 0 = disable
#define ECA_SERIAL_BAUDRATE   9600

// -------------------------------------------------------------------------
//  Analog output modes
// -------------------------------------------------------------------------
#define ECA_ENABLE_PWM_OUT    1      // 1 = enable PWM output on PIN_OUTPUT_PWM
#define ECA_ENABLE_DAC_OUT    1      // 1 = enable MCP4725 I2C DAC output
#define ECA_DAC_I2C_ADDR      0x60   // MCP4725 I2C address (0x60 or 0x62/0x63)

// -------------------------------------------------------------------------
//  Output scaling
// -------------------------------------------------------------------------
#define PWM_MULTIPLIER        255    // 8-bit PWM range
#define DAC_MULTIPLIER        4095   // 12-bit DAC range

// -------------------------------------------------------------------------
//  Voltage mapping
// -------------------------------------------------------------------------
//  Ethanol 0%   -> VOLTAGE_MIN (default 0.50V)
//  Ethanol 100% -> VOLTAGE_MAX (default 4.50V)
#define ECA_VOLTAGE_MIN       0.5
#define ECA_VOLTAGE_MAX       4.5
#define ECA_VOLTAGE_RAIL      5.0    // Full-scale voltage

// -------------------------------------------------------------------------
//  Analog error voltages
// -------------------------------------------------------------------------
#define ECA_ERROR_V_DISCONNECTED  0.10   // sensor disconnected (<= 0 Hz)
#define ECA_ERROR_V_CONTAMINATED  4.80   // contaminated fuel   (< 50 Hz)
#define ECA_ERROR_V_HIGH_WATER    4.90   // high water content  (> 150 Hz)

// -------------------------------------------------------------------------
//  Timing
// -------------------------------------------------------------------------
#define ECA_REFRESH_DELAY_MS  1000   // main loop delay (ms)

// -------------------------------------------------------------------------
//  Ethanol calculation offsets
// -------------------------------------------------------------------------
#define ECA_ECONTENT_ADDER    0
#define ECA_ECONTENT_FIXED    0

// -------------------------------------------------------------------------
//  CAN bus support (opt-in)
// -------------------------------------------------------------------------
//  Set ECA_ENABLE_CAN to 1 to enable CAN bus output.
//  Requires an MCP2515-based CAN transceiver module connected via SPI.
//  When disabled (default), no CAN code is compiled and behavior is
//  identical to the original analog-only firmware.
// -------------------------------------------------------------------------
#define ECA_ENABLE_CAN        0      // 0 = disabled (default), 1 = enabled

// CAN arbitration ID (11-bit standard)
#define ECA_CAN_ARBID         0xEC   // default: 236 (0xEC)

// CAN bus speed (bits/sec)
#define ECA_CAN_BAUDRATE      500000 // 500 kbps (most automotive networks)

// CAN chip-select pin for MCP2515 module
#define ECA_CAN_CS_PIN        10     // SPI chip select (change if shared)

// CAN transmission interval (ms)  — how often CAN frames are sent
#define ECA_CAN_TX_INTERVAL_MS  100  // 10 Hz update rate

// -------------------------------------------------------------------------
//  CAN payload layout (byte positions within the 8-byte CAN data frame)
// -------------------------------------------------------------------------
//  Byte 0: Ethanol content (0-100%, raw value, scale 1, offset 0)
//  Byte 1: Sensor frequency high byte  (Hz * 10, big-endian)
//  Byte 2: Sensor frequency low byte
//  Byte 3: Output voltage high byte    (mV, big-endian)
//  Byte 4: Output voltage low byte
//  Byte 5: Fuel temperature (degC + 40 offset, 0 = -40°C)
//  Byte 6: Status / error code (enumerated, see eca_can.h)
//  Byte 7: Status bit flags
// -------------------------------------------------------------------------
#define ECA_CAN_BYTE_ETHANOL      0
#define ECA_CAN_BYTE_FREQ_HI      1
#define ECA_CAN_BYTE_FREQ_LO      2
#define ECA_CAN_BYTE_VOLTAGE_HI   3
#define ECA_CAN_BYTE_VOLTAGE_LO   4
#define ECA_CAN_BYTE_FUEL_TEMP    5
#define ECA_CAN_BYTE_STATUS       6
#define ECA_CAN_BYTE_FLAGS        7

// -------------------------------------------------------------------------
//  CAN signal scaling
// -------------------------------------------------------------------------
//  Frequency is transmitted as (Hz * 10) to provide 0.1 Hz resolution
//  Voltage is transmitted as millivolts (mV)
//  Temperature is transmitted as (degC + 40) so 0x00 = -40°C
// -------------------------------------------------------------------------
#define ECA_CAN_FREQ_SCALE    10     // frequency multiplier
#define ECA_CAN_TEMP_OFFSET   40     // temperature offset (degC)

#endif // ECA_CONFIG_H
