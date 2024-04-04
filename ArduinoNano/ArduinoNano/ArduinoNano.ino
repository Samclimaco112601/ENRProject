//Code including rain gauge sensor and radio communication

#include <MySensors.h>
#include <RH_NRF24.h>

// Enable debug prints to serial monitor
#define MY_DEBUG

// Use a bit lower baudrate for serial prints on ESP8266 than default in MyConfig.h
#define MY_BAUD_RATE 9600

// Enables and select radio type (if attached)
#define MY_RADIO_RF24

// Set LOW transmit power level as default, if you have an amplified NRF-module and
// power your radio separately with a good regulator you can turn up PA level.
#define MY_RF24_PA_LEVEL RF24_PA_LOW

// Enable repeater functionality for this node
#define MY_REPEATER_FEATURE


// Set this node's subscribe and publish topic prefix
#define MY_MQTT_PUBLISH_TOPIC_PREFIX "mygateway7-out"
#define MY_MQTT_SUBSCRIBE_TOPIC_PREFIX "mygateway7-in"

// Set MQTT client id
#define MY_MQTT_CLIENT_ID "mysensors-7"

#define MY_WIFI_SSID "NETGEAR13"
#define MY_WIFI_PASSWORD "smilingcoconut656"

#define MY_CONTROLLER_URL_ADDRESS "wint0178.mynetgear.com"

// The MQTT broker port to to open
#define MY_PORT 1883

#define RAIN_RATE_CHILD_ID 0

//create mysensors varibles, send as V_custom not as a JSON
MyMessage rainMSG(RAIN_RATE_CHILD_ID, V_RAINRATE);

//Pin and intterupt
byte sensorPin = 2;
byte sensorInterrupt = 0;

float calibrationFactor = 6.0;  // sensor outputs approximately 6.0 pulses per second per litre/minute of flow.
float sampleRate = 1000;        //miliiseconds
volatile byte pulseCount;
float flowRate;
unsigned int flowMilliLitres;
unsigned long totalMilliLitres;
unsigned long oldTime;
unsigned long lastSent;

void setup() {
  // Initialize a serial connection for reporting values to the host
  Serial.begin(9600);

  pinMode(sensorPin, INPUT);
  digitalWrite(sensorPin, HIGH);

  pulseCount = 0;
  flowRate = 0.0;
  flowMilliLitres = 0;
  totalMilliLitres = 0;
  oldTime = 0;
  lastSent = 0;

  // The Hall-effect sensor is connected to pin 2 which uses interrupt 0.
  // Configured to trigger on a FALLING state change (transition from HIGH
  // state to LOW state)
  attachInterrupt(sensorInterrupt, pulseCounter, FALLING);
}

void presentation() {
  sendSketchInfo("Rainfall Sensor", "2.3.2");
  present(RAIN_RATE_CHILD_ID, S_RAIN);
}

void loop() {

  if ((millis() - oldTime) > sampleRate)  // Only process counters once per second
  {
    // Disable the interrupt while calculating flow rate and sending the value to
    // the host
    detachInterrupt(sensorInterrupt);

    // Because this loop may not complete in exactly 1 second intervals we calculate
    // the number of milliseconds that have passed since the last execution and use
    // that to scale the output. We also apply the calibrationFactor to scale the output
    // based on the number of pulses per second per units of measure (litres/minute in
    // this case) coming from the sensor.
    flowRate = ((sampleRate / (millis() - oldTime)) * pulseCount) / calibrationFactor;

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
    Serial.print(flowRate);  // Print the integer part of the variable
    Serial.print("L/min");
    Serial.print("\t");  // Print tab space

    // Print the cumulative total of litres flowed since starting
    Serial.print("Output Liquid Quantity: ");
    Serial.print(totalMilliLitres);
    Serial.println("mL");

    // Reset the pulse counter so we can start incrementing again
    pulseCount = 0;

    // Enable the interrupt again now that we've finished sending output
    attachInterrupt(sensorInterrupt, pulseCounter, FALLING);
  }
  if (millis() - lastSent > 2000) {  //if time is greater than 2s
    lastSent = millis();
    send(rainMSG.set(flowRate, 1));
  }
}

/*
Interrupt Service Routine
 */
void pulseCounter() {
  // Increment the pulse counter
  pulseCount++;
}