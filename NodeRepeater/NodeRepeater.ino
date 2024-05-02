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

// Libraries
#include <MySensors.h>
#include <RH_NRF24.h>
#include <Adafruit_MPU6050.h>

#define RAIN_RATE_CHILD_ID 0
#define MOVEMENT_CHILD_ID 1

//create mysensors varibles, send as V_custom not as a JSON
MyMessage rainMSG(RAIN_RATE_CHILD_ID, V_RAINRATE);
MyMessage moveMSG(MOVEMENT_CHILD_ID, V_TRIPPED);

// Gyroscope setup
Adafruit_MPU6050 mpu;
float rotationDetection = .5; //In rad/second

//Pin and intterupt
//byte sensorPin = 5;
//byte sensorInterrupt = 2;

float calibrationFactor = 6.0;  // sensor outputs approximately 6.0 pulses per second per litre/minute of flow.
float sampleRate = 1000;        //miliiseconds
volatile byte pulseCount;
float flowRate;
unsigned int flowMilliLitres;
unsigned long totalMilliLitres;
unsigned long oldTime;
unsigned long lastSent;
unsigned long lastSentMovement;

void setup() {
  // Initialize a serial connection for reporting values to the host
  Serial.begin(9600);

  while (!Serial)
    delay(10);

  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
  }

  //pinMode(sensorPin, INPUT);
  //digitalWrite(sensorPin, HIGH);

  pulseCount = 0;
  flowRate = 5.0;
  flowMilliLitres = 0;
  totalMilliLitres = 0;
  oldTime = 0;
  lastSent = 0;
  lastSentMovement = 0;

  // The Hall-effect sensor is connected to pin 2 which uses interrupt 0.
  // Configured to trigger on a FALLING state change (transition from HIGH
  // state to LOW state)
  //attachInterrupt(sensorInterrupt, pulseCounter, FALLING);
}

void presentation() {
  sendSketchInfo("Rainfall+Movement Sensor", "2.3.2");
  present(RAIN_RATE_CHILD_ID, S_RAIN);
  present(MOVEMENT_CHILD_ID, S_BINARY);
}

void loop() {

//   if ((millis() - oldTime) > sampleRate)  // Only process counters once per second
//   {
//     // Disable the interrupt while calculating flow rate and sending the value to
//     // the host
//     detachInterrupt(sensorInterrupt);

//     // Because this loop may not complete in exactly 1 second intervals we calculate
//     // the number of milliseconds that have passed since the last execution and use
//     // that to scale the output. We also apply the calibrationFactor to scale the output
//     // based on the number of pulses per second per units of measure (litres/minute in
//     // this case) coming from the sensor.
//     flowRate = ((sampleRate / (millis() - oldTime)) * pulseCount) / calibrationFactor;

//     // Note the time this processing pass was executed. Note that because we've
//     // disabled interrupts the millis() function won't actually be incrementing right
//     // at this point, but it will still return the value it was set to just before
//     // interrupts went away.
//     oldTime = millis();

//     // Divide the flow rate in litres/minute by 60 to determine how many litres have
//     // passed through the sensor in this 1 second interval, then multiply by 1000 to
//     // convert to millilitres.
//     flowMilliLitres = (flowRate / 60) * 1000;

//     // Add the millilitres passed in this second to the cumulative total
//     totalMilliLitres += flowMilliLitres;

//     unsigned int frac;

//     // Print the flow rate for this second in litres / minute
//     Serial.print("Flow rate: ");
//     Serial.print(flowRate);  // Print the integer part of the variable
//     Serial.print("L/min");
//     Serial.print("\t");  // Print tab space

//     // Print the cumulative total of litres flowed since starting
//     Serial.print("Output Liquid Quantity: ");
//     Serial.print(totalMilliLitres);
//     Serial.println("mL");

//     // Reset the pulse counter so we can start incrementing again
//     pulseCount = 0;

//     // Enable the interrupt again now that we've finished sending output
//     attachInterrupt(sensorInterrupt, pulseCounter, FALLING);
//   }
  if (millis() - lastSent > 5000) {  //if time is greater than 5s
    lastSent = millis();
    send(rainMSG.set(flowRate, 2));
  }

  sensors_event_t accel, gyro, temp;
  mpu.getEvent(&accel, &gyro, &temp);

  /* Display the results (rotation is measured in rad/s) */
  Serial.print("\t\tGyro X: ");
  Serial.print(gyro.gyro.x);
  Serial.print(" \tY: ");
  Serial.print(gyro.gyro.y);
  Serial.print(" \tZ: ");
  Serial.print(gyro.gyro.z);
  Serial.println(" radians/s\n");

  /* Landlslide detection based on rotation*/
  float totalRotation = sqrt(gyro.gyro.x*gyro.gyro.x + gyro.gyro.y*gyro.gyro.y + gyro.gyro.z*gyro.gyro.z);

  if(totalRotation > rotationDetection){
    Serial.println("Movement Detected\n");
    send(moveMSG.set(true));
    moveMSG.send
    lastSentMovement = millis();
  } else if(millis() - lastSentMovement > 1000){ // if movement has not been detected for 20s
    moveMSG.set(false);
    lastSentMovement = millis();
  }
  
}

// /*
// Interrupt Service Routine
//  */
// void pulseCounter() {
//   // Increment the pulse counter
//   pulseCount++;
// }