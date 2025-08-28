#include <Arduino.h>
#include <WiFi.h>
#include "pp_state.h"
#include "pp_topics.h"
#include "pp_auto.h"      // autoEnabled, soil_ema, soilLow/High, autoMaxMs

// локални хелпъри (идентични на твоите)
static String lightLabel(int v){
  if (v > 3000) return "Dark";
  else if (v > 2000) return "Dim";
  else if (v > 800)  return "Light";
  else if (v > 40)   return "Bright";
  else               return "Very Bright";
}
static String moistureLabel(int v){
  if (v >= 4000) return "Sensor Dry / Disconnected";
  else if (v > 3500) return "Very Dry";
  else if (v > 2800) return "Dry";
  else if (v > 2200) return "Moist";
  else if (v > 1800) return "Wet";
  else return "Very Wet";
}

void publishSensors(){
  // използва глобалните стойности от pp_state.h и pp_auto.h
  String payload = "{";
  payload += "\"temp_c\":"      + String(temperature, 1) + ",";
  payload += "\"humidity\":"    + String(humidity, 1) + ",";
  payload += "\"soil_raw\":"    + String(moistureValue) + ",";
  payload += "\"light_raw\":"   + String(lightValue) + ",";
  payload += "\"moistureValue\":\"" + moistureLabel(moistureValue) + "\",";
  payload += "\"lightValue\":\""    + lightLabel(lightValue) + "\",";
  payload += "\"rssi\":"        + String(WiFi.RSSI()) + ",";
  payload += "\"ts_unix\":"     + String((uint32_t)time(nullptr)) + ",";
  payload += "\"auto\":"        + String(autoEnabled ? 1 : 0) + ",";
  payload += "\"soil_ema\":"    + String(soil_ema, 1) + ",";
  payload += "\"soilLow\":"     + String(soilLow) + ",";
  payload += "\"soilHigh\":"    + String(soilHigh) + ",";
  payload += "\"max_ms\":"      + String(autoMaxMs);
  payload += "}";

  if (!mqtt.publish(TOPIC_SENS, payload.c_str())) {
    Serial.println("Publish sensors FAILED");
  } else {
    Serial.println("Published sensors: " + payload);
  }
}
