#pragma once
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

// Глобални обекти (дефинирани в main.cpp)
extern WiFiClientSecure net;
extern PubSubClient mqtt;
extern unsigned long pumpUntil;
extern const int DHTPIN;
extern const int MOISTURE_PIN;
extern const int LIGHT_PIN;
extern int   moistureValue;
extern int   lightValue;
extern float temperature;
extern float humidity;
// Пинове за помпа/LED (дефинирани в main.cpp)
extern const int PUMP_PIN;
extern const int ACT_PIN;

// Хелпери (дефинирани в main.cpp)
void ledOn();
void ledOff();
