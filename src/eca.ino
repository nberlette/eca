/** 
 * ECA: Ethanol Content Analyzer
 *
 * Converts a 50-150hz flexfuel frequency to 0-5volt analog signal (PWM or DAC),
 * with optional CAN bus output for automotive integration.
 *  -> See README.md for full documentation.
 * ----------------------------------------------------------------------------
 *   MIT (c) 2021-2026 Nicholas Berlette <https://github.com/nberlette/eca>
 */

#include "eca_config.h"

#if ECA_ENABLE_DAC_OUT
  #include <MCP4725.h>
  MCP4725 dac;
#endif

#if ECA_ENABLE_CAN
  #include "eca_can.h"
  EcaCan ecaCan;
  EcaStatus ecaStatus = ECA_STATUS_UNKNOWN;
#endif

volatile uint16_t countTick  = 0;
volatile uint16_t revTick;

static long highTime = 0;
static long lowTime = 0;
static long tempPulse;

ISR(TIMER1_CAPT_vect)
{ // Pulse detected, interrupt triggered
  // save duration of last revolution
	revTick = ICR1;
  // restart timer for next revolution
	TCNT1 = 0;
}

ISR(TIMER1_OVF_vect) 
{ // counter overflow/timeout
  revTick = 0;
}

void setup()
{
  if (ECA_ENABLE_SERIAL == 1) 
  {
    Serial.begin(ECA_SERIAL_BAUDRATE);
  }
  pinMode(PIN_INPUT_SENSOR, INPUT);

#if ECA_ENABLE_PWM_OUT
  setPwmFrequency(PIN_OUTPUT_PWM, 1); 
#endif

  setupTimer();
  setVoltage(ECA_ERROR_V_DISCONNECTED, true);

#if ECA_ENABLE_CAN
  if (!ecaCan.begin()) {
    if (ECA_ENABLE_SERIAL == 1) {
      Serial.println(F("CAN init failed"));
    }
  }
#endif
}

void setupTimer ()
{           
	TCCR1A = 0; 
  // Falling edge trigger, Timer = CPU/256, noise-cancellation
	TCCR1B = 132; 
	TCCR1C = 0;
  // Enable input capture (ICP1) and overflow interrupt (OVF1)
	TIMSK1 = 33; 
	TCNT1 = 0;
}

void setVoltage (double volts, bool init = false) 
{
  const int maxVolts = ECA_VOLTAGE_RAIL;

#if ECA_ENABLE_PWM_OUT
  if (init) {
    pinMode(PIN_OUTPUT_PWM, OUTPUT);
    TCCR1B = TCCR1B & 0b11111000 | 0x01;
  }
  analogWrite(PIN_OUTPUT_PWM, int((PWM_MULTIPLIER * (volts / maxVolts))));
#endif

#if ECA_ENABLE_DAC_OUT
  if (init) { 
    dac.begin(ECA_DAC_I2C_ADDR);
  }
  dac.setVoltage(int(DAC_MULTIPLIER * (volts / maxVolts)), false);
#endif
}

int getTempC (unsigned long highTime, unsigned long lowTime) 
{
  // fuel temperature (degC)
	// 1 millisecond = -40C, 5 milliseconds = 125C
  unsigned long pulseTime = highTime + lowTime;
	float frequency = float(1000000 / pulseTime);
	float dutyCycle = 100 * (highTime / float(lowTime + highTime));
	float totalTime = float(1.0 / frequency);
	float period = float(100 - dutyCycle) * totalTime;

  return int((40.25 * (10 * period)) - 81.25);
}

