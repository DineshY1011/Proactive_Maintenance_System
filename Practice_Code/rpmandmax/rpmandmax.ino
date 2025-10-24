#include <esp_now.h>
#include <WiFi.h>
#include <TM1637Display.h>
#include <max6675.h>
#include "esp_wifi.h"

// ------- MAX6675 Thermocouple Pins -------
int thermoCLK = 18;
int thermoCS = 5;
int thermoDO = 19;
MAX6675 thermocouple(thermoCLK, thermoCS, thermoDO);

// ------- TM1637 Display Pins -------
#define CLK 2
#define DIO 4
TM1637Display display(CLK, DIO);

// ------- Hall Sensor -------
#define HALL_SENSOR_PIN 15

// ------- Receiver MAC Address -------
uint8_t receiverAddress[] = {0x2C, 0xBC, 0xBB, 0x0C, 0x11, 0xA4};

// ------- Data Structure -------
typedef struct struct_message {
  uint8_t type;
  float value; // Used for both temperature (°C) and RPM
} struct_message;

struct_message myData;

// ------- ESP-NOW Channel Variables -------
int workingChannel = -1;
int failedAttempts = 0;
const int MAX_FAILED_ATTEMPTS = 3;
bool ackReceived = false;

// ------- Temperature Settings -------
#define TEMP_THRESHOLD 50

// ------- RPM Calculation Variables -------
volatile int pulseCount = 0;
unsigned long lastPulseTime = 0;
unsigned long lastRPMCheck = 0;
const int pulseInterval = 50;   // Debounce
const int sampleTime = 1000;    // RPM Sample Time (ms)
int currentRPM = 0;
#define RPM_THRESHOLD 1000

bool alertToggle = false;

// ------- ISR for Hall Sensor -------
void IRAM_ATTR pulseISR() {
  unsigned long now = millis();
  if (now - lastPulseTime > pulseInterval) {
    pulseCount++;
    lastPulseTime = now;
  }
}

// ------- On Data Sent Callback -------
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  ackReceived = (status == ESP_NOW_SEND_SUCCESS);
}

// ------- Try Channel -------
bool tryChannel(int channel) {
  WiFi.mode(WIFI_STA);
  esp_wifi_set_promiscuous(false);
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  delay(10);

  if (esp_now_init() != ESP_OK) return false;
  esp_now_register_send_cb(OnDataSent);

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, receiverAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  if (!esp_now_is_peer_exist(receiverAddress)) {
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
      esp_now_deinit();
      return false;
    }
  }

  // Send dummy message
  ackReceived = false;
  struct_message dummy;
  dummy.type = 2;
  dummy.value = 9999;
  esp_now_send(receiverAddress, (uint8_t *)&dummy, sizeof(dummy));
  delay(200);
  esp_now_deinit();
  return ackReceived;
}

// ------- Scan All Channels -------
bool findReceiverChannel() {
  Serial.println("🔍 Scanning channels 1 to 13...");
  for (int ch = 1; ch <= 13; ch++) {
    Serial.print("📡 Testing channel ");
    Serial.println(ch);
    if (tryChannel(ch)) {
      workingChannel = ch;
      Serial.print("✅ Receiver found on channel ");
      Serial.println(ch);
      return true;
    }
  }
  workingChannel = -1;
  return false;
}

// ------- Setup ESP-NOW -------
bool setupESPNow() {
  WiFi.mode(WIFI_STA);
  esp_wifi_set_channel(workingChannel, WIFI_SECOND_CHAN_NONE);

  if (esp_now_init() != ESP_OK) return false;
  esp_now_register_send_cb(OnDataSent);

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, receiverAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  if (!esp_now_is_peer_exist(receiverAddress)) {
    if (esp_now_add_peer(&peerInfo) != ESP_OK) return false;
  }
  Serial.print("📡 Locked to Channel: ");
  Serial.println(workingChannel);
  return true;
}

