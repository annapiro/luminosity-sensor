/*
  Luminosity measurement
  Adafruit TSL2591 Digital Light Sensor
  Maximum Lux: 88K 

  Temperature measurement
  Adafruit MAX31865 RTD PT100 Amplifier

*/

#include <Adafruit_MAX31865.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_TSL2591.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>

Adafruit_TSL2591 tsl = Adafruit_TSL2591(2591); // pass in a number for the sensor identifier (for your use later)
LiquidCrystal_I2C lcd(0x27,16,2);  // set the LCD address to 0x27 for a 16 chars and 2 line display
Adafruit_MAX31865 thermo = Adafruit_MAX31865(10); // using hardware SPI, so just passing in the CS pin

// The value of the Rref resistor for PT100
#define RREF      430.0
// The 'nominal' 0-degrees-C resistance of the sensor
#define RNOMINAL  100.0

bool logging = false;  // flag to indicate logging state
unsigned long logStartTime = 0;  // logging start time tracker
const long LOG_DURATION = 10000;  // logging duration in milliseconds

// fitted coefficients for converting raw counts to irradiance
const float A = 0.04560855009496846;
const float B = 1.0538497422122406;

// error margin due to temperature, based on empirical observations
const float ERROR_MARGIN = 0.05;

// minimum CH0 value that the sensor was calibrated with
const uint16_t CALIBRATION_THRESHOLD = 1500;

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

// convert CH0 raw counts to irradiance (W/m2)
float countsToIrradiance(uint16_t counts) {
  return A * pow(counts, B);
}

float calculateError(uint16_t counts, float irrad) {
  float countsError = counts * ERROR_MARGIN;
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
  lcd.print("             ");

  // round the irradiance to one decimal place and pad with spaces
  char irradRound[8];
  dtostrf(irrad, 1, 1, irradRound);
  char irradFormat[8];
  snprintf(irradFormat, 7, "%-6s", irradRound);
  lcd.setCursor(6, 0);
  lcd.print(irradFormat);

  lcd.setCursor(3, 1);
  lcd.print("+-");
  
  // choose correct field width to align the numbers
  int minWidth;  
  if (irrad > 9999) {
    minWidth = 7;
  } else if (irrad >= 1000) {
    minWidth = 6;
  } else if (irrad >= 100) {
    minWidth = 5;
  } else if (irrad >= 10) {
    minWidth = 4;
  } else {
    minWidth = 3;
  }

  if (errorMargin == -1) {
    lcd.print(" ???");
  } else {
    // create a string that will define formatting of the error number based on the above field width
    char formatString[5];
    snprintf(formatString, 5, "%%%ds", minWidth);
    
    // round the error margin to one decimal place and pad with spaces
    char errorRound[8];
    dtostrf(errorMargin, 1, 1, errorRound);
    char errorFormat[8];
    snprintf(errorFormat, 7, formatString, errorRound);
    lcd.setCursor(6, 1);
    lcd.print(errorFormat);
  }
  
  // round and pad the temperature
  // note: the temperature is rounded to one decimal place, but with current screen space constraints it is printed as integer
  char tempRound[5];
  dtostrf(temp, 1, 1, tempRound);
  char tempFormat[5];
  snprintf(tempFormat, 3, "%-3s", tempRound);
  lcd.setCursor(13, 1);
  lcd.print(tempFormat);
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
  // calculate possible error due to temperature
  float error = calculateError(ch0, irrad); 
  float temp = thermo.temperature(RNOMINAL, RREF);

  thermoCheckFaults();
  // only display the error if the measurement exceeds the threshold as the sensor wasn't calibrated in low-light conditions
  updateDisplay(irrad, ch0 >= CALIBRATION_THRESHOLD ? error : -1, temp);

  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    if (command.equalsIgnoreCase("log")) {
      logging = true;
      logStartTime = millis();
      Serial.println("ArduinoTime,CH0,CH1,Lux,Irradiance[W/m2],Error[W/m2],Temperature[C]");
    }
  }

  // print data in CSV format
  if (logging) {
    Serial.print(millis()); Serial.print(","); // timestamp
    Serial.print(ch0); Serial.print(","); // channel 0 (full spectrum)
    Serial.print(ch1); Serial.print(","); // channel 1 (IR)
    Serial.print(lux); Serial.print(",");
    Serial.print(irrad); Serial.print(",");
    Serial.print(error); Serial.print(",");
    Serial.println(temp);

    // stop logging after specified time
    if (millis() - logStartTime >= LOG_DURATION) {
      logging = false;
      Serial.println("Logging stopped.");
      Serial.println();
    }
  }

  delay(500);
}
