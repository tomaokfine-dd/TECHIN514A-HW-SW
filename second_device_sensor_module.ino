/*
 * Second Device - Environmental Sensing Module
 * 
 * Hardware:
 * - XIAO ESP32 microcontroller
 * - TSL25911FN light sensor
 * - VL53L0X distance sensor
 * - 3.7V Li-Po battery
 * 
 * Functionality:
 * This device monitors ambient light and proximity, then communicates
 * with the motor control device to adjust the stepper motor position
 * based on environmental conditions.
 * 
 * Required Libraries:
 * - Adafruit_TSL2591
 * - VL53L0X
 * - WiFi (built-in ESP32)
 */

#include <Wire.h>
#include <Adafruit_TSL2591.h>
#include <VL53L0X.h>
#include <WiFi.h>

// Sensor objects
Adafruit_TSL2591 tsl = Adafruit_TSL2591(2591);
VL53L0X distanceSensor;

// WiFi credentials
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// Motor control device IP
const char* motorDeviceIP = "192.168.1.100";
const int motorDevicePort = 8080;

// Thresholds
const uint16_t LIGHT_THRESHOLD = 100;  // Lux
const uint16_t DISTANCE_THRESHOLD = 500;  // mm

void setup() {
  Serial.begin(115200);
  Wire.begin();
  
  Serial.println("Initializing Environmental Sensing Module...");
  
  // Initialize light sensor
  if (!tsl.begin()) {
    Serial.println("ERROR: TSL25911FN not found!");
    while (1);
  }
  Serial.println("TSL25911FN light sensor initialized");
  
  // Configure light sensor
  tsl.setGain(TSL2591_GAIN_MED);
  tsl.setTiming(TSL2591_INTEGRATIONTIME_300MS);
  
  // Initialize distance sensor
  distanceSensor.setTimeout(500);
  if (!distanceSensor.init()) {
    Serial.println("ERROR: VL53L0X not found!");
    while (1);
  }
  distanceSensor.startContinuous();
  Serial.println("VL53L0X distance sensor initialized");
  
  // Connect to WiFi
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  
  Serial.println("Setup complete!");
}

void loop() {
  // Read light sensor
  uint32_t lum = tsl.getFullLuminosity();
  uint16_t ir = lum >> 16;
  uint16_t full = lum & 0xFFFF;
  uint16_t lux = tsl.calculateLux(full, ir);
  
  // Read distance sensor
  uint16_t distance = distanceSensor.readRangeContinuousMillimeters();
  
  // Print sensor values
  Serial.print("Light: ");
  Serial.print(lux);
  Serial.print(" lux, Distance: ");
  Serial.print(distance);
  Serial.println(" mm");
  
  // Check thresholds and send motor commands
  if (lux < LIGHT_THRESHOLD) {
    sendMotorCommand("OPEN");  // Open blinds when dark
  } else if (lux > LIGHT_THRESHOLD * 2) {
    sendMotorCommand("CLOSE");  // Close blinds when bright
  }
  
  if (distance < DISTANCE_THRESHOLD) {
    sendMotorCommand("STOP");  // Stop motor if object detected nearby
  }
  
  delay(1000);  // Update every second
}

void sendMotorCommand(String command) {
  WiFiClient client;
  
  if (client.connect(motorDeviceIP, motorDevicePort)) {
    client.println(command);
    Serial.println("Sent command: " + command);
    client.stop();
  } else {
    Serial.println("Connection to motor device failed");
  }
}
