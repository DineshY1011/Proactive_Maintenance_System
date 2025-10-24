import paho.mqtt.client as mqtt
from influxdb import InfluxDBClient
import json
from datetime import datetime

# MQTT settings
MQTT_BROKER = "broker.emqx.io"
MQTT_PORT = 1883
MQTT_TOPIC = "/esp32/sensorData"

# InfluxDB settings
INFLUX_HOST = "localhost"
INFLUX_PORT = 8086
INFLUX_DB = "sensor_data"

# Connect to InfluxDB
influx_client = InfluxDBClient(host=INFLUX_HOST, port=INFLUX_PORT)
influx_client.switch_database(INFLUX_DB)

def on_connect(client, userdata, flags, rc):
    print(f"[MQTT] Connected with result code {rc}")
    client.subscribe(MQTT_TOPIC)

def on_message(client, userdata, msg):
    print(f"[MQTT] Message received on topic {msg.topic}")
    try:
        payload = msg.payload.decode()
        data = json.loads(payload)  # Assuming ESP32 sends JSON
        print(f"[DATA] {data}")

        # Example: { "temperature": 26.5, "humidity": 60 }
        json_body = [
            {
                "measurement": "environment",
                "time": datetime.utcnow().isoformat(),
                "fields": {
                    "temperature": float(data["temperature"]),
                    "rpm": float(data["rpm"]),
                    "vibration": float(data["vibration"]),
                    "voltage": float(data["voltage"]),
                    "current": float(data["current"]),
                    "power": float(data["power"]),
                    "energy": float(data["energy"]),
                    "frequency": float(data["frequency"]),
                    "powerFactor": float(data["powerFactor"])
                }
            }
        ]

        influx_client.write_points(json_body)
        print("[InfluxDB] Data written")

    except Exception as e:
        print(f"[Error] {e}")

# Connect to MQTT broker
mqtt_client = mqtt.Client()
mqtt_client.on_connect = on_connect
mqtt_client.on_message = on_message

mqtt_client.connect(MQTT_BROKER, MQTT_PORT, 60)
mqtt_client.loop_forever()