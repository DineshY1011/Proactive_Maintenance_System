#include <WiFi.h>
#include <esp_now.h>
#include <PubSubClient.h>

// Wi-Fi Credentials (required for MQTT)
const char* ssid = "dineshnp2";
const char* password = "dinesh21";

// MQTT Broker
const char* mqtt_server = "broker.emqx.io"; // Or your private broker
const int mqtt_port = 1883;
const char* mqtt_topic = "/esp32/temperature";

// MQTT Client
WiFiClient espClient;
PubSubClient client(espClient);

// Must match sender
typedef struct struct_message {
  float temperatureC;
} struct_message;

struct_message incomingData;

// Function to publish to MQTT
void publishToMQTT(float temperature) {
  char jsonPayload[64];
  snprintf(jsonPayload, sizeof(jsonPayload), "{\"temperature\": %.2f}", temperature); // JSON format

  client.publish(mqtt_topic, jsonPayload);

  Serial.print("📡 Sent to MQTT (JSON): ");
  Serial.println(jsonPayload);
}

// ESP-NOW receive callback
void OnDataRecv(const esp_now_recv_info_t *info, const uint8_t *data, int data_len) {
  memcpy(&incomingData, data, sizeof(incomingData));

  Serial.print("📥 Received Temp from MAC: ");
  for (int i = 0; i < 6; i++) {
    Serial.printf("%02X", info->src_addr[i]);
    if (i < 5) Serial.print(":");
  }
  Serial.println();

  Serial.print("🌡 Temperature Received (°C): ");
  Serial.println(incomingData.temperatureC);

  // Publish to MQTT
  publishToMQTT(incomingData.temperatureC);
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  // Connect to WiFi (required for MQTT)
  WiFi.begin(ssid, password);
  Serial.print("🔌 Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ WiFi connected");

  Serial.print("Receiver MAC: ");
  Serial.println(WiFi.macAddress());

    // ✅ Print current Wi-Fi channel
  Serial.print("📶 Current Wi-Fi Channel: ");
  Serial.println(WiFi.channel());

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("❌ ESP-NOW init failed");
    return;
  }
  Serial.println("✅ ESP-NOW Initialized");

  esp_now_register_recv_cb(OnDataRecv);

  // Init MQTT
  client.setServer(mqtt_server, mqtt_port);
}

void loop() {
  if (!client.connected()) {
    // Auto reconnect to MQTT
    while (!client.connected()) {
      Serial.print("🔁 Connecting to MQTT...");
      if (client.connect("ESP32ReceiverClient")) {
        Serial.println("✅ Connected to MQTT");
      } else {
        Serial.print("❌ Failed, rc=");
        Serial.print(client.state());
        delay(2000);
      }
    }
  }
  client.loop(); // Maintain MQTT connection
}