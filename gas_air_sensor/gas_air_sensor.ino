#include <PimaticProbe.h>
#include <math.h>

PimaticProbe probe = PimaticProbe(0, 1011);
float backoff_slope = 1.0/20;
int backoff_mult = 6000.0;
int backoff_half = 100;
int temp_calibration_slope = 1;
int temp_offset = 0;

unsigned long previousMillis = 0;

void setup() {
  // put your setup code here, to run once:
  //  Serial.begin(9600);
  pinMode(0, OUTPUT);
  pinMode(4, INPUT);
}

float sigmoid(float value, float half, float slope) {
  return 1.0 - (1.0 / (1 + exp(-slope * (value - half))));
}

int readTemp() {
  unsigned long admux_store = ADMUX;

   //ADC Multiplexer Selection Register
  ADMUX = 0;
  ADMUX |= (0 << REFS2);  //Internal 2.56V Voltage Reference with external capacitor on AREF pin
  ADMUX |= (1 << REFS1);  //Internal 2.56V Voltage Reference with external capacitor on AREF pin
  ADMUX |= (1 << REFS0);  //Internal 2.56V Voltage Reference with external capacitor on AREF pin
  ADMUX |= (1 << MUX3);  //Temperature Sensor - 100111
  ADMUX |= (1 << MUX2);  //Temperature Sensor - 100111
  ADMUX |= (1 << MUX1);  //Temperature Sensor - 100111
  ADMUX |= (1 << MUX0);  //Temperature Sensor - 100111

  //ADC Control and Status Register A 
  ADCSRA = 0;
  ADCSRA |= (1 << ADEN);  //Enable the ADC
  ADCSRA |= (1 << ADPS2);  //ADC Prescaler - 16 (16MHz -> 1MHz)

  ADCSRA |= (1 << ADSC);  //Start temperature conversion
  while (bit_is_set(ADCSRA, ADSC));  //Wait for conversion to finish
  byte low  = ADCL;
  byte high = ADCH;
  int temperature = (high << 8) | low;  //Result is in kelvin
  ADMUX = admux_store;
  return temp_calibration_slope * temperature + temp_offset;
}

void loop() {
  int val = analogRead(2);
  int temp = readTemp();
  unsigned long backoff_msecs = backoff_mult * sigmoid(val, backoff_half, backoff_slope);
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= backoff_msecs) {
    previousMillis = currentMillis;
    probe.transmit(true, temp, 2, 7);
    probe.transmit(true, val, 1, 7);
    delay(1000);
  }
}