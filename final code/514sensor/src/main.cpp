#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <esp_now.h>
#include <Adafruit_VEML7700.h>
#include <vl53l4cx_class.h>

Adafruit_VEML7700 veml;
VL53L4CX sensor(&Wire, 2);

#define DIST_THRESHOLD 800
#define LUX_MIN 30.0

// Display Board MAC address
uint8_t displayMac[] = {0x98, 0x3D, 0xAE, 0xAC, 0xF7, 0x70};

unsigned long screenOnSeconds = 0;
unsigned long lastCheckTime = 0;

typedef struct {
  unsigned long minutes;
} DataPacket;

void onSent(const uint8_t* mac, esp_now_send_status_t status) {
  Serial.printf("Send %s\n", status == ESP_NOW_SEND_SUCCESS ? "success" : "failed");
}

void setup() {
  Serial.begin(115200);
  Wire.begin();
  delay(1000);

  veml.begin();

  // Initialize VL53L4CX
  pinMode(2, OUTPUT);
  digitalWrite(2, LOW);
  delay(10);
  digitalWrite(2, HIGH);
  delay(10);
  sensor.begin();
  sensor.VL53L4CX_Off();
  sensor.InitSensor(0x52);
  sensor.VL53L4CX_StartMeasurement();

  // Initialize ESP-NOW
  WiFi.mode(WIFI_STA);
  esp_now_init();
  esp_now_register_send_cb(onSent);

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, displayMac, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);

  Serial.println("ESP-NOW sender started");
  lastCheckTime = millis();
}

void loop() {
  // Read light level
  float lux = veml.readLux();
  bool screenOn = lux >= LUX_MIN;

  // Read distance
  int dist = -1;
  VL53L4CX_MultiRangingData_t data;
  uint8_t ready = 0;
  sensor.VL53L4CX_GetMeasurementDataReady(&ready);
  if (ready) {
    sensor.VL53L4CX_GetMultiRangingData(&data);
    if (data.NumberOfObjectsFound > 0) {
      int d = data.RangeData[0].RangeMilliMeter;
      int s = data.RangeData[0].RangeStatus;
      if (s == 0 && d > 0 && d < 6000) dist = d;
    }
    sensor.VL53L4CX_ClearInterruptAndStartMeasurement();
  }

  // Determine if user is at computer
  bool personHere = dist > 0 && dist < DIST_THRESHOLD;
  bool usingComputer = screenOn && personHere;

  // Update timer and send data every second
  if (millis() - lastCheckTime >= 1000) {
    if (usingComputer) screenOnSeconds++;
    lastCheckTime = millis();

    DataPacket packet;
    packet.minutes = screenOnSeconds / 60;
    esp_now_send(displayMac, (uint8_t*)&packet, sizeof(packet));
    Serial.printf("Sent: %lu min\n", packet.minutes);
  }

  Serial.printf("Lux:%.1f | Dist:%dmm | Status:%s\n",
    lux, dist, usingComputer ? "In use" : "Not in use");

  delay(100);
}