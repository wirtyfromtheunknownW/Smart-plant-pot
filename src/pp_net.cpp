#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "pp_state.h"
#include "pp_topics.h"
#include "pp_net.h"
#include "pp_auto.h"      // за onMqtt()
#include "tls_ca.h"
#include <esp_wifi.h>

#ifndef WIFI_COUNTRY
#define WIFI_COUNTRY "BG"
#endif

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

#if defined(ARDUINO_EVENT_WIFI_STA_GOT_IP)
  #define EVT_GOT_IP   ARDUINO_EVENT_WIFI_STA_GOT_IP
  #define EVT_DISCONN  ARDUINO_EVENT_WIFI_STA_DISCONNECTED
#elif defined(IP_EVENT_STA_GOT_IP) && defined(WIFI_EVENT_STA_DISCONNECTED)
  #define EVT_GOT_IP   IP_EVENT_STA_GOT_IP
  #define EVT_DISCONN  WIFI_EVENT_STA_DISCONNECTED
#else
  // very old cores
  #define EVT_GOT_IP   SYSTEM_EVENT_STA_GOT_IP
  #define EVT_DISCONN  SYSTEM_EVENT_STA_DISCONNECTED
#endif

extern void onMqtt(char* topic, uint8_t* payload, unsigned int length);

void wifiStartAttempt() {
  WiFi.mode(WIFI_STA);
  WiFi.persistent(false);
  WiFi.setAutoReconnect(false);
  WiFi.setHostname("esp32-plantpot");

  const char* ssid = usingBackup ? WIFI_SSID_backup : WIFI_SSID;
  const char* pass = usingBackup ? WIFI_PASS_backup : WIFI_PASS;
  Serial.printf("WiFi begin -> %s\n", ssid);
  WiFi.begin(ssid, pass);
}

void onWifiEvent(WiFiEvent_t event) {
  switch(event) {
    case IP_EVENT_STA_GOT_IP:
      Serial.printf("WiFi IP: %s\n", WiFi.localIP().toString().c_str());
      wifiBackoffMs = 1000;
      nextMqttRetry = 0; mqttBackoffMs = 1000;
      break;

    case WIFI_EVENT_STA_DISCONNECTED:
      nextWifiRetry = millis() + wifiBackoffMs;
      wifiBackoffMs = min<unsigned long>(wifiBackoffMs * 2, 60000);
      usingBackup = !usingBackup;
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

