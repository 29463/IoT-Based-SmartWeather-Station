#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DHT.h>

#define DHTPIN D4
#define DHTTYPE DHT11
#define WIND_SENSOR_PIN D6

const char* ssid = "RAAS-STAFF";
const char* password = "R@@s#098";

DHT dht(DHTPIN, DHTTYPE);
ESP8266WebServer server(80);

volatile unsigned long rotationCount = 0;
unsigned long lastWindMillis = 0;
const unsigned long windInterval = 1000;
const float windSpeedFactor = 2.4;
float windSpeed = 0.0;

float temperature = 0.0;
float humidity = 0.0;
bool heatstrokeAlert = false;
bool fogAlert = false;
bool dewAlert = false;
bool frostAlert = false;
bool stormAlert = false;
bool rainAlert = false;

void readSensors() {
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();

  if (!isnan(temp)) temperature = temp;
  if (!isnan(hum)) humidity = hum;

  heatstrokeAlert = (temperature > 40 && humidity < 30);
  fogAlert = (humidity > 90 && temperature < 25);
  dewAlert = (humidity > 90 && temperature < 20);
  frostAlert = (temperature < 5);
  stormAlert = (windSpeed > 50);
  rainAlert = (humidity > 95);
}

String getSensorData() {
  String json = "{";
  json += "\"temp\":" + String(temperature, 2) + ",";
  json += "\"hum\":" + String(humidity, 2) + ",";
  json += "\"wind\":" + String(windSpeed, 2) + ",";
  json += "\"heatstroke\":" + String(heatstrokeAlert ? 1 : 0) + ",";
  json += "\"fog\":" + String(fogAlert ? 1 : 0) + ",";
  json += "\"dew\":" + String(dewAlert ? 1 : 0) + ",";
  json += "\"frost\":" + String(frostAlert ? 1 : 0) + ",";
  json += "\"storm\":" + String(stormAlert ? 1 : 0) + ",";
  json += "\"rain\":" + String(rainAlert ? 1 : 0);
  json += "}";
  return json;
}

void handleData() {
  server.send(200, "application/json", getSensorData());
}

void handleRoot() {
  String html = R"====(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <title>Smart Weather Station</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
  <style>
    body {
      font-family: 'Segoe UI', sans-serif;
      background: linear-gradient(to right, #f9fbe7, #e1f5fe);
      margin: 0;
      padding: 0;
      color: #333;
    }
    header {
      background-color: #003366;
      color: white;
      padding: 15px 0;
      text-align: center;
    }
    h1, h2, h3, h4, h5 {
      margin: 4px;
    }
    .container {
      max-width: 1000px;
      margin: auto;
      padding: 10px;
    }
    .grid {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
      gap: 15px;
    }
    .card {
      background: white;
      border-radius: 15px;
      padding: 15px;
      box-shadow: 0 4px 12px rgba(0,0,0,0.15);
      text-align: center;
      transition: 0.3s ease-in-out;
    }
    .card:hover {
      transform: translateY(-5px);
      box-shadow: 0 6px 16px rgba(0,0,0,0.2);
    }
    .label {
      font-size: 1.2em;
      margin-bottom: 8px;
      font-weight: bold;
    }
    canvas {
      height: 180px !important;
    }
  </style>
</head>
<body>
  <header>
    <h1>IOT BASED SMART WEATHER STATION</h1>
    <h2>THE HUNAR FOUNDATION</h2>
    <h3>THF-RAAS</h3>
    <h4>DESIGNED BY: MUHAMMAD ABEER, SYED SHARIB, SYED SUFIYAN</h4>
    <h5>SUPERVISED BY: Engr. Babu Mal</h5>
  </header>
  <div class="container">
    <div class="grid">
      <div class="card"><div class="label">🌡️ Temperature (°C)</div><canvas id="tempChart"></canvas></div>
      <div class="card"><div class="label">💧 Humidity (%)</div><canvas id="humChart"></canvas></div>
      <div class="card"><div class="label">🌬️ Wind Speed (km/h)</div><canvas id="windChart"></canvas></div>
      <div class="card"><div class="label">🥵 Heatstroke Risk</div><canvas id="heatstrokeChart"></canvas></div>
      <div class="card"><div class="label">🌫️ Fog Alert</div><canvas id="fogChart"></canvas></div>
      <div class="card"><div class="label">💧 Dew Alert</div><canvas id="dewChart"></canvas></div>
      <div class="card"><div class="label">❄️ Frost Alert</div><canvas id="frostChart"></canvas></div>
      <div class="card"><div class="label">🌪️ Storm Alert</div><canvas id="stormChart"></canvas></div>
      <div class="card"><div class="label">🌧️ Rain Alert</div><canvas id="rainChart"></canvas></div>
    </div>
  </div>
  <script>
    const charts = {};
    const labels = ['Temp (°C)', 'Humidity (%)', 'Wind (km/h)', 'Heatstroke', 'Fog', 'Dew', 'Frost', 'Storm', 'Rain'];
    const ids = ['temp', 'hum', 'wind', 'heatstroke', 'fog', 'dew', 'frost', 'storm', 'rain'];
    const colors = ['#e53935', '#1e88e5', '#43a047', '#ffb300', '#6a1b9a', '#00897b', '#607d8b', '#d32f2f', '#1565c0'];

    ids.forEach((id, i) => {
      charts[id] = new Chart(document.getElementById(id + 'Chart'), {
        type: i < 3 ? 'line' : 'bar',
        data: {
          labels: [],
          datasets: [{
            label: labels[i],
            backgroundColor: i < 3 ? 'transparent' : colors[i] + '88',
            borderColor: colors[i],
            data: [],
            tension: 0.4,
            fill: false
          }]
        },
        options: {
          responsive: true,
          maintainAspectRatio: false,
          scales: {
            x: { display: false },
            y: {
              beginAtZero: true,
              max: i >= 3 ? 1.5 : undefined
            }
          }
        }
      });
    });

    function updateChart(chart, value) {
      const now = new Date().toLocaleTimeString();
      if (chart.data.labels.length > 10) {
        chart.data.labels.shift();
        chart.data.datasets[0].data.shift();
      }
      chart.data.labels.push(now);
      chart.data.datasets[0].data.push(value);
      chart.update();
    }

    function fetchData() {
      fetch('/data')
        .then(res => res.json())
        .then(data => {
          ids.forEach(id => updateChart(charts[id], data[id]));
        });
    }

    setInterval(fetchData, 3000);
  </script>
</body>
</html>
  )====";
  server.send(200, "text/html", html);
}

void ICACHE_RAM_ATTR countRotation() {
  rotationCount++;
}

void setup() {
  Serial.begin(115200);
  dht.begin();
  delay(2000);
  pinMode(WIND_SENSOR_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(WIND_SENSOR_PIN), countRotation, RISING);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.begin();
  Serial.println("Server started");
}

void loop() {
  server.handleClient();
  if (millis() - lastWindMillis >= windInterval) {
    lastWindMillis = millis();
    unsigned long count = rotationCount;
    rotationCount = 0;
    windSpeed = count * windSpeedFactor;
    readSensors();
  }
}
