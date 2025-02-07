/*
  Luminosity measurement
  Adafruit TSL2591 Digital Light Sensor
  Maximum Lux: 88K 

  Temperature measurement
  Adafruit MAX31865 RTD PT100 Amplifier

*/

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_TSL2591.h"
#include <Adafruit_MAX31865.h>
#include <math.h>

Adafruit_TSL2591 tsl = Adafruit_TSL2591(2591); // pass in a number for the sensor identifier (for your use later)
LiquidCrystal_I2C lcd(0x27,16,2);  // set the LCD address to 0x27 for a 16 chars and 2 line display
Adafruit_MAX31865 thermo = Adafruit_MAX31865(10); // using hardware SPI, so just passing in the CS pin

// The value of the Rref resistor for PT100
#define RREF      430.0
// The 'nominal' 0-degrees-C resistance of the sensor
#define RNOMINAL  100.0

bool logging = false;  // flag to indicate logging state
unsigned long logStartTime = 0;  // variable to track logging timme
const long logDuration = 20000;  // logging duration in milliseconds

// fitted coefficients for converting raw counts to irradiance
const float a = 0.032823239664847834;
const float b = 1.039138018146907;

// error margin due to temperature, based on empirical observations
const float errorMargin = 0.05;

// From TSL2591 example sketches - configure gain and integration time
void setupLightSensor(void) {
  if (tsl.begin()) {
    Serial.println(F("Found a TSL2591 sensor"));
  } else {
    Serial.println(F("No sensor found!"));
    while (1);
  }

  tsl.setGain(TSL2591_GAIN_LOW);    // 1x gain (bright light)
  
  tsl.setTiming(TSL2591_INTEGRATIONTIME_100MS);  // shortest integration time (bright light)

  // display gain and integration time for reference
  Serial.println(F("------------------------------------"));
  Serial.print  (F("Gain:         "));
  tsl2591Gain_t gain = tsl.getGain();
  switch(gain) {
    case TSL2591_GAIN_LOW:
      Serial.println(F("1x (Low)"));
      break;
    case TSL2591_GAIN_MED:
      Serial.println(F("25x (Medium)"));
      break;
    case TSL2591_GAIN_HIGH:
      Serial.println(F("428x (High)"));
      break;
    case TSL2591_GAIN_MAX:
      Serial.println(F("9876x (Max)"));
      break;
  }
  Serial.print  (F("Timing:       "));
  Serial.print((tsl.getTiming() + 1) * 100, DEC); 
  Serial.println(F(" ms"));
  Serial.println(F("------------------------------------"));
  Serial.println(F(""));
}

void setupThermoSensor(void) {
  thermo.begin(MAX31865_2WIRE);
}

// print a summary of the light sensor readings to the serial monitor
void printLightSummary(uint16_t full, uint16_t ir) {
  Serial.print(F("[ ")); Serial.print(millis()); Serial.print(F(" ms ] "));
  Serial.print(F("IR: ")); Serial.print(ir);  Serial.print(F("  "));
  Serial.print(F("Full: ")); Serial.print(full); Serial.print(F("  "));
  Serial.print(F("Visible: ")); Serial.print(full - ir); Serial.print(F("  "));
  Serial.print(F("Lux: ")); Serial.println(tsl.calculateLux(full, ir), 6);
}

// convert CH0 raw counts to irradiance (W/m2)
float countsToIrradiance(uint16_t counts) {
  return a * pow(counts, b);
}

float calculateError(uint16_t counts, float irrad) {
  float countsError = counts * errorMargin;
  float lowerBound = countsToIrradiance(counts - countsError);
  float upperBound = countsToIrradiance(counts + countsError);
  float diffDown = irrad - lowerBound;
  float diffUp = upperBound - irrad;
  return (diffDown > diffUp) ? diffDown : diffUp;
}

// From MAX31865 example sketches - check and print any faults
void thermoCheckFaults() {
  uint8_t fault = thermo.readFault();
  if (fault) {
    Serial.print("Fault 0x"); Serial.println(fault, HEX);
    if (fault & MAX31865_FAULT_HIGHTHRESH) {
      Serial.println("RTD High Threshold"); 
    }
    if (fault & MAX31865_FAULT_LOWTHRESH) {
      Serial.println("RTD Low Threshold"); 
    }
    if (fault & MAX31865_FAULT_REFINLOW) {
      Serial.println("REFIN- > 0.85 x Bias"); 
    }
    if (fault & MAX31865_FAULT_REFINHIGH) {
      Serial.println("REFIN- < 0.85 x Bias - FORCE- open"); 
    }
    if (fault & MAX31865_FAULT_RTDINLOW) {
      Serial.println("RTDIN- < 0.85 x Bias - FORCE- open"); 
    }
    if (fault & MAX31865_FAULT_OVUV) {
      Serial.println("Under/Over voltage"); 
    }
    thermo.clearFault();
  }
}

void updateDisplay(float irrad, float errorMargin, float temp) {
  // clear the display
  lcd.setCursor(5, 0);
  lcd.print("        ");
  lcd.setCursor(0, 1);
  lcd.print("            ");
  
  // print new values
  lcd.setCursor(6, 0);
  lcd.print(irrad);

  int padding;  
  if (irrad >= 2500) {
    padding = 1;
  } else if (irrad >= 1000) {
    padding = 2;
  } else if (irrad >= 250) {
    padding = 1;
  } else if (irrad >= 100) {
    padding = 2;
  } else if (irrad >= 10) {
    padding = 1;
  } else {
    padding = 0;
  }

  lcd.setCursor(3 + padding, 1);
  lcd.print("+-");
  lcd.setCursor(6 + padding, 1);
  lcd.print(errorMargin);
  lcd.setCursor(13, 1);
  lcd.print((int) temp);
}

void setup(void) {
  Serial.begin(115200); 
    
  setupLightSensor();
  setupThermoSensor();
  
  // initialize the display
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("W/m2:");
  lcd.setCursor(13, 0);
  lcd.print("C:");
}

void loop(void) { 
  uint16_t ch0 = tsl.getLuminosity(TSL2591_FULLSPECTRUM);
  uint16_t ch1 = tsl.getLuminosity(TSL2591_INFRARED);
  float lux = tsl.calculateLux(ch0, ch1);
  float irrad = countsToIrradiance(ch0);
  float error = calculateError(ch0, irrad);  // calculate possible error due to temperature
  float temp = thermo.temperature(RNOMINAL, RREF);
  
  thermoCheckFaults();
  updateDisplay(irrad, error, temp);
  // printLightSummary(ch0, ch1);

  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    if (command.equalsIgnoreCase("log")) {
      logging = true;
      logStartTime = millis();
      Serial.println("Logging started...");
      Serial.println("Arduino time,CH0,CH1,Lux,Irradiance[W/m2],Error[W/m2],Temperature[C]");
    }
  }

  if (logging) {
    // print data in CSV format
    Serial.print(millis()); // timestamp
    Serial.print(",");
    Serial.print(ch0); // channel 0 - full spectrum
    Serial.print(",");
    Serial.print(ch1); // channel 1 - IR
    Serial.print(",");
    Serial.print(lux);
    Serial.print(",");
    Serial.print(irrad);
    Serial.print(",");
    Serial.print(error);
    Serial.print(",");
    Serial.println(temp);

    // stop logging after specified time
    if (millis() - logStartTime >= logDuration) {
      logging = false;
      Serial.println("Logging stopped.");
      Serial.println();
    }
  }

  delay(500);
}