// ------- Setup -------
void setup() {
  Serial.begin(115200);
  display.setBrightness(0x0f);
  display.showNumberDec(0);

  pinMode(HALL_SENSOR_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(HALL_SENSOR_PIN), pulseISR, FALLING);

  if (!findReceiverChannel()) {
    Serial.println("❌ No receiver found. Halting.");
    while (true);
  }

  if (!setupESPNow()) {
    Serial.println("❌ Failed to setup ESP-NOW. Halting.");
    while (true);
  }
}

// ------- Loop -------
void loop() {
  // ----- Temperature Measurement -----
  float tempC = thermocouple.readCelsius();
  if (!isnan(tempC) && tempC > 0 && tempC < 1000) {
    Serial.print("🌡 Temp (°C): ");
    Serial.println(tempC);
    bool overTemp = (tempC >= TEMP_THRESHOLD);
    int tempInt = (int)tempC;

    if (overTemp) {
      if (alertToggle) {
        for (int i = 0; i < 3; i++) {
          display.showNumberDec(tempInt, false);
          delay(300); display.clear(); delay(300);
        }
      } else {
        uint8_t alert[] = {0x77, 0x38, 0x50, 0x78};
        for (int i = 0; i < 3; i++) {
          display.setSegments(alert);
          delay(300); display.clear(); delay(300);
        }
      }
      alertToggle = !alertToggle;
    } else {
      display.showNumberDec(tempInt, false);
    }

    // Send temp data
    myData.type = 1;
    myData.value = tempC;
    ackReceived = false;
    esp_err_t result = esp_now_send(receiverAddress, (uint8_t *)&myData, sizeof(myData));

    if (result == ESP_OK) {
      delay(200);
      Serial.println(ackReceived ? "✅ Temp sent" : "⚠️ Sent, no ACK");
      failedAttempts = ackReceived ? 0 : failedAttempts + 1;
    } else {
      Serial.print("❌ Temp send error: ");
      Serial.println(result);
      failedAttempts++;
    }
  }

  // ----- RPM Calculation -----
  unsigned long currentMillis = millis();
  if (currentMillis - lastRPMCheck >= sampleTime) {
    detachInterrupt(digitalPinToInterrupt(HALL_SENSOR_PIN));
    currentRPM = pulseCount * 60;
    pulseCount = 0;
    lastRPMCheck = currentMillis;
    attachInterrupt(digitalPinToInterrupt(HALL_SENSOR_PIN), pulseISR, FALLING);

    Serial.print("⚙ RPM: ");
    Serial.println(currentRPM);

    if (currentRPM >= RPM_THRESHOLD) {
      if (alertToggle) {
        for (int i = 0; i < 3; i++) {
          display.showNumberDec(currentRPM, false);
          delay(300); display.clear(); delay(300);
        }
      } else {
        uint8_t alert[] = {0x77, 0x38, 0x50, 0x78};
        for (int i = 0; i < 3; i++) {
          display.setSegments(alert);
          delay(300); display.clear(); delay(300);
        }
      }
      alertToggle = !alertToggle;
    } else {
      display.showNumberDec(currentRPM, false);
    }

    // Send RPM data
    myData.type = 2;
    myData.value = currentRPM;
    ackReceived = false;
    esp_err_t result = esp_now_send(receiverAddress, (uint8_t *)&myData, sizeof(myData));

    if (result == ESP_OK) {
      delay(200);
      Serial.println(ackReceived ? "✅ RPM sent" : "⚠️ Sent, no ACK");
      failedAttempts = ackReceived ? 0 : failedAttempts + 1;
    } else {
      Serial.print("❌ RPM send error: ");
      Serial.println(result);
      failedAttempts++;
    }
  }

  // ----- Recovery on Failure -----
  if (failedAttempts >= MAX_FAILED_ATTEMPTS) {
    Serial.println("🔄 Too many failures. Reconnecting...");
    esp_now_del_peer(receiverAddress);
    esp_now_deinit();
    while (true) {
      if (findReceiverChannel() && setupESPNow()) {
        Serial.println("✅ Reconnected");
        failedAttempts = 0;
        break;
      }
      Serial.println("🔁 Retrying in 5 seconds...");
      delay(5000);
    }
  }

  delay(1000);  // 1 second loop
}