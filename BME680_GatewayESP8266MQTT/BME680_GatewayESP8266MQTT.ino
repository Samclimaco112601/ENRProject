
// Enable debug prints to serial monitor
#define MY_DEBUG

// Use a bit lower baudrate for serial prints on ESP8266 than default in MyConfig.h
#define MY_BAUD_RATE 9600

// Enables and select radio type (if attached)
#define MY_RADIO_RF24
#define MY_RF24_CE_PIN 2 // 2 = D4 on ESP8266


#define MY_GATEWAY_MQTT_CLIENT
#define MY_GATEWAY_ESP8266

// Set this node's subscribe and publish topic prefix
#define MY_MQTT_PUBLISH_TOPIC_PREFIX "mygateway6-out"
#define MY_MQTT_SUBSCRIBE_TOPIC_PREFIX "mygateway6-in"

// Set MQTT client id
#define MY_MQTT_CLIENT_ID "mysensors-6"

// Enable these if your MQTT broker requires username/password
//#define MY_MQTT_USER "username"
//#define MY_MQTT_PASSWORD "password"

// Set WIFI SSID and password
//#define MY_WIFI_SSID "SPEEDY CASA 1"
//#define MY_WIFI_PASSWORD "aguilaincaecuador"

#define MY_WIFI_SSID "NETGEAR13"
#define MY_WIFI_PASSWORD "smilingcoconut656"

// Set the hostname for the WiFi Client. This is the hostname
// passed to the DHCP server if not static.
#define MY_HOSTNAME "ESP8266_MQTT_GW"

// Enable MY_IP_ADDRESS here if you want a static ip address (no DHCP)
//#define MY_IP_ADDRESS 192,168,178,87

// If using static ip you can define Gateway and Subnet address as well
//#define MY_IP_GATEWAY_ADDRESS 192,168,178,1
//#define MY_IP_SUBNET_ADDRESS 255,255,255,0

// MQTT broker ip address.
//#define MY_CONTROLLER_IP_ADDRESS 192, 168, 1, 6

//MQTT broker if using URL instead of ip address.
//#define MY_CONTROLLER_URL_ADDRESS "ecuador.mynetgear.com"
#define MY_CONTROLLER_URL_ADDRESS "wint0178.mynetgear.com"

// The MQTT broker port to to open
#define MY_PORT 1883

#define TEMP_CHILD_ID 0
#define HUM_CHILD_ID 1
#define BARO_CHILD_ID 2
#define RAIN_LOG_CHILD_ID 3

#include <MySensors.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BME680.h"
#include <RH_NRF24.h>
#include <EEPROM.h>
#include <ESP8266WiFi.h>

//create BME sensor
Adafruit_BME680 bme; // I2C

//create mysensors varibles, send as V_custom not as a JSON 
MyMessage temperatureMSG(TEMP_CHILD_ID, V_TEMP);
MyMessage humidityMSG(HUM_CHILD_ID, V_HUM);
MyMessage pressureMSG(BARO_CHILD_ID, V_PRESSURE);
MyMessage RainRateMSG(RAIN_LOG_CHILD_ID, V_RAINRATE);
MyMessage RainMSG(RAIN_LOG_CHILD_ID, V_RAIN);

void setup()
{
 Wire.begin();
 Serial.begin(9600); 
 if (!bme.begin()) {
    Serial.println("Could not find a valid BME680 sensor, check wiring!");
    while (1);
    }else{
      Serial.print("BME found");
      }
  
  // Set up oversampling and filter initialization
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150); // 320*C for 150 ms
}

void presentation()
{
sendSketchInfo("BME680 Sensor", "2.3.2");

present(TEMP_CHILD_ID, S_TEMP);
present(HUM_CHILD_ID, S_HUM);
present(BARO_CHILD_ID, S_BARO);
}


void loop()
{
  bme.performReading();

  Serial.print("Temperature = ");
  Serial.print(bme.temperature * 1.8 + 32);
  Serial.println(" *F");
  send(temperatureMSG.set((bme.temperature * 1.8 + 32), 1));
  Serial.print("Pressure = ");
  Serial.print(bme.pressure / 3386);
  Serial.println(" in. Hg");
  send(pressureMSG.set((bme.pressure / 3386), 1));
  Serial.print("Humidity = ");
  Serial.print(bme.humidity);
  Serial.println(" %");
  send(humidityMSG.set(bme.humidity, 1));
  Serial.println();
  


  delay(2000);
}