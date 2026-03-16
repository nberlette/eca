/**
 * ECA: Ethanol Content Analyzer - CAN Bus Support
 *
 * Optional CAN bus output module. Only compiled when ECA_ENABLE_CAN is set
 * to 1 in eca_config.h.
 *
 * Requires an MCP2515-based CAN transceiver connected via SPI.
 * Uses the mcp_can library (https://github.com/coryjfowler/MCP_CAN_lib).
 * -------------------------------------------------------------------------
 *   MIT (c) 2021-2026 Nicholas Berlette <https://github.com/nberlette/eca>
 */

#ifndef ECA_CAN_H
#define ECA_CAN_H

#include "eca_config.h"

#if ECA_ENABLE_CAN

#include <mcp_can.h>
#include <SPI.h>

// =========================================================================
//  Status / error codes (enumerated)
//  These correspond 1:1 with the analog error voltages.
// =========================================================================
//
//  Code | Name              | Analog Voltage | Condition
//  -----|-------------------|----------------|----------------------------
//  0x00 | STATUS_OK         | 0.50-4.50V     | Normal valid reading
//  0x01 | STATUS_DISCONNECTED | 0.10V        | Sensor disconnected (<= 0 Hz)
//  0x02 | STATUS_CONTAMINATED | 4.80V        | Contaminated fuel (< 50 Hz)
//  0x03 | STATUS_HIGH_WATER | 4.90V          | High water content (> 150 Hz)
//  0xFF | STATUS_UNKNOWN    | —              | Unknown / initializing
// =========================================================================
enum EcaStatus : uint8_t {
  ECA_STATUS_OK            = 0x00,  // Normal valid ethanol reading
  ECA_STATUS_DISCONNECTED  = 0x01,  // Sensor disconnected / no signal
  ECA_STATUS_CONTAMINATED  = 0x02,  // Contaminated fuel (freq < 50 Hz)
  ECA_STATUS_HIGH_WATER    = 0x03,  // High water content (freq > 150 Hz)
  ECA_STATUS_UNKNOWN       = 0xFF   // Unknown / initializing
};

// =========================================================================
//  Status bit flags (byte 7 of CAN payload)
//  Individual bits for fault conditions — can be combined.
// =========================================================================
#define ECA_FLAG_SENSOR_OK        0x01  // bit 0: sensor connected & valid
#define ECA_FLAG_DISCONNECTED     0x02  // bit 1: sensor disconnected
#define ECA_FLAG_CONTAMINATED     0x04  // bit 2: contaminated fuel
#define ECA_FLAG_HIGH_WATER       0x08  // bit 3: high water content
#define ECA_FLAG_ERROR            0x80  // bit 7: any error present

// =========================================================================
//  CAN controller wrapper
// =========================================================================
class EcaCan {
public:
  EcaCan();

  /**
   * Initialize the MCP2515 CAN controller.
   * Returns true on success, false on failure.
   */
  bool begin();

  /**
   * Send the ECA CAN frame with current sensor data.
   *
   * @param ethanol   Ethanol content (0-100%)
   * @param freqHz    Sensor frequency in Hz (floating point)
   * @param voltageMv Output voltage in millivolts
   * @param tempC     Fuel temperature in degrees Celsius
   * @param status    EcaStatus enumerated code
   */
  void send(uint8_t ethanol, float freqHz, uint16_t voltageMv,
            int8_t tempC, EcaStatus status);

  /**
   * Check if enough time has elapsed since the last transmission
   * based on ECA_CAN_TX_INTERVAL_MS. Call this in the main loop
   * to throttle CAN output independently of the main refresh rate.
   */
  bool ready();

private:
  MCP_CAN _can;
  unsigned long _lastTxMs;

  /** Build status bit-flags byte from an EcaStatus code. */
  uint8_t _buildFlags(EcaStatus status);
};

// =========================================================================
//  Implementation (header-only for Arduino sketch simplicity)
// =========================================================================

EcaCan::EcaCan()
  : _can(ECA_CAN_CS_PIN), _lastTxMs(0) {}

bool EcaCan::begin() {
  // CAN_500KBPS corresponds to ECA_CAN_BAUDRATE default of 500 kbps.
  // MCP_ANY = accept all incoming messages (we only transmit, but set a sane default).
  if (_can.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ) == CAN_OK) {
    _can.setMode(MCP_NORMAL);
    return true;
  }
  return false;
}

bool EcaCan::ready() {
  unsigned long now = millis();
  if ((now - _lastTxMs) >= ECA_CAN_TX_INTERVAL_MS) {
    return true;
  }
  return false;
}

void EcaCan::send(uint8_t ethanol, float freqHz, uint16_t voltageMv,
                  int8_t tempC, EcaStatus status) {
  uint8_t data[8];

  // Byte 0: ethanol content (0-100)
  data[ECA_CAN_BYTE_ETHANOL] = ethanol;

  // Bytes 1-2: frequency * 10, big-endian (0.1 Hz resolution)
  uint16_t freqScaled = (uint16_t)(freqHz * ECA_CAN_FREQ_SCALE);
  data[ECA_CAN_BYTE_FREQ_HI] = (freqScaled >> 8) & 0xFF;
  data[ECA_CAN_BYTE_FREQ_LO] = freqScaled & 0xFF;

  // Bytes 3-4: output voltage in mV, big-endian
  data[ECA_CAN_BYTE_VOLTAGE_HI] = (voltageMv >> 8) & 0xFF;
  data[ECA_CAN_BYTE_VOLTAGE_LO] = voltageMv & 0xFF;

  // Byte 5: fuel temperature (degC + offset), clamped to 0-255
  int16_t tempEncoded = (int16_t)tempC + ECA_CAN_TEMP_OFFSET;
  if (tempEncoded < 0) tempEncoded = 0;
  if (tempEncoded > 255) tempEncoded = 255;
  data[ECA_CAN_BYTE_FUEL_TEMP] = (uint8_t)tempEncoded;

  // Byte 6: enumerated status code
  data[ECA_CAN_BYTE_STATUS] = (uint8_t)status;

  // Byte 7: status bit flags
  data[ECA_CAN_BYTE_FLAGS] = _buildFlags(status);

  // Transmit standard (11-bit) CAN frame, 8 data bytes
  _can.sendMsgBuf(ECA_CAN_ARBID, 0, 8, data);
  _lastTxMs = millis();
}

uint8_t EcaCan::_buildFlags(EcaStatus status) {
  uint8_t flags = 0;
  switch (status) {
    case ECA_STATUS_OK:
      flags = ECA_FLAG_SENSOR_OK;
      break;
    case ECA_STATUS_DISCONNECTED:
      flags = ECA_FLAG_DISCONNECTED | ECA_FLAG_ERROR;
      break;
    case ECA_STATUS_CONTAMINATED:
      flags = ECA_FLAG_CONTAMINATED | ECA_FLAG_ERROR;
      break;
    case ECA_STATUS_HIGH_WATER:
      flags = ECA_FLAG_HIGH_WATER | ECA_FLAG_ERROR;
      break;
    default:
      flags = ECA_FLAG_ERROR;
      break;
  }
  return flags;
}

#endif // ECA_ENABLE_CAN
#endif // ECA_CAN_H
