#include <WiFi.h>
#include <WebServer.h>
#include <DHT.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET -1 
#define DHTPIN 4
#define DHTTYPE DHT11

#define MOISTURE_PIN 32  // ADC1_CH4
#define LIGHT_PIN 33     // ADC1_CH5

DHT dht(DHTPIN, DHTTYPE);
WebServer server(80);

int moistureValue = 0;
int lightValue = 0;
float temperature = 0;
float humidity = 0;

unsigned long lastMoistureLightRead = 0;
unsigned long lastDHTRead = 0;

const unsigned long moistureLightInterval = 30UL * 60UL * 1000UL; // 30 minutes
const unsigned long dhtInterval = 15UL * 60UL * 1000UL;           // 15 minutes
#define SENSOR_INTERVAL_MS 10000  // 60 seconds

const char* ssid = "A1_A3AEFA";
const char* password = "430af6a4";


Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);


  String getLightLevel(int lightValue) {
  if (lightValue > 3000) {
    return "Dark";
  } else if (lightValue > 2000) {
    return "Dim";
  } else if (lightValue > 800) {
    return "Light";
  } else if (lightValue > 40) {
    return "Bright";
  } else {
    return "Very Bright";
  }
}

String getMoistureLevel(int moistureValue) {
  if (moistureValue >= 4000) {
    return "Sensor Dry / Disconnected";
  } else if (moistureValue > 3500) {
    return "Very Dry";
  } else if (moistureValue > 2800) {
    return "Dry";
  } else if (moistureValue > 2200) {
    return "Moist";
  } else if (moistureValue > 1800) {
    return "Wet";
  } else {
    return "Very Wet";
  }
}


void handleRoot() {
  String html = "<html><head><title>Smart Pot</title></head><body>";
  html += "<meta http-equiv='refresh' content='10'>";
  html += "<h1>Sensor Values</h1>";
  html += "<p>Moisture: " + getMoistureLevel(moistureValue); + "</p>";
  html += "<p>Light: " + getLightLevel(lightValue); + "</p>";
  html += "<p>Temperature: " + String(temperature) + " &deg;C</p>";
  html += "<p>Humidity: " + String(humidity) + " %</p>";
  html += "</body></html>";

//////////////////////////////

/////////////////////////////

  server.send(200, "text/html", html);
}

void setup() {
  Serial.begin(115200);

  // Initialize sensors
  dht.begin();

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected! IP address: ");
  Serial.println(WiFi.localIP());

  // Setup HTTP server
  server.on("/", handleRoot);
   server.onNotFound([]() {
    server.sendHeader("Location", "/");
    server.send(302, "text/plain", "");
  });
  server.begin();

  // Initial sensor reads
  moistureValue = analogRead(MOISTURE_PIN);
  lightValue = analogRead(LIGHT_PIN);
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();

  lastMoistureLightRead = millis();
  lastDHTRead = millis();
}

void loop() {
  server.handleClient();

  unsigned long currentMillis = millis();

  // Moisture & light every 30 minutes
  if (currentMillis - lastMoistureLightRead >= SENSOR_INTERVAL_MS ) { //moistureLightInterval
    moistureValue = analogRead(MOISTURE_PIN);
    lightValue = analogRead(LIGHT_PIN);
    Serial.printf("Moisture: %d, Light: %d\n", moistureValue, lightValue);
    lastMoistureLightRead = currentMillis;
    
  }

  // DHT11 every 15 minutes
  if (currentMillis - lastDHTRead >= SENSOR_INTERVAL_MS ) { //dhtInterval
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    if (!isnan(h) && !isnan(t)) {
      humidity = h;
      temperature = t - 8; // Adjust for calibration if needed
      Serial.printf("DHT11 Temp: %.1f °C, Humidity: %.1f %%\n", temperature, humidity);
    } else {
      Serial.println("Failed to read from DHT sensor!");
    }
    lastDHTRead = currentMillis;
  }

}
