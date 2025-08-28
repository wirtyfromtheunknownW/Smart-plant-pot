#pragma once
extern int   moistureValue;
extern int   lightValue;
extern float temperature;
extern float humidity;

void sensorsInit();
void sensorsRead();