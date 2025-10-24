# 🏭 Industry Proactive Maintenance System Using Sensor Data Monitoring and Cloud Forecasting

## 📘 Project Overview

This project was developed as part of an industrial internship focusing on creating a **Proactive Maintenance System** for smart industries.

The goal was to continuously monitor critical machine parameters such as **temperature, vibration, power, and RPM** and provide real-time analytics using IoT technologies and cloud integration.

The system enables **predictive maintenance** by detecting early signs of machine failure through live sensor data monitoring and long-term trend visualization.

---

## 👥 Team Composition

- **Team Size:** 5 Members
- **My Role:** Temperature Monitoring System & Cloud Dashboard Developer

### 🧩 My Contributions:
1. Designed and implemented the **Temperature Monitoring System** using the MAX6675 thermocouple sensor and ESP32-C6.
2. Developed the **Grafana dashboard** and integrated **InfluxDB** for real-time and historical data visualization across all modules.
3. Configured data pipelines from MQTT to InfluxDB, enabling seamless visualization and storage of multi-sensor data.

---

## 🔧 My Module — Temperature Monitoring System (MAX6675)

### ⚙️ Hardware Components
- **ESP32-C6 (PCBcupid Glyph C6)** – Core microcontroller
- **MAX6675 K-Type Thermocouple Module** – Temperature sensor
- **TM1637 4-Digit 7-Segment Display** – Real-time temperature display
- **Thermocouple Probe** – Measures machine surface temperature

### 🧠 Working Principle
1. Continuously reads temperature data from the MAX6675 sensor.
2. Displays readings on the TM1637 display in °C.
3. Transmits data wirelessly to a receiver node via **ESP-NOW protocol**.
4. Issues a **high-temperature alert** if temperature exceeds 50 °C.
5. Ensures reliability with automatic channel scanning and reconnection logic.

### 🔑 Key Features
- Real-time temperature monitoring (0–1000 °C range)
- Alert indicator for high temperature
- Wireless ESP-NOW communication (router-free)
- Data validation and error recovery system
- Structured message format:
```json
{
  "type": 1,
  "temperatureC": 45.8
}
```

---

## 🌐 System Integration (Team Project)

The system integrates four primary sensor modules:

| Sensor | Parameter | Description |
|--------|-----------|-------------|
| MAX6675 | Temperature | Detects machine overheating |
| MPU6050 | Vibration | Monitors vibration intensity |
| A3144 Hall Sensor | RPM | Calculates rotational speed |
| PZEM-004T | Power | Monitors voltage, current, power factor |

Each sensor node (ESP32) sends data to a gateway ESP32 via **ESP-NOW**.

The gateway then publishes the readings to an **MQTT broker**, from which they are stored in **InfluxDB** and visualized through **Grafana dashboards**.

---

## 📊 My Work — Grafana Dashboard & InfluxDB Integration

### 🧩 Overview
To enhance the system's analytical capability and usability, I implemented **Grafana** and **InfluxDB** as part of my contribution.

This addition allowed **real-time visualization, historical trend analysis, and alert management** across all sensor parameters.

### ⚙️ Implementation Steps
1. Configured **InfluxDB** as a time-series database to store sensor data received from the MQTT broker.
2. Used **Python script** to subscribe to MQTT topics and push data into InfluxDB in JSON format.
3. Designed an interactive **Grafana dashboard** connected to InfluxDB, visualizing live and historical readings.
4. Added dynamic panels for Temperature, Vibration, RPM, and Power with **threshold-based alerts**.

### 📈 Dashboard Features
- Real-time visualization with auto-refreshing graphs
- Historical trend analysis for performance forecasting
- Alert triggers for abnormal readings
- Custom time-range filtering (Last 1 hour, 1 day, 1 week)
- Unified dashboard combining all sensors

### 🗄️ Data Flow
```
     [Industrial Machine]
              │
    [ESP32-C6 Sensor Nodes]
(MAX6675 / MPU6050 / PZEM-004T / A3144)
              │
           ESP-NOW
              │
      [Gateway ESP32-C6]
              │
          MQTT Broker
              │
┌───────────────┬────────────────┐
│    Grafana    │  Web Dashboard │
│(Visualization)| (Real-time UI) │
└───────────────┴────────────────┘
              │
InfluxDB (Historical Storage)
```

---

## 💻 MQTT Web Dashboard (Base Version)

Before introducing Grafana, an initial **MQTT-based Web Dashboard** was built using:
- HTML, JavaScript, Chart.js, and MQTT.js
- Subscribed to sensor topics and displayed real-time graphs in the browser
- Responsive and modular interface for live factory floor monitoring

---

## 🧠 Skills and Technologies Learned

- **Embedded Programming:** ESP32 (ESP-NOW)
- **IoT Communication:** MQTT, ESP-NOW mesh networking
- **Cloud & Databases:** InfluxDB setup, retention policies, queries
- **Data Visualization:** Grafana dashboard creation and alert configuration
- **Full IoT Pipeline:** Sensor → Wireless → Cloud → Visualization

---

## 🧾 Outcome

✅ Designed a Temperature Monitoring Prototype using MAX6675  
✅ Successfully implemented Grafana + InfluxDB integration for all modules  
✅ Created an end-to-end IoT-based predictive maintenance system  
✅ Demonstrated data reliability, historical tracking, and real-time analytics

---

## 📅 Internship Details

- **Title:** *Industry Proactive Maintenance System Using Sensor Data Monitoring and Cloud Forecasting*
- **Duration:** *June 2025 – July 2025*
- **Team Size:** 5
- **My Role:** *Temperature Monitoring & Grafana–InfluxDB Developer*
- **Organization:** *Sanleva Prime Solutions*