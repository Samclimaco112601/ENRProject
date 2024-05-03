//Code including rain gauge sensor, gyroscope, and radio communication

// Enable debug prints to serial monitor
#define MY_DEBUG

// Use a bit lower baudrate for serial prints on ESP8266 than default in MyConfig.h
#define MY_BAUD_RATE 9600

// Enables and select radio type (if attached)
#define MY_RADIO_RF24
#define MY_RF24_CE_PIN 2  // D4 pin

// Set LOW transmit power level as default, if you have an amplified NRF-module and
// power your radio separately with a good regulator you can turn up PA level.
#define MY_RF24_PA_LEVEL RF24_PA_HIGH

// Enabled repeater feature for this node
#define MY_REPEATER_FEATURE

// Libraries
#include <MySensors.h>
#include <Adafruit_MPU6050.h>

#define RAIN_CHILD_ID 0
#define RAIN_RATE_CHILD_ID 1
#define MOVEMENT_CHILD_ID 2

//create mysensors varibles, send as V_custom
MyMessage rainMSG(RAIN_CHILD_ID, V_RAIN);
MyMessage rainRateMSG(RAIN_RATE_CHILD_ID, V_RAINRATE);
MyMessage moveMSG(MOVEMENT_CHILD_ID, V_TRIPPED);

// Gyroscope attributes
Adafruit_MPU6050 mpu;
float rotationDetection = .5;  //In rad/second
unsigned long lastSentMovementTime = 0;

// Rain gauge attributes and intialization
#define rainGaugePin 0          // D3 pin for the ESP8266 controller
float calibrationFactor = 6.0;  // The flow sensor outputs approximately 6.0 pulses/second per L/min
float flowRate = 0;
volatile byte pulseCount = 0;
unsigned int flowMilliLitres = 0;
unsigned long totalMilliLitres = 0;
unsigned long lastRainRateTime = 0;
unsigned long lastDay = 0;

void setup() {
  // Initialize a serial connection for reporting values to the host
  Serial.begin(9600);

  while (!Serial)
    delay(10);

  while (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    delay(100);
  }

  //Rain Gauge setup and initialization
  pinMode(rainGaugePin, INPUT);
  digitalWrite(rainGaugePin, HIGH);

  pulseCount = 0;
  flowRate = 5.0;
  flowMilliLitres = 0;
  totalMilliLitres = 0;
  lastRainRateTime = 0;
  lastSentMovementTime = 0;

  // Configured to trigger on a FALLING state change
  attachInterrupt(rainGaugePin, pulseCounter, FALLING);
}


void presentation() {
  sendSketchInfo("Rainfall+Movement Sensor", "2.3.2");
  present(RAIN_RATE_CHILD_ID, S_RAIN);
  present(RAIN_RATE_CHILD_ID, S_RAIN);
  present(MOVEMENT_CHILD_ID, S_BINARY);
}


void loop() {
  if (abs(millis() - lastRainRateTime) > 1000) {  // Only process counters once per second

    // Disable the interrupt while calculating flow rate
    detachInterrupt(rainGaugePin);

    // We also apply the calibrationFactor to scale the output
    // based on the number of pulses per second per units of measure (litres/minute in
    // this case) coming from the sensor.
    flowRate = ((1000.0 / (millis() - lastRainRateTime)) * pulseCount) / calibrationFactor;

    lastRainRateTime = millis();

    // Calculates the total water in mL
    flowMilliLitres = (flowRate / 60) * 1000;

    // Add the millilitres passed in this second to the cumulative total
    totalMilliLitres += flowMilliLitres;

    // Sends the flow rate to MyController server
    send(rainRateMSG.set(flowRate, 1));
    send(rainMSG.set(float(totalMilliLitres), 0));

    // Reset the pulse counter so we can start incrementing again
    pulseCount = 0;

    // Enable the interrupt again now that we've finished sending output
    attachInterrupt(digitalPinToInterrupt(rainGaugePin), pulseCounter, FALLING);
  }

  // if a day has past
  if (millis() - lastDay > 86400000) { 
    flowMilliLitres = 0; //reset rain total
  }

  // Get Gyroscope values
  sensors_event_t accel, gyro, temp;
  mpu.getEvent(&accel, &gyro, &temp);

  /* Landlslide detection based on rotation*/
  float totalRotation = sqrt(gyro.gyro.x * gyro.gyro.x + gyro.gyro.y * gyro.gyro.y + gyro.gyro.z * gyro.gyro.z);

  // If movement has been detected
  if (totalRotation > rotationDetection) {
    Serial.println("Movement Detected\n");
    send(moveMSG.set(true));
    lastSentMovementTime = millis();
  } else if (abs(millis() - lastSentMovementTime) > 3000) {  // if movement has not been detected for 3s
    send(moveMSG.set(false));
    lastSentMovementTime = millis();
  }
}

// Interrupt Service Routine
ICACHE_RAM_ATTR void pulseCounter() {
  // Increment the pulse counter
  pulseCount++;
}