int getEthanol (unsigned long pulseTime)
{
	float frequency = float(1000000 / pulseTime);
	// 20000 uS = 50 HZ - ~6667 uS = 150 HZ
	if (pulseTime >= 20100 || pulseTime <= 6400) 
  {
    if (pulseTime == 0) 
    { // sensor disconnected / short circuit
      setVoltage(ECA_ERROR_V_DISCONNECTED);
#if ECA_ENABLE_CAN
      ecaStatus = ECA_STATUS_DISCONNECTED;
#endif
    } 
    else if (pulseTime >= 20100) 
    { // contaminated fuel supply
      setVoltage(ECA_ERROR_V_CONTAMINATED);
#if ECA_ENABLE_CAN
      ecaStatus = ECA_STATUS_CONTAMINATED;
#endif
    }
    else if ((pulseTime <= 6400) && (pulseTime >= 1)) 
    { // high water content in fuel
      setVoltage(ECA_ERROR_V_HIGH_WATER);
#if ECA_ENABLE_CAN
      ecaStatus = ECA_STATUS_HIGH_WATER;
#endif
    }
    if (countTick < 2) 
    {
      countTick++; 
    }
		return;
	}

#if ECA_ENABLE_CAN
  ecaStatus = ECA_STATUS_OK;
#endif

  int eContent = frequency - (50 - ECA_ECONTENT_ADDER);
  return clamp(eContent, 0, 100);
}

float setVoltageFromEthanol (int ethanol)
{
  float desiredVoltage = mapf(ethanol, 0, 100, ECA_VOLTAGE_MIN, ECA_VOLTAGE_MAX);
  setVoltage(desiredVoltage, false);
  return desiredVoltage;
}

double mapf (double val, double x1, double x2, double y1, double y2)
{
	return (val - x1) * (y2 - y1) / (x2 - x1) + y1;
}

int clamp (int val, int min, int max)
{
  if (val < min) {
    val = min;
  } else if (val > max) {
    val = max;
  }
  return val;
}

int cToF (int tempC)
{
  return clamp(int(tempC * 1.8 + 32), -39, 250);
}

void setPwmFrequency(int pin, int divisor) 
{ 
  // This code snippet raises the timers linked to the PWM outputs
  // This way the PWM frequency can be raised or lowered. 
  // Prescaler of 1 sets PWM output to 32KHz (pin 3, 11)
  byte mode; 

  if(pin == 5 || pin == 6 || pin == 9 || pin == 10) {
    switch(divisor) {
      case 1: mode = 0x01; break;
      case 8: mode = 0x02; break;
      case 64: mode = 0x03; break;
      case 256: mode = 0x04; break;
      case 1024: mode = 0x05; break;
      default: return;
    }
    if(pin == 5 || pin == 6) {
      TCCR0B = TCCR0B & 0b11111000 | mode;
    } else {
      TCCR1B = TCCR1B & 0b11111000 | mode;
    }
  } else if(pin == 3 || pin == 11) {
    switch(divisor) {
      case 1: mode = 0x01; break;
      case 8: mode = 0x02; break;
      case 32: mode = 0x03; break;
      case 64: mode = 0x04; break;
      case 128: mode = 0x05; break;
      case 256: mode = 0x06; break;
      case 1024: mode = 0x7; break;
      default: return;
    }
    TCCR2B = TCCR2B & 0b11111000 | mode;
  }
}

void loop ()
{
  unsigned long highTime = pulseIn(PIN_INPUT_SENSOR, HIGH);
  unsigned long lowTime = pulseIn(PIN_INPUT_SENSOR, LOW);

  unsigned long pulseTime = highTime + lowTime;
	float frequency = float(1000000 / pulseTime);

  int eContent = getEthanol(pulseTime);
  float outputVoltage = setVoltageFromEthanol(eContent);

  int tempC = getTempC(highTime, lowTime);
	int tempF = cToF(tempC);

#if ECA_ENABLE_CAN
  if (ecaCan.ready()) {
    uint16_t voltageMv = (uint16_t)(outputVoltage * 1000);
    ecaCan.send((uint8_t)eContent, frequency, voltageMv,
                (int8_t)tempC, ecaStatus);
  }
#endif

  if (ECA_ENABLE_SERIAL == 1) 
  {
    Serial.print(F("Ethanol: "));
    Serial.print(eContent);
    Serial.print(F("%  •  Fuel Temp: "));
    Serial.print(tempC);
    Serial.print(F("°C ("));
    Serial.print(tempF);
    Serial.println(F("°F)"));
  }
  delay(ECA_REFRESH_DELAY_MS);
	countTick = 0;
}
