// #include <WiFi.h>
#include <WebServer.h>
#include <DHT.h>
#include <Adafruit_SSD1306.h>
#include <WifiClientSecure.h>
#include <PubSubClient.h>
#include <WifiClientSecure.h>
#include "tls_ca.h"

#define DHTPIN 4
#define DHTTYPE DHT11
#define MOISTURE_PIN 32
#define LIGHT_PIN 33
#define PUMP_PIN 5
#define ACT_PIN 2               // onboard LED for emulation


const char* WIFI_SSID = "A1_A3AEFA";
const char* WIFI_PASS = "430af6a4";

const char* MQTT_HOST = "4629ff2d3b384c748f95b499f10e4f5b.s1.eu.hivemq.cloud";
const int   MQTT_PORT = 8883; // TLS (Transport Layer Security)
const char* MQTT_USER = "hivemq.webclient.1755195003530";
const char* MQTT_PASS = "8354ECQbTRgAeafh:%!$";

const char* TOPIC_SENS = "plantpot/sensors";
const char* TOPIC_CMD  = "plantpot/pump"; ///cmd
const char* TOPIC_ACK  = "plantpot/pump/ack";
const char* TOPIC_STATUS = "plantpot/status";

// Калибрация и изглаждане
static const float TEMP_SCALE   = 1.0f;
static const float TEMP_OFFSET_C= -6.0f;  // твоята температурна корекция
static const float HUM_SCALE    = 1.0f;
static const float HUM_OFFSET   = 24.0f;  // твоята влажностна корекция
static const float EMA_ALPHA    = 0.30f;  // EMA (Exponential Moving Average)
static float t_ema = NAN, h_ema = NAN;
unsigned long pumpUntil = 0;

const bool LED_ACTIVE_HIGH = true; // ако не светва, смени на true

inline void ledOn()  { digitalWrite(ACT_PIN, LED_ACTIVE_HIGH ? HIGH : LOW); }
inline void ledOff() { digitalWrite(ACT_PIN, LED_ACTIVE_HIGH ? LOW  : HIGH); }

WebServer server(80);

DHT dht(DHTPIN, DHTTYPE);
WiFiClientSecure net;
PubSubClient mqtt(net);

int moistureValue = 0;
int lightValue = 0;
float temperature = 0;
float humidity = 0;

unsigned long lastMoistureLightRead = 0;
unsigned long lastDHTRead = 0;

const unsigned long moistureLightInterval = 30UL * 60UL * 1000UL; // 30 minutes
const unsigned long dhtInterval = 15UL * 60UL * 1000UL;           // 15 minutes
#define SENSOR_INTERVAL_MS 10000  // 60 seconds

String jsonEscape(const String& s) { // minimal escaper
  String out; out.reserve(s.length()+4);
  for (char c: s) { if (c=='"'||c=='\\') out += '\\'; out += c; }
  return out;
}


void mqttEnsure() {
  while (!mqtt.connected()) {
    Serial.print("MQTT connect...");
    if (mqtt.connect("esp32-plantpot", MQTT_USER, MQTT_PASS)) {
      Serial.println("ok");
    } else {
      Serial.printf("fail rc=%d, retry in 5s\n", mqtt.state());
      delay(5000);
    }
  }
}



// void onMqtt(char* topic, byte* payload, unsigned int len){
//   String t(topic);
//   String msg; msg.reserve(len); for (unsigned i=0;i<len;i++) msg += (char)payload[i];

