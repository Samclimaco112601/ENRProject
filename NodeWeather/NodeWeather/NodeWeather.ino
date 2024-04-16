//Code including rain gauge sensor and radio communication

// Enable debug prints to serial monitor
#define MY_DEBUG

// Use a bit lower baudrate for serial prints on ESP8266 than default in MyConfig.h
#define MY_BAUD_RATE 9600

// Enables and select radio type (if attached)
#define MY_RADIO_RF24
#define MY_RF24_CE_PIN 2 // D4 pin

// Set LOW transmit power level as default, if you have an amplified NRF-module and
// power your radio separately with a good regulator you can turn up PA level.
#define MY_RF24_PA_LEVEL RF24_PA_HIGH

// Enabled repeater feature for this node
#define MY_REPEATER_FEATURE

#define TEMP_CHILD_ID 0
#define HUM_CHILD_ID 1
#define BARO_CHILD_ID 2
#define CHILD_ID_DUST_PM3 3    //.3 um
#define CHILD_ID_DUST_PM5 4    //.5 um
#define CHILD_ID_DUST_PM10 5   //1.0 um
#define CHILD_ID_DUST_PM25 6   //2.5 um
#define CHILD_ID_DUST_PM50 7   //5.0 um
#define CHILD_ID_DUST_PM100 8  //10.0 um

// Libraries
#include <MySensors.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BME680.h"
#include "Adafruit_PM25AQI.h"
#include <RH_NRF24.h>
#include <EEPROM.h>
#include <ESP8266WiFi.h>

//create BME sensor
Adafruit_BME680 bme;  // I2C

//create aerosol particle sensor
Adafruit_PM25AQI aqi = Adafruit_PM25AQI();

//create mysensors varibles, send as V_custom not as a JSON
MyMessage temperatureMSG(TEMP_CHILD_ID, V_TEMP);
MyMessage humidityMSG(HUM_CHILD_ID, V_HUM);
MyMessage pressureMSG(BARO_CHILD_ID, V_PRESSURE);
MyMessage msgDust3(CHILD_ID_DUST_PM3, V_LEVEL);
MyMessage msgDust3b(CHILD_ID_DUST_PM3, V_UNIT_PREFIX);
MyMessage msgDust5(CHILD_ID_DUST_PM5, V_LEVEL);
MyMessage msgDust5b(CHILD_ID_DUST_PM5, V_UNIT_PREFIX);
MyMessage msgDust10(CHILD_ID_DUST_PM10, V_LEVEL);
MyMessage msgDust10b(CHILD_ID_DUST_PM10, V_UNIT_PREFIX);
MyMessage msgDust25(CHILD_ID_DUST_PM25, V_LEVEL);
MyMessage msgDust25b(CHILD_ID_DUST_PM25, V_UNIT_PREFIX);
MyMessage msgDust50(CHILD_ID_DUST_PM50, V_LEVEL);
MyMessage msgDust50b(CHILD_ID_DUST_PM50, V_UNIT_PREFIX);
MyMessage msgDust100(CHILD_ID_DUST_PM100, V_LEVEL);
MyMessage msgDust100b(CHILD_ID_DUST_PM100, V_UNIT_PREFIX);


void setup() {
  Wire.begin();
  Serial.begin(9600);
  if (!bme.begin()) {
    Serial.println("Could not find a valid BME680 sensor, check wiring!");
    while (1)
      ;
  } else {
    Serial.print("BME found");
  }

  // Set up oversampling and filter initialization
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150);  // 320*C for 150 ms

  // Setup for the air particle sensor
  if (!aqi.begin_I2C()) {  // connect to the sensor over I2C
    Serial.println("Could not find PM 2.5 sensor!");
    delay(1000);
    while (1)
      ;
  } else {
    Serial.print("PM25 found");
  }
}

void presentation() {
  sendSketchInfo("Aerosol and Weather Sensor Combined", "2.3.2");

  present(CHILD_ID_DUST_PM3, S_DUST);
  send(msgDust3b.set("um/.1L"));
  present(CHILD_ID_DUST_PM5, S_DUST);
  send(msgDust5b.set("um/.1L"));
  present(CHILD_ID_DUST_PM10, S_DUST);
  send(msgDust10b.set("um/.1L"));
  present(CHILD_ID_DUST_PM25, S_DUST);
  send(msgDust25b.set("um/.1L"));
  present(CHILD_ID_DUST_PM50, S_DUST);
  send(msgDust50b.set("um/.1L"));
  present(CHILD_ID_DUST_PM100, S_DUST);
  send(msgDust100b.set("um/.1L"));
  present(TEMP_CHILD_ID, S_TEMP);
  present(HUM_CHILD_ID, S_HUM);
  present(BARO_CHILD_ID, S_BARO);
}


void loop() {
  // BME680
  bme.performReading();

  Serial.print("Temperature = ");
  Serial.print(bme.temperature);
  Serial.println(" *F");
  send(temperatureMSG.set((bme.temperature), 1));
  Serial.print("Pressure = ");
  Serial.print(float(bme.pressure) / 3386.0);
  Serial.println(" in. Hg");
  send(pressureMSG.set((float(bme.pressure) / 3386.0), 1));
  Serial.print("Humidity = ");
  Serial.print(bme.humidity);
  Serial.println(" %");
  send(humidityMSG.set(bme.humidity, 1));
  Serial.println();

  //PM25 sensor
  PM25_AQI_Data data;
  if (!aqi.read(&data)) {
    Serial.println("Could not read from AQI");
    delay(500);  // try again in a bit!
    return;
  }
  Serial.println("AQI reading success");


  Serial.println(F("---------------------------------------"));
  Serial.print(F("Particles > 0.3um / 0.1L air:"));
  Serial.println(data.particles_03um);
  send(msgDust3.set(float(data.particles_03um), 1));
  Serial.print(F("Particles > 0.5um / 0.1L air:"));
  Serial.println(data.particles_05um);
  send(msgDust5.set(float(data.particles_05um), 1));
  Serial.print(F("Particles > 1.0um / 0.1L air:"));
  Serial.println(data.particles_10um);
  send(msgDust10.set(float(data.particles_10um), 1));
  Serial.print(F("Particles > 2.5um / 0.1L air:"));
  Serial.println(data.particles_25um);
  send(msgDust25.set(float(data.particles_25um), 1));
  Serial.print(F("Particles > 5.0um / 0.1L air:"));
  Serial.println(data.particles_50um);
  send(msgDust50.set(float(data.particles_50um), 1));
  Serial.print(F("Particles > 10 um / 0.1L air:"));
  Serial.println(data.particles_100um);
  send(msgDust100.set(float(data.particles_100um), 1));
  Serial.println(F("---------------------------------------"));

  delay(5000);
}