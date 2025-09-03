#include <Arduino.h>
#include <DHT.h>
#include "pp_state.h"
#include "pp_auto.h"     // за soil_ema
#include "pp_sensors.h"

// === Константи за калибрация и изглаждане ===
static const float TEMP_SCALE    = 1.0f;
static const float TEMP_OFFSET_C = 0.0f;  // твоята температурна корекция
static const float HUM_SCALE     = 1.0f;
static const float HUM_OFFSET    = 0.0f;  // твоята влажностна корекция
static const float EMA_ALPHA_TH  = 0.30f;  // EMA за T/H
static const float SOIL_ALPHA    = 0.20f;  // EMA за почва (същото като в авто)

// const int DHTPIN       = D1;
// const int MOISTURE_PIN = A0;  
// const int LIGHT_PIN    = A1;  
// const int PUMP_PIN     = D5;
// const int ACT_PIN      = D10;

// === Пинове за DHT – дефинирани са в main.cpp като макроси ===
#ifndef DHTPIN
#define DHTPIN 2
#endif
#ifndef DHTTYPE
#define DHTTYPE DHT22
#endif

// Локални EMA акумулатори за T/H
static float t_ema = NAN, h_ema = NAN;

// Локален DHT обект
static DHT dht(DHTPIN, DHTTYPE);

// void sensorsInit() {
//   dht.begin();
//   analogReadResolution(12);
//   analogSetAttenuation(ADC_11db);
// }

void sensorsInit() {
  dht.begin();
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);
  analogSetPinAttenuation(MOISTURE_PIN, ADC_11db);
  analogSetPinAttenuation(LIGHT_PIN,    ADC_11db);
  pinMode(MOISTURE_PIN, INPUT);
  pinMode(LIGHT_PIN,    INPUT);
}

void sensorsRead() {
  // RAW ADC
  moistureValue = analogRead(MOISTURE_PIN);
  lightValue    = analogRead(LIGHT_PIN);

  
  // EMA за почвата, стойността е глобална (в pp_auto.h)
  if (isnan(soil_ema)) soil_ema = moistureValue;
  else soil_ema = SOIL_ALPHA * moistureValue + (1.0f - SOIL_ALPHA) * soil_ema;

  // DHT с EMA и калибрация
  float h_raw = dht.readHumidity();
  float t_raw = dht.readTemperature();
  if (!isnan(h_raw) && !isnan(t_raw)) {
    float t_corr = t_raw * TEMP_SCALE + TEMP_OFFSET_C;
    float h_corr = constrain(h_raw * HUM_SCALE + HUM_OFFSET, 0.0f, 100.0f);

    if (isnan(t_ema)) { t_ema = t_corr; h_ema = h_corr; }
    else {
      t_ema = EMA_ALPHA_TH * t_corr + (1.0f - EMA_ALPHA_TH) * t_ema;
      h_ema = EMA_ALPHA_TH * h_corr + (1.0f - EMA_ALPHA_TH) * h_ema;
    }
    temperature = t_ema;
    humidity    = h_ema;
  }
}
