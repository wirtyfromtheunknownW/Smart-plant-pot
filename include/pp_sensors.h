#pragma once
extern const int DHTPIN;
extern const int MOISTURE_PIN;
extern const int LIGHT_PIN;
extern const int PUMP_PIN;
extern const int ACT_PIN;

// // Pin mapping for Seeed Studio XIAO ESP32-C6
// static const int DHTPIN       = 2;   // D1 pin
// static const int MOISTURE_PIN = 0;   // A0 pin
// static const int LIGHT_PIN    = 1;   // A1 pin
// static const int PUMP_PIN     = 5;   // D5 pin
// static const int ACT_PIN      = 21;  // D10 pin (LED)

// optional future use
static const int SDA_PIN      = 4;   // D4
static const int SCL_PIN      = 6;   // D6

void sensorsInit();
void sensorsRead();