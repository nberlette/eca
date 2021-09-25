/** 
 * ECA: Ethanol Content Analyzer
 *
 * Converts a 50-150hz flexfuel frequency to 0-5volt analog signal (PWM or DAC)
 *  -> See README.md
 * ----------------------------------------------------------------------------
 *    MIT © 2021 Nicholas Berlette <https://github.com/nberlette/eca>
 */

#define VERSION           "1.0.1"

#define PIN_INPUT_SENSOR  10
#define PIN_OUTPUT_PWM    9

#define ENABLE_SERIAL     1
#define SERIAL_BAUDRATE   9600

#define ENABLE_DAC_OUT    1
#define ENABLE_PWM_OUT    1

#define PWM_MULTIPLIER    255
#define DAC_MULTIPLIER    4095

#ifdef ENABLE_DAC_OUT
  #include <MCP4725.h>
  MCP4725 dac;
#endif

const int voltageMin    = 0.5;
const int voltageMax    = 4.5;

const int eContentAdder = 0;
const int eContentFixed = 0;

const int refreshDelay      = 1000;

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
  if (ENABLE_SERIAL == 1) 
  {
    Serial.begin(SERIAL_BAUDRATE);
  }
  pinMode(PIN_INPUT_SENSOR, INPUT);

  if (defined(ENABLE_PWM_OUT) && ENABLE_PWM_OUT == 1)
  {
    setPwmFrequency(PIN_OUTPUT_PWM, 1); 
  }
  setupTimer();
  setVoltage(0.1, true);
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
  const int maxVolts = 5.0;

  if (defined(ENABLE_PWM_OUT) && ENABLE_PWM_OUT == 1)
  {
    if (init) {
      pinMode(PIN_OUTPUT_PWM, OUTPUT);
      TCCR1B = TCCR1B & 0b11111000 | 0x01;
    }
    analogWrite(PIN_OUTPUT_PWM, int((PWM_MULTIPLIER * (volts / maxVolts))));
  }
  if (defined(ENABLE_DAC_OUT) && ENABLE_DAC_OUT == 1)
  {
    if (init) { 
      dac.begin(0x60);
    }
    dac.setVoltage(int(DAC_MULTIPLIER * (volts / maxVolts)), false);
  }
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
      setVoltage(0.1);
    } 
    else if (pulseTime >= 20100) 
    { // contaminated fuel supply
      setVoltage(4.8);
    }
    else if ((pulseTime <= 6400) && (pulseTime >= 1)) 
    { // high water content in fuel
      setVoltage(4.9);
    }
    if (countTick < 2) 
    {
      countTick++; 
    }
		return;
	}

  int eContent = frequency - (50 - eContentAdder);
  return clamp(eContent, 0, 100);
}

float setVoltageFromEthanol (int ethanol)
{
  float desiredVoltage = mapf(ethanol, 0, 100, voltageMin, voltageMax);
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

  eContent = getEthanol(pulseTime);
  setVoltageFromEthanol(eContent);

  tempC = getTempC(highTime, lowTime);
	tempF = cToF(tempC);

  if (ENABLE_SERIAL == 1) 
  {
    Serial << "Ethanol: " << eContent << "\%  •  Fuel Temp: " << tempC << "°C (" << tempF << "°F)" << endl;
    Serial.println();
  }
  delay(refreshDelay);
	countTick = 0;
}
