// #include <Arduino.h>
// #include "pp_state.h"
// #include "pp_topics.h"
// #include "pp_auto.h"
// #include <PubSubClient.h>

// // Авто-настройки (дефиниции)
// bool  autoEnabled = false;
// int   soilLow = 2800;
// int   soilHigh = 2200;
// unsigned long autoMaxMs = 30000UL;
// float soil_ema = NAN;

// void publishCfg() {
//   String j = "{";
//   j += "\"auto\":" + String(autoEnabled ? 1 : 0) + ",";
//   j += "\"soilLow\":" + String(soilLow) + ",";
//   j += "\"soilHigh\":" + String(soilHigh) + ",";
//   j += "\"max_ms\":" + String(autoMaxMs);
//   j += "}";
//   mqtt.publish(TOPIC_CFG, j.c_str(), true);
// }

// void pumpOn()  { digitalWrite(PUMP_PIN, HIGH); ledOn();  }
// void pumpOff() { digitalWrite(PUMP_PIN, LOW);  ledOff(); }

// void onMqtt(char* topic, byte* payload, unsigned int len){
//   String t(topic), msg; msg.reserve(len);
//   for (unsigned i=0;i<len;i++) msg += (char)payload[i];
//   msg.trim();

//   if (t != TOPIC_CMD) return;

//   String result = "ok";
//   String up = msg; up.toUpperCase();

//   if (up.startsWith("ON")) {
//     int ms = 5000;
//     int sp = up.indexOf(' ');
//     if (sp > 0) {
//       int sec = up.substring(sp+1).toInt();
//       ms = constrain(sec*1000, 500, 600000);
//     }
//     pumpOn();
//     pumpUntil = millis() + ms;
//   } else if (up == "OFF") {
//     pumpOff();
//     pumpUntil = 0;
//   } else if (up == "AUTO ON") {
//     autoEnabled = true;  publishCfg();
//   } else if (up == "AUTO OFF") {
//     autoEnabled = false; publishCfg();
//   } else if (up.startsWith("SET ")) {
//     if (up.indexOf("LOW ") == 4)       { soilLow  = constrain(up.substring(8).toInt(), 100, 4095); publishCfg(); }
//     else if (up.indexOf("HIGH ") == 4) { soilHigh = constrain(up.substring(9).toInt(), 50, soilLow-1); publishCfg(); }
//     else if (up.indexOf("MAXMS ") == 4){ autoMaxMs = (unsigned long)constrain(up.substring(10).toInt(), 500, 600000); publishCfg(); }
//     else { result = "bad_set"; }
//   } else if (up == "STATUS") {
//     publishCfg();
//   } else {
//     result = "bad_cmd";
//   }

//   String ack = String("{\"cmd\":\"")+msg+"\",\"result\":\""+result+"\"}";
//   mqtt.publish(TOPIC_ACK, ack.c_str(), false);
// }

// void autoTick() {
//   // Ранно стоп по хистерезис
//   if (autoEnabled && pumpUntil != 0 && soil_ema <= soilHigh) {
//     pumpOff();
//     pumpUntil = 0;
//     mqtt.publish(TOPIC_ACK, "{\"cmd\":\"AUTO_STOP\",\"result\":\"ok\"}", false);
//   }
//   // Стартиране при сухо
//   if (autoEnabled && pumpUntil == 0 && soil_ema >= soilLow) {
//     pumpOn();
//     pumpUntil = millis() + autoMaxMs;
//     mqtt.publish(TOPIC_ACK, "{\"cmd\":\"AUTO_START\",\"result\":\"ok\"}", false);
//   }
// }

#include <Arduino.h>
#include <PubSubClient.h>
#include "pp_state.h"
#include "pp_topics.h"
#include "pp_auto.h"

// дефиниции
bool  autoEnabled = false;
int   soilLow = 2800;
int   soilHigh = 2200;
unsigned long autoMaxMs = 30000UL;
float soil_ema = NAN;

void publishCfg() {
  String j = "{";
  j += "\"auto\":" + String(autoEnabled ? 1 : 0) + ",";
  j += "\"soilLow\":" + String(soilLow) + ",";
  j += "\"soilHigh\":" + String(soilHigh) + ",";
  j += "\"max_ms\":" + String(autoMaxMs);
  j += "}";
  mqtt.publish(TOPIC_CFG, j.c_str(), true);
}

void pumpOn()  { digitalWrite(PUMP_PIN, HIGH); ledOn();  }
void pumpOff() { digitalWrite(PUMP_PIN, LOW);  ledOff(); }

void onMqtt(char* topic, byte* payload, unsigned int len){
  String t(topic), msg; msg.reserve(len);
  for (unsigned i=0;i<len;i++) msg += (char)payload[i];
  msg.trim();
  if (t != TOPIC_CMD) return;

  String result = "ok";
  String up = msg; up.toUpperCase();

  if (up.startsWith("ON")) {
    int ms = 5000; int sp = up.indexOf(' ');
    if (sp > 0) { int sec = up.substring(sp+1).toInt(); ms = constrain(sec*1000, 500, 600000); }
    pumpOn(); pumpUntil = millis() + ms;
  } else if (up == "OFF") {
    pumpOff(); pumpUntil = 0;
  } else if (up == "AUTO ON") {
    autoEnabled = true;  publishCfg();
  } else if (up == "AUTO OFF") {
    autoEnabled = false; publishCfg();
  } else if (up.startsWith("SET ")) {
    if (up.indexOf("LOW ") == 4)       { soilLow  = constrain(up.substring(8).toInt(), 100, 4095); publishCfg(); }
    else if (up.indexOf("HIGH ") == 4) { soilHigh = constrain(up.substring(9).toInt(), 50, soilLow-1); publishCfg(); }
    else if (up.indexOf("MAXMS ") == 4){ autoMaxMs = (unsigned long)constrain(up.substring(10).toInt(), 500, 600000); publishCfg(); }
    else { result = "bad_set"; }
  } else if (up == "STATUS") {
    publishCfg();
  } else {
    result = "bad_cmd";
  }

  String ack = String("{\"cmd\":\"")+msg+"\",\"result\":\""+result+"\"}";
  mqtt.publish(TOPIC_ACK, ack.c_str(), false);
}

void autoTick() {
  if (autoEnabled && pumpUntil != 0 && soil_ema <= soilHigh) {
    pumpOff(); pumpUntil = 0;
    mqtt.publish(TOPIC_ACK, "{\"cmd\":\"AUTO_STOP\",\"result\":\"ok\"}", false);
  }
  if (autoEnabled && pumpUntil == 0 && soil_ema >= soilLow) {
    pumpOn(); pumpUntil = millis() + autoMaxMs;
    mqtt.publish(TOPIC_ACK, "{\"cmd\":\"AUTO_START\",\"result\":\"ok\"}", false);
  }
}
