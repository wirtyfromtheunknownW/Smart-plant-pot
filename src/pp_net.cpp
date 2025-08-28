#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "pp_state.h"
#include "pp_topics.h"
#include "pp_net.h"
#include "pp_auto.h"      // за onMqtt()
#include "tls_ca.h"

// extern void onMqtt(char* topic, byte* payload, unsigned int len);

// Взимаме тайните и MQTT от main.cpp
extern const char* WIFI_SSID;
extern const char* WIFI_PASS;
extern const char* WIFI_SSID_backup;
extern const char* WIFI_PASS_backup;
extern const char* MQTT_HOST;
extern const int   MQTT_PORT;
extern const char* MQTT_USER;
extern const char* MQTT_PASS;

static unsigned long nextWifiRetry = 0;
static unsigned long wifiBackoffMs = 1000;
static unsigned long nextMqttRetry = 0;
static unsigned long mqttBackoffMs = 1000;
static bool usingBackup = false;

void wifiStartAttempt() {
  WiFi.mode(WIFI_STA);
  WiFi.persistent(false);
  WiFi.setAutoReconnect(false);
  const char* ssid = usingBackup ? WIFI_SSID_backup : WIFI_SSID;
  const char* pass = usingBackup ? WIFI_PASS_backup : WIFI_PASS;
  Serial.printf("WiFi begin -> %s\n", ssid);
  WiFi.begin(ssid, pass);
}

void onWifiEvent(WiFiEvent_t event) {
  switch(event) {
    case SYSTEM_EVENT_STA_GOT_IP:
      Serial.printf("WiFi IP: %s\n", WiFi.localIP().toString().c_str());
      wifiBackoffMs = 1000;
      nextMqttRetry = 0; mqttBackoffMs = 1000;
      break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
      Serial.println("WiFi disconnected");
      nextWifiRetry = millis() + wifiBackoffMs;
      wifiBackoffMs = min<unsigned long>(wifiBackoffMs * 2, 60000);
      usingBackup = !usingBackup; // редуване primary/backup
      break;
    default: break;
  }
}

void wifiTick() {
  if (WiFi.status() == WL_CONNECTED) return;
  if ((long)(millis() - nextWifiRetry) >= 0) {
    wifiStartAttempt();
    nextWifiRetry = millis() + wifiBackoffMs;
  }
}

void mqttEnsure() {
  if (WiFi.status() != WL_CONNECTED) return;
  if (mqtt.connected()) return;
  if ((long)(millis() - nextMqttRetry) < 0) return;

  // уникален clientId
  char cid[48];
  uint32_t chip = (uint32_t)ESP.getEfuseMac();
  snprintf(cid, sizeof(cid), "esp32-plantpot-%06X", chip & 0xFFFFFF);

  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setKeepAlive(10);
  mqtt.setCallback(onMqtt);
  net.setCACert(TLS_CA_PEM);

  bool ok = mqtt.connect(cid, MQTT_USER, MQTT_PASS,
                         TOPIC_STATUS, 1, true, "offline");
  if (ok) {
    mqtt.publish(TOPIC_STATUS, "online", true);
    mqtt.subscribe(TOPIC_CMD);
    mqttBackoffMs = 1000;
    nextMqttRetry = 0;
    Serial.println("MQTT connected");
  } else {
    Serial.printf("MQTT rc=%d\n", mqtt.state());
    nextMqttRetry = millis() + mqttBackoffMs;
    mqttBackoffMs = min<unsigned long>(mqttBackoffMs * 2, 60000);
  }
}
