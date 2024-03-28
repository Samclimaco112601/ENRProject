
// Enable debug prints to serial monitor
#define MY_DEBUG

// Use a bit lower baudrate for serial prints on ESP8266 than default in MyConfig.h
#define MY_BAUD_RATE 9600

// Enables and select radio type (if attached)
#define MY_RADIO_RF24
#define MY_RF24_CE_PIN 2  // 2 = D4 on ESP8266


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
Adafruit_BME680 bme;  // I2C

//create mysensors varibles, send as V_custom not as a JSON
MyMessage temperatureMSG(TEMP_CHILD_ID, V_TEMP);
MyMessage humidityMSG(HUM_CHILD_ID, V_HUM);
MyMessage pressureMSG(BARO_CHILD_ID, V_PRESSURE);
MyMessage RainRateMSG(RAIN_LOG_CHILD_ID, V_RAINRATE);
MyMessage RainMSG(RAIN_LOG_CHILD_ID, V_RAIN);

byte sensorInterrupt = 0;
byte sensorPin = 16;  // pin D3

// The hall-effect flow sensor outputs approximately 4.5 pulses per second per
// litre/minute of flow.
float calibrationFactor = 4.5;

volatile byte pulseCount;

float flowRate;
unsigned int flowMilliLitres;
unsigned long totalMilliLitres;

unsigned long oldTime;


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

  //Setup for the rain gauge
  pinMode(sensorPin, INPUT);
  digitalWrite(sensorPin, HIGH);

  pulseCount = 0;
  flowRate = 0.0;
  flowMilliLitres = 0;
  totalMilliLitres = 0;
  oldTime = 0;

  // The Hall-effect sensor is connected to pin 2 which uses interrupt 0.
  // Configured to trigger on a FALLING state change (transition from HIGH
  // state to LOW state)
  attachInterrupt(sensorInterrupt, pulseCounter, FALLING);
}

void presentation() {
  sendSketchInfo("BME680 Sensor", "2.3.2");

  present(TEMP_CHILD_ID, S_TEMP);
  present(HUM_CHILD_ID, S_HUM);
  present(BARO_CHILD_ID, S_BARO);
}


void loop() {
  // BME680 loop
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


  //Rain Gauge Loop
  if ((millis() - oldTime) > 1000) {  // Only process counters once per second

    // Disable the interrupt while calculating flow rate and sending the value to
    // the host
    detachInterrupt(sensorInterrupt);

    // Because this loop may not complete in exactly 1 second intervals we calculate
    // the number of milliseconds that have passed since the last execution and use
    // that to scale the output. We also apply the calibrationFactor to scale the output
    // based on the number of pulses per second per units of measure (litres/minute in
    // this case) coming from the sensor.
    flowRate = ((1000.0 / (millis() - oldTime)) * pulseCount) / calibrationFactor;

    // Note the time this processing pass was executed. Note that because we've
    // disabled interrupts the millis() function won't actually be incrementing right
    // at this point, but it will still return the value it was set to just before
    // interrupts went away.
    oldTime = millis();

    // Divide the flow rate in litres/minute by 60 to determine how many litres have
    // passed through the sensor in this 1 second interval, then multiply by 1000 to
    // convert to millilitres.
    flowMilliLitres = (flowRate / 60) * 1000;

    // Add the millilitres passed in this second to the cumulative total
    totalMilliLitres += flowMilliLitres;

    unsigned int frac;

    // Print the flow rate for this second in litres / minute
    Serial.print("Flow rate: ");
    Serial.print(int(flowRate));  // Print the integer part of the variable
    send(RainRateMSG.set(int(flowRate), 1)); //sends to mycontroller
    Serial.print("L/min");
    Serial.print("\t");  // Print tab space

    // Print the cumulative total of litres flowed since starting
    Serial.print("Output Liquid Quantity: ");
    Serial.print(totalMilliLitres);
    send(RainRateMSG.set(totalMilliLitres, 1)); //sends to mycontroller
    Serial.println("mL");

    // Reset the pulse counter so we can start incrementing again
    pulseCount = 0;

    // Enable the interrupt again now that we've finished sending output
    attachInterrupt(sensorInterrupt, pulseCounter, FALLING);

    Serial.println();
  }

  delay(2000);
}


/*
Interrupt Service Routine
 */
ICACHE_RAM_ATTR void pulseCounter() {
  // Increment the pulse counter
  pulseCount++;
}
