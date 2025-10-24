#include <WiFi.h>
#include <PubSubClient.h>

// WiFi credentials
const char* ssid = "dineshnp2";
const char* password = "dinesh21";

// MQTT broker
const char* mqtt_server = "broker.emqx.io";

WiFiClient espClient;
PubSubClient client(espClient);

// Timer for publishing
unsigned long lastMsg = 0;
const long interval = 10000;  // 10 seconds

void setup_wifi() {
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

// Callback function to handle incoming messages
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("\nMessage received on topic: ");
  Serial.print(topic);
  Serial.print(" | Message: ");

  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  Serial.println(message);

  // Example: take action
  if (message == "ON") {
    Serial.println("Received command: Turn ON");
    // digitalWrite(LED_BUILTIN, HIGH);
  } else if (message == "OFF") {
    Serial.println("Received command: Turn OFF");
    // digitalWrite(LED_BUILTIN, LOW);
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Connecting to MQTT...");
    if (client.connect("ESP32_Bidirectional_Client")) {
      Serial.println("connected");
      client.subscribe("/esp32/test1/hi");  // subscribe to MQTTX command topic
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  setup_wifi();

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  // Optional: pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long now = millis();
  if (now - lastMsg > interval) {
    lastMsg = now;
    String message = "Hii from ESP32C6 at " + String(millis() / 1000) + "s";
    client.publish("/esp32/test1/hello", message.c_str());  // publish to MQTTX
    Serial.println("Published: " + message);
  }
}
