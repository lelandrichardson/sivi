#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include "Adafruit_Sensor.h"
#include "Adafruit_TSL2591.h"
#include <Adafruit_BMP280.h>
#include "Adafruit_SHT31.h"
#include "Adafruit_AMG88xx.h"


#define MICROS_PER_MS 1000
#define MICROS_PER_SEC 1000000
#define MILLIS_PER_SEC 1000
#define DEBOUNCE_MICROS 300 * MICROS_PER_MS
#define SLEEP_ENABLED true

// TODO:
// - wifi tcp/http connection, response?
// - serial ascom interface?
// - rg11
// - OLED display, sleep/wake

// TwoWire i2c = TwoWire(1);


/* TSL2591 Digital Light Sensor */
/* Dynamic Range: 600M:1 */
/* Maximum Lux: 88K */
Adafruit_TSL2591 tsl = Adafruit_TSL2591(2591); // pass in a number for the sensor identifier (for your use later)
uint16_t luminosity = 0;

void setupLightSensor() {
  if (!tsl.begin(&Wire, 0x29)) {
    Serial.println("No TSL2591 sensor found...");
    // Heltec.display -> clear();
    // Heltec.display -> drawString(0, 0, "No TSL2591 sensor found...");
    // Heltec.display -> display();
    // TODO: error handling
    while (1) delay(10);
  }
  // You can change the gain on the fly, to adapt to brighter/dimmer light situations
  //tsl.setGain(TSL2591_GAIN_LOW);    // 1x gain (bright light)
  tsl.setGain(TSL2591_GAIN_MED);      // 25x gain
  //tsl.setGain(TSL2591_GAIN_HIGH);   // 428x gain
  
  // Changing the integration time gives you a longer time over which to sense light
  // longer timelines are slower, but are good in very low light situtations!
  // tsl.setTiming(TSL2591_INTEGRATIONTIME_100MS);  // shortest integration time (bright light)
  // tsl.setTiming(TSL2591_INTEGRATIONTIME_200MS);
  tsl.setTiming(TSL2591_INTEGRATIONTIME_300MS);
  // tsl.setTiming(TSL2591_INTEGRATIONTIME_400MS);
  // tsl.setTiming(TSL2591_INTEGRATIONTIME_500MS);
  // tsl.setTiming(TSL2591_INTEGRATIONTIME_600MS);  // longest integration time (dim light)
}

void loopLightSensor() {
  // Simple data read example. Just read the infrared, fullspecrtrum diode 
  // or 'visible' (difference between the two) channels.
  // This can take 100-600 milliseconds! Uncomment whichever of the following you want to read
  luminosity = tsl.getLuminosity(TSL2591_VISIBLE);
  //luminosity = tsl.getLuminosity(TSL2591_FULLSPECTRUM);
  //luminosity = tsl.getLuminosity(TSL2591_INFRARED);
}


















/* BMP280 Barometric Pressure & Altitude Sensor */
Adafruit_BMP280 bmp;

// temperature, in celsius
float bmpTemperature = 0.0;

// barometric pressure, in Pascals
float bmpPressure = 0.0;

// altitude of sensor, in meters
float bmpAltitude = 0.0;

void setupBMP() {
  bmp = Adafruit_BMP280(&Wire);
  if (!bmp.begin()) {
    Serial.println("No BMP280 sensor found...");
    // Heltec.display -> clear();
    // Heltec.display -> drawString(0, 0, "No BMP280 sensor found...");
    // Heltec.display -> display();
    while (1) delay(10);
  }

  /* Default settings from datasheet. */
  bmp.setSampling(
    Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
    Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
    Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
    Adafruit_BMP280::FILTER_X16,      /* Filtering. */
    Adafruit_BMP280::STANDBY_MS_500   /* Standby time. */
  );
}

void loopBMP() {
    bmpTemperature = bmp.readTemperature();
    bmpPressure = bmp.readPressure();
    bmpAltitude = bmp.readAltitude(1013.25); /* Adjusted to local forecast! */
}




























/* SHT31-D Temperature & Humidity Sensor */
Adafruit_SHT31 sht31 = Adafruit_SHT31(&Wire);

// Temperature, in Celsius
float shtTemperature = 0.0;

// Humidity, in percent
float shtHumidity = 0.0;

void setupTempSensor() {
  if (!sht31.begin(0x44)) {   // Set to 0x45 for alternate i2c addr
    Serial.println("No SHT31 sensor found...");
    // Heltec.display -> clear();
    // Heltec.display -> drawString(0, 0, "No SHT31 sensor found...");
    // Heltec.display -> display();
    while (1) delay(10);
  }
  sht31.heater(false);
}

void loopTempSensor() {
  float t = sht31.readTemperature();
  float h = sht31.readHumidity();

  if (!isnan(t)) {  // check if 'is not a number'
    shtTemperature = t;
  }
  
  if (!isnan(h)) {  // check if 'is not a number'
    shtHumidity = h;
  }
}












/* AMG8833 IR Thermal Camera */
Adafruit_AMG88xx amg;
float amgPixels[AMG88xx_PIXEL_ARRAY_SIZE]; // buffer for full frame of temperatures

void setupCloudSensor() {
  if (!amg.begin()) {
    Serial.println("No AMG8833 sensor found...");
    while (1) delay(10);
  }
}

void loopCloudSensor() {
  amg.readPixels(amgPixels);
}







void loopSerial() {
  Serial.print("Temp: ");
  Serial.print(shtTemperature);
  Serial.println(" C");

  Serial.print("Pressure: ");
  Serial.print(bmpPressure);
  Serial.println(" kPa");

  Serial.print("Humidity: ");
  Serial.print(shtHumidity);
  Serial.println(" %");

  Serial.print("Altitude: ");
  Serial.print(bmpAltitude);
  Serial.println(" m");

  Serial.print("Clouds: ");
  Serial.print(123);
  Serial.println(" %");

  Serial.print("[");
  for(int i=1; i<=AMG88xx_PIXEL_ARRAY_SIZE; i++){
    Serial.print(amgPixels[i-1]);
    Serial.print(", ");
    if( i%8 == 0 ) Serial.println();
  }
  Serial.println("]");
  Serial.println();
}



















void setup() {
  Serial.begin(115200);
  while (!Serial); // Waiting for Serial Monitor
  Wire.begin();
  delay(1000);
  setupLightSensor();
  setupBMP();
  setupTempSensor();
  setupCloudSensor();
  delay(1000);
}

void loop() {
  loopLightSensor();
  loopBMP();
  loopTempSensor();
  loopCloudSensor();
  loopSerial();
  delay(5000);
}

