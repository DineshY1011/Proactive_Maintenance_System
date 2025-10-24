#include <esp_now.h>
#include <WiFi.h>
#include <TM1637Display.h>
#include <max6675.h>
#include "esp_wifi.h"

// MAX6675 pins
int thermoCLK = 18;
int thermoCS = 5;
int thermoDO = 19;
MAX6675 thermocouple(thermoCLK, thermoCS, thermoDO);

// TM1637 Display pins
#define CLK 2
#define DIO 4
TM1637Display display(CLK, DIO);

// Receiver MAC address (replace if different)
uint8_t receiverAddress[] = {0x2C, 0xBC, 0xBB, 0x0C, 0x11, 0xA4};

typedef struct struct_message {
  uint8_t type;           // New field to identify message type
  float temperatureC;
} struct_message;

struct_message myData;

// Channel & status management
int workingChannel = -1;
int failedAttempts = 0;
const int MAX_FAILED_ATTEMPTS = 3;

bool ackReceived = false;

#define TEMP_THRESHOLD 50
bool alertToggle = false;  // toggles between display styles

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  ackReceived = (status == ESP_NOW_SEND_SUCCESS);
}

// Try to send a dummy message on a specific channel to check if receiver responds
bool tryChannel(int channel) {
  WiFi.mode(WIFI_STA);
  esp_wifi_set_promiscuous(false);
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  delay(10);

  if (esp_now_init() != ESP_OK) {
    Serial.println("❌ ESP-NOW init failed on channel " + String(channel));
    return false;
  }

  esp_now_register_send_cb(OnDataSent);

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, receiverAddress, 6);
  peerInfo.channel = 0;  // 0 = allow any
  peerInfo.encrypt = false;

  if (!esp_now_is_peer_exist(receiverAddress)) {
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
      Serial.println("❌ Failed to add peer");
      esp_now_deinit();
      return false;
    }
  }

  // Send dummy test packet
  ackReceived = false;
  struct_message testData;
  testData.type = 1;  // Type 1 = Temperature
  testData.temperatureC = 99.9;
  esp_now_send(receiverAddress, (uint8_t *)&testData, sizeof(testData));
  delay(200);

  esp_now_deinit();
  return ackReceived;
}

// Scan for the receiver channel (1–13)
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

// Setup ESP-NOW for the current channel
bool setupESPNow() {
  WiFi.mode(WIFI_STA);
  esp_wifi_set_channel(workingChannel, WIFI_SECOND_CHAN_NONE);

  if (esp_now_init() != ESP_OK) {
    Serial.println("❌ ESP-NOW init failed during setup");
    return false;
  }

  esp_now_register_send_cb(OnDataSent);

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, receiverAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (!esp_now_is_peer_exist(receiverAddress)) {
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
      Serial.println("❌ Failed to add peer");
      return false;
    }
  }

  Serial.print("📡 Locked to Channel: ");
  Serial.println(workingChannel);
  return true;
}

void setup() {
  Serial.begin(115200);
  display.setBrightness(0x0f);
  display.showNumberDec(0);

  // Initial channel discovery
  if (!findReceiverChannel()) {
    Serial.println("❌ No receiver found. Halting.");
    while (true);
  }

  if (!setupESPNow()) {
    Serial.println("❌ Failed to setup ESP-NOW. Halting.");
    while (true);
  }
}

void loop() {
  float tempC = thermocouple.readCelsius();
  Serial.print("🌡 Temperature (°C): ");
  Serial.println(tempC);

  bool overThreshold = (tempC >= TEMP_THRESHOLD);

  if (isnan(tempC) || tempC < 0 || tempC > 1000) {
    display.showNumberDec(0, false);
    myData.type = 1;
    myData.temperatureC = 0;
    } else if (overThreshold) {
    Serial.print("🚨 ALERT: High Temperature = ");
    Serial.println(tempC);

    int tempInt = (int)tempC;
    myData.type = 1;
    myData.temperatureC = tempC;

    if (alertToggle) {
      // 🔁 Blink actual temperature 3 times
      for (int i = 0; i < 3; i++) {
        display.showNumberDec(tempInt, false);
        delay(300);
        display.clear();
        delay(300);
      }
    } else {
      // 🔁 Blink "ALRT" 3 times using segments
      uint8_t alert[] = {
  0x77, // A
  0x38, // L
  0x50, // R
  0x78  // T
};

      for (int i = 0; i < 3; i++) {
        display.setSegments(alert);
        delay(300);
        display.clear();
        delay(300);
      }
    }

    alertToggle = !alertToggle;  // flip next cycle
    myData.type = 1;
    myData.temperatureC = tempC;

  } else {
    // Normal temperature, show regularly
    int tempInt = (int)tempC;
    display.showNumberDec(tempInt, false);
    myData.type = 1;
    myData.temperatureC = tempC;
  }

  // Send data over ESP-NOW
  ackReceived = false;
  esp_err_t result = esp_now_send(receiverAddress, (uint8_t *)&myData, sizeof(myData));

  if (result == ESP_OK) {
    delay(200);  // wait for ack
    if (ackReceived) {
      Serial.println("✅ Data sent successfully");
      failedAttempts = 0;
    } else {
      Serial.println("⚠️ Sent but no ACK");
      failedAttempts++;
    }
  } else {
    Serial.print("❌ Send error: ");
    Serial.println(result);
    failedAttempts++;
  }

  // Reconnect if needed
  if (failedAttempts >= MAX_FAILED_ATTEMPTS) {
    Serial.println("🔄 Too many failures. Re-scanning channels...");

    // Clean up
    esp_now_del_peer(receiverAddress);
    esp_now_deinit();

    // Retry until we re-find the receiver
    while (true) {
      if (findReceiverChannel() && setupESPNow()) {
        Serial.println("✅ Reconnected successfully!");
        failedAttempts = 0;
        break;
      }
      Serial.println("🔁 Receiver not found. Retrying in 5 seconds...");
      delay(5000);
    }
  }
  delay(1000);
}