//   if (t == TOPIC_CMD){
//     String result = "error";
//     if (msg.equalsIgnoreCase("ON"))  { digitalWrite(PUMP_PIN, HIGH); result="ok"; }
//     if (msg.equalsIgnoreCase("OFF")) { digitalWrite(PUMP_PIN, LOW);  result="ok"; }
//     // прост ACK (JSON (JavaScript Object Notation))
//     String ack = String("{\"cmd\":\"")+msg+"\",\"result\":\""+result+"\"}";
//     mqtt.publish(TOPIC_ACK, ack.c_str(), false);
//   }
// }

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

  String lightLabel(int v){
  if (v > 3000) return "Dark";
  else if (v > 2000) return "Dim";
  else if (v > 800)  return "Light";
  else if (v > 40)   return "Bright";
  else               return "Very Bright";
}
String moistureLabel(int v){
  if (v >= 4000) return "Sensor Dry / Disconnected";
  else if (v > 3500) return "Very Dry";
  else if (v > 2800) return "Dry";
  else if (v > 2200) return "Moist";
  else if (v > 1800) return "Wet";
  else return "Very Wet";
}


    void onMqtt(char* topic, byte* payload, unsigned int len){
  String t(topic);
  String msg; msg.reserve(len);
  for (unsigned i=0;i<len;i++) msg += (char)payload[i];
  msg.trim();

  if (t == TOPIC_CMD){
    String result = "ok";
    String up = msg; up.toUpperCase();

    if (up.startsWith("ON")){
      int ms = 5000;
      int sp = up.indexOf(' ');
      if (sp > 0) {
        int sec = up.substring(sp+1).toInt();
        ms = constrain(sec*1000, 500, 600000);
      }
      ledOn();
      pumpUntil = millis() + ms;
    } else if (up == "OFF"){
      ledOff();
      pumpUntil = 0;
    } else {
      result = "bad_cmd";
    }

    String ack = String("{\"cmd\":\"")+msg+"\",\"result\":\""+result+"\"}";
    mqtt.publish(TOPIC_ACK, ack.c_str(), false);
  }
}


  bool mqttConnect(){
    // net.setInsecure(); // за бърз старт; за дипломна ползвай setCACert(...) вместо това
    net.setCACert(TLS_CA_PEM);
    mqtt.setServer(MQTT_HOST, MQTT_PORT);
    mqtt.setCallback(onMqtt);
    mqtt.setKeepAlive(10);

    // will: "offline" retained
    const char* clientId = "esp32-plantpot";

    if (!mqtt.connect(clientId, MQTT_USER, MQTT_PASS,
                      TOPIC_STATUS, 1, true, "offline")) return false;

    mqtt.publish(TOPIC_STATUS, "online", true); // retained "online"
    mqtt.subscribe(TOPIC_CMD);     
                 // команди към помпата
    return true;
  }

  // Connect to Wi-Fi
    void wifiConnect(){
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    while (WiFi.status()!=WL_CONNECTED){ delay(300); }
  }



int clampi(int v,int lo,int hi){ return v<lo?lo:(v>hi?hi:v); }



void setup() {
  Serial.begin(115200);

  pinMode(PUMP_PIN, OUTPUT); 
  digitalWrite(PUMP_PIN, LOW);
  pinMode(ACT_PIN,  OUTPUT); ledOff();

  dht.begin();
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);

  wifiConnect();
  // If mqttConnect() already does setCallback(onMqtt), do NOT call it here.
  mqtt.setCallback(onMqtt);
  while (!mqttConnect()) { delay(2000); }

}

void loop() {
  if (!mqtt.connected()) {
    if (!mqttConnect()) { delay(2000); return; }
  }
  mqtt.loop();

  // auto-OFF every loop, precise timing
  if (pumpUntil && (long)(millis() - pumpUntil) >= 0) {
    ledOff();
    pumpUntil = 0;
    mqtt.publish(TOPIC_ACK, "{\"cmd\":\"AUTO_OFF\",\"result\":\"ok\"}", false);
  }

  static unsigned long t0 = 0;
  if (millis() - t0 >= 10000) {
    t0 = millis();

    moistureValue = analogRead(MOISTURE_PIN);
    lightValue    = analogRead(LIGHT_PIN);

    float h_raw = dht.readHumidity();
    float t_raw = dht.readTemperature();
    bool dhtOK = !(isnan(h_raw) || isnan(t_raw));
    if (dhtOK) {
      float t_corr = t_raw * TEMP_SCALE + TEMP_OFFSET_C;
      float h_corr = h_raw * HUM_SCALE  + HUM_OFFSET;
      h_corr = constrain(h_corr, 0.0f, 100.0f);

      if (isnan(t_ema)) { t_ema = t_corr; h_ema = h_corr; }
      else {
        t_ema = EMA_ALPHA * t_corr + (1.0f - EMA_ALPHA) * t_ema;
        h_ema = EMA_ALPHA * h_corr + (1.0f - EMA_ALPHA) * h_ema;
      }
      temperature = t_ema;
      humidity    = h_ema;
    }

    String payload = "{";
    payload += "\"temp_c\":"      + String(temperature, 1) + ",";
    payload += "\"humidity\":"    + String(humidity, 1) + ",";
    payload += "\"moistureValue\":\"" + moistureLabel(moistureValue) + "\",";
    payload += "\"lightValue\":\""    + lightLabel(lightValue) + "\"";
    payload += "}";

    if (mqtt.publish(TOPIC_SENS, payload.c_str())) {
      Serial.println("Published: " + payload);
    } else {
      Serial.println("Publish failed");
    }
  }
}
