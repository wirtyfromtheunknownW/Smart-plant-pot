#include <DHT.h>
#include "pp_sensors.h"
#include "pp_state.h"
#include "pp_topics.h"
#include "pp_net.h"
#include "pp_auto.h"
#include "pp_diag.h"
#include "pp_publish.h"


// DHTTYPE остава макро от DHT.h -> не пипай
const int DHTPIN       = 2;
const int MOISTURE_PIN = 0;
const int LIGHT_PIN    = 1;
const int PUMP_PIN     = 5;
const int ACT_PIN      = 15; // onboard LED for emulation

const char* WIFI_SSID = "A1_A3AEFA";
const char* WIFI_PASS = "430af6a4";

const char* WIFI_SSID_backup = "Wirty";
const char* WIFI_PASS_backup = "123456789";

const char* MQTT_HOST = "4629ff2d3b384c748f95b499f10e4f5b.s1.eu.hivemq.cloud";
const int   MQTT_PORT = 8883; // TLS (Transport Layer Security)
const char* MQTT_USER = "hivemq.webclient.1755195003530";
const char* MQTT_PASS = "8354ECQbTRgAeafh:%!$";


const char* TOPIC_SENS = "plantpot/sensors";
const char* TOPIC_CMD  = "plantpot/pump"; ///cmd
const char* TOPIC_ACK  = "plantpot/pump/ack";
const char* TOPIC_STATUS = "plantpot/status";
const char* TOPIC_DIAG = "plantpot/diag";
const char* TOPIC_CFG  = "plantpot/cfg";

const bool LED_ACTIVE_HIGH = false; // ако не светва, смени на true

void ledOn()  { digitalWrite(ACT_PIN,LED_ACTIVE_HIGH ? HIGH : LOW); }
void ledOff() { digitalWrite(ACT_PIN, LED_ACTIVE_HIGH ? LOW  : HIGH);  }

// inline void ledOn()  { digitalWrite(ACT_PIN, LED_ACTIVE_HIGH ? HIGH : LOW); }
// inline void ledOff() { digitalWrite(ACT_PIN, LED_ACTIVE_HIGH ? LOW  : HIGH); }


WiFiClientSecure net;
PubSubClient mqtt(net);
unsigned long pumpUntil = 0;

int moistureValue = 0;
int lightValue = 0;
float temperature = 0;
float humidity = 0;

unsigned long lastMoistureLightRead = 0;
unsigned long lastDHTRead = 0;

static unsigned long nextWifiRetry = 0;
static unsigned long wifiBackoffMs = 1000;   // старт 1 s, кап 60 s
static unsigned long nextMqttRetry = 0;
static unsigned long mqttBackoffMs = 1000;   // старт 1 s, кап 60 s
static bool usingBackup = false;


static const float SOIL_ALPHA = 0.20f; // EMA

String jsonEscape(const String& s) { // minimal escaper
  String out; out.reserve(s.length()+4);
  for (char c: s) { if (c=='"'||c=='\\') out += '\\'; out += c; }
  return out;
}

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


  // Connect to Wi-Fi
//  static bool wifiTry(const char* ssid, const char* pass, uint32_t timeoutMs){
//   WiFi.mode(WIFI_STA);
//   WiFi.begin(ssid, pass);
//   uint32_t t0 = millis();
//   while (WiFi.status() != WL_CONNECTED && (millis() - t0) < timeoutMs) {
//     delay(250);
//   }
//   return WiFi.status() == WL_CONNECTED;
// }

// void wifiConnect(){
//   // преминаваме към неблокираща, събитийна логика; не прави нищо тук
// }



int clampi(int v,int lo,int hi){ return v<lo?lo:(v>hi?hi:v); }


void sensorsRead();

void setup() {
  Serial.begin(115200);
  sensorsInit();
  pinMode(PUMP_PIN, OUTPUT); digitalWrite(PUMP_PIN, LOW);
  pinMode(ACT_PIN,  OUTPUT); ledOff();

  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);
  analogSetPinAttenuation(MOISTURE_PIN, ADC_11db);


  // WiFi.onEvent(onWifiEvent);
  WiFi.begin(WIFI_SSID_backup, WIFI_PASS_backup);
  // analogReadResolution(12);
  // analogSetAttenuation(ADC_11db);

  usingBackup = false;     // започни с primary
  nextWifiRetry = 0;       // позволи незабавен опит
  // wifiTick();              // kick

  // MQTT ще се свърже през mqttEnsure() в loop()
// END REPLACE setup_header
}

void loop() {
  
  // wifiTick();
  mqttEnsure();
  if (mqtt.connected()) mqtt.loop();

  // auto-OFF every loop, precise timing
  if (pumpUntil && (long)(millis() - pumpUntil) >= 0) {
  // pumpOff();
  ledOff();
  pumpUntil = 0;
  mqtt.publish(TOPIC_ACK, "{\"cmd\":\"AUTO_OFF\",\"result\":\"ok\"}", false);
}


  static unsigned long t0 = 0;
  if (millis() - t0 >= 10000) {
    t0 = millis();

    sensorsRead();

    // moistureValue = analogRead(MOISTURE_PIN);
    if (isnan(soil_ema)) soil_ema = moistureValue;
    else soil_ema = 0.20f * moistureValue + 0.80f * soil_ema;
    // lightValue = analogRead(LIGHT_PIN);

      // === AUTO режим ===
  // Включи ако СУХО (soil_ema >= soilLow) и не помпи в момента
  if (autoEnabled && pumpUntil == 0 && soil_ema >= soilLow) {
    unsigned long ms = autoMaxMs;  // може да направим и импулси по-къси по-късно
    pumpOn();
    pumpUntil = millis() + ms;
    mqtt.publish(TOPIC_ACK, "{\"cmd\":\"AUTO_START\",\"result\":\"ok\"}", false);
  }

  // Ранно спиране ако вече е Мокро (soil_ema <= soilHigh)
  if (autoEnabled && pumpUntil != 0 && soil_ema <= soilHigh) {
    pumpOff();
    pumpUntil = 0;
    mqtt.publish(TOPIC_ACK, "{\"cmd\":\"AUTO_STOP\",\"result\":\"ok\"}", false);
  }
    publishSensors();

    static unsigned long tDiag = 0;
    if (millis() - tDiag >= 60000UL) {
      tDiag = millis();
      if (mqtt.connected()) publishDiag();
    }
    Serial.printf("soil_raw=%d  soil_ema=%.1f  light_raw=%d\n",
              moistureValue, soil_ema, lightValue);
  }
  autoTick();
}
