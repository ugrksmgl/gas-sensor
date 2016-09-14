#include <PimaticProbe.h>
#include <math.h>

PimaticProbe probe = PimaticProbe(0, 1011);
float backoff_slope = 1.0/20;

// TODO: Update to 60000 before deployment.
int backoff_mult = 6000.0;

int gas_backoff_half = 100;
int air_backoff_half = 60;
int temp_backoff_half = 310;

// Computed using
// Reading -> Temp in K      
// 269 -> 273        
// 283 -> 295        
// 290 -> 308        
int temp_calibration_slope = 1.653061;
int temp_offset = -171.9592;

unsigned long previousMillis = 0;

void setup() {
  // put your setup code here, to run once:
  // Transmitter
  pinMode(0, OUTPUT);

  // Gas Sensor MQ-2
  pinMode(4, INPUT);

  // Air Quality Sensor MQ-5
  pinMode(3, INPUT);
}

float sigmoid(float value, float half, float slope) {
  return 1.0 - (1.0 / (1 + exp(-slope * (value - half))));
}

int readTemp() {
  unsigned long admux_store = ADMUX;

   //ADC Multiplexer Selection Register
  ADMUX = 0;
  ADMUX |= (0 << REFS2) | (1 << REFS1) | (1 << REFS0);  // Internal 1.1V Voltage Reference
  ADMUX |= (1 << MUX3) | (1 << MUX2) | (1 << MUX1) | (1 << MUX0);  // Temperature Sensor - 1111

  //ADC Control and Status Register A 
  ADCSRA = 0;
  ADCSRA |= (1 << ADEN);  //Enable the ADC
  ADCSRA |= (1 << ADPS2);  //ADC Prescaler - 16 (16MHz -> 1MHz)

  ADCSRA |= (1 << ADSC);  //Start temperature conversion
  while (bit_is_set(ADCSRA, ADSC));  //Wait for conversion to finish
  byte low  = ADCL;
  byte high = ADCH;
  int temperature = (high << 8) | low;  // Result is in kelvin
  ADMUX = admux_store;
  return temp_calibration_slope * temperature + temp_offset;
}

void send(int temp, int gas, int air) {
  probe.transmit(true, gas, 1, 7);
  probe.transmit(true, temp, 2, 7);
  probe.transmit(true, air, 3, 7);
}

void loop() {
  int gas = analogRead(2);
  int air = analogRead(3);
  int temp = readTemp();

  double gas_backoff = sigmoid(gas, gas_backoff_half, backoff_slope);
  double air_backoff = sigmoid(gas, air_backoff_half, backoff_slope);
  double temp_backoff = sigmoid(gas, temp_backoff_half, backoff_slope);

  unsigned long backoff_msecs = backoff_mult * min(gas_backoff, min(air_backoff, temp_backoff));
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= backoff_msecs) {
    previousMillis = currentMillis;
    send(temp, gas, air);
    // TODO: Check if necessary.
    delay(1000);
  }
}
