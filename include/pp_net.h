#pragma once
#include <WiFi.h>
#pragma once
extern const char* WIFI_SSID;
extern const char* WIFI_PASS;
extern const char* WIFI_SSID_backup;
extern const char* WIFI_PASS_backup;

extern const char* MQTT_HOST;
extern const int   MQTT_PORT;
extern const char* MQTT_USER;
extern const char* MQTT_PASS;

void wifiStartAttempt();
void onWifiEvent(WiFiEvent_t event);
void wifiTick();
void mqttEnsure();