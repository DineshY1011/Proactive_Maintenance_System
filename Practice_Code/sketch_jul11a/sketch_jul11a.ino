#include <esp_now.h>
#include <WiFi.h>
#include <max6675.h>

// MAX6675 Pins
int thermoCLK = 18;
int thermoCS = 5;
int thermoDO = 19;

MAX6675 thermocouple(thermoCLK, thermoCS, thermoDO);

// MAC address of receiver ESP32 (replace with actual)
uint8_t receiverAddress[] = {0x54, 0x32, 0x04, 0x2F, 0x2F, 0x44};

// Structure for sending data
typedef struct struct_message {
  float temperatureC;
} struct_message;

struct_message myData;

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  Serial.println("Sender board initializing...");

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Register peer
  esp_now_peer_info_t peerInfo;
  memcpy(peerInfo.peer_addr, receiverAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }

  Serial.println("ESP-NOW ready");
}

void loop() {
  myData.temperatureC = thermocouple.readCelsius();
  Serial.print("Sending Temperature: ");
  Serial.println(myData.temperatureC);

  esp_err_t result = esp_now_send(receiverAddress, (uint8_t *) &myData, sizeof(myData));

  if (result == ESP_OK) {
    Serial.println("Sent successfully");
  } else {
    Serial.println("Send failed");
  }

  delay(2000);
}