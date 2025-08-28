#include <Arduino.h>
#include "pp_state.h"
#include "pp_topics.h"

void publishDiag() {
  unsigned long up = millis() / 1000UL;
  String ip = (WiFi.status() == WL_CONNECTED) ? WiFi.localIP().toString() : String("-");
  int rssi = (WiFi.status() == WL_CONNECTED) ? WiFi.RSSI() : 0;

  String j = "{";
  j += "\"uptime_s\":" + String(up) + ",";
  j += "\"free_heap\":" + String(ESP.getFreeHeap()) + ",";
  j += "\"wifi_ok\":" + String(WiFi.status()==WL_CONNECTED ? 1 : 0) + ",";
  j += "\"mqtt_ok\":" + String(mqtt.connected() ? 1 : 0) + ",";
  j += "\"rssi\":" + String(rssi) + ",";
  j += "\"ip\":\"" + ip + "\"";
  j += "}";

  mqtt.publish(TOPIC_DIAG, j.c_str(), false);
}
