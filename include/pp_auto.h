#pragma once
void pumpOn();
void pumpOff();
void onMqtt(char* topic, byte* payload, unsigned int len);
void publishCfg();
void autoTick();

extern bool  autoEnabled;
extern int   soilLow, soilHigh;
extern unsigned long autoMaxMs;
extern float soil_ema;