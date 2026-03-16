#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <Adafruit_NeoPixel.h>

#define PIN1 4
#define PIN2 6
#define PIN3 8
#define PIN4 9
#define LED_PIN 3
#define NUM_LEDS 1

Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

unsigned long receivedMinutes = 0;
unsigned long lastMinutes = 0;
int motorPos = 0;
int lastStage = -1;

typedef struct { unsigned long minutes; } DataPacket;

int motorSteps[4][4] = {
  {1,0,0,1},{1,0,1,0},{0,1,1,0},{0,1,0,1}
};

void setMotor(int step) {
  digitalWrite(PIN1, motorSteps[step][0]);
  digitalWrite(PIN2, motorSteps[step][1]);
  digitalWrite(PIN3, motorSteps[step][2]);
  digitalWrite(PIN4, motorSteps[step][3]);
}

void moveMotor(int steps) {
  if (steps > 0) {
    for (int i = 0; i < steps; i++) {
      motorPos++;
      setMotor(motorPos % 4);
      delay(2);
    }
  } else {
    for (int i = 0; i < abs(steps); i++) {
      motorPos--;
      setMotor(((motorPos % 4) + 4) % 4);
      delay(2);
    }
  }
}

void onReceive(const uint8_t* mac, const uint8_t* data, int len) {
  DataPacket packet;
  memcpy(&packet, data, sizeof(packet));
  receivedMinutes = packet.minutes;
}

int getStage(unsigned long min) {
  if (min < 60) return 0;
  if (min < 120) return 1;
  if (min < 180) return 2;
  return 3;
}

void setLED(int stage) {
  if (stage == 0) {
    strip.setBrightness(20);
    strip.setPixelColor(0, strip.Color(0, 255, 0));
  } else if (stage == 1) {
    strip.setBrightness(20);
    strip.setPixelColor(0, strip.Color(255, 200, 0));
  } else if (stage == 2) {
    strip.setBrightness(20);
    strip.setPixelColor(0, strip.Color(255, 80, 0));
  } else {
    strip.setBrightness(150);
    strip.setPixelColor(0, strip.Color(255, 0, 0));
  }
  strip.show();
}

void setup() {
  Serial.begin(115200);

  pinMode(PIN1, OUTPUT);
  pinMode(PIN2, OUTPUT);
  pinMode(PIN3, OUTPUT);
  pinMode(PIN4, OUTPUT);

  WiFi.mode(WIFI_STA);
  esp_now_init();
  esp_now_register_recv_cb(onReceive);

  strip.begin();
  delay(500);
  setLED(0);

  Serial.println("Started");
}

void loop() {
  if (receivedMinutes != lastMinutes) {
    int diff = (int)(receivedMinutes - lastMinutes);

    if (receivedMinutes >= 180) {
      
      moveMotor(-motorPos);
      Serial.println("Returned to origin");
    } else {
      
      moveMotor(diff * 6);
    }
    lastMinutes = receivedMinutes;
  }

  int stage = getStage(receivedMinutes);
  if (stage != lastStage) {
    setLED(stage);
    lastStage = stage;
  }

  Serial.printf("Total: %lu min | Motor pos: %d\n", receivedMinutes, motorPos);
  delay(1000);
}