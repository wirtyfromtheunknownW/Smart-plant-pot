#include "DHTesp.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET -1 

int pinDHT = 15;
int pinMoisture = 2; // Pin for soil moisture sensor
int _moisture,sensor_analog,analogValue;
int pinLight = 4; // Pin for light sensor (not used in this example, but can be added later)
// int analogValue = 0; // Variable to store the analog value from the light sensor

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

  void getLightLevel(int analogValue) {
  if (analogValue > 3000) {
    display.println("Light: Dark");
  } else if (analogValue > 2000) {
    display.println("Light: Dim");
  } else if (analogValue > 800) {
    display.println("Light: Light");
  } else if (analogValue > 40) {
    display.println("Light: Bright");
  } else {
    display.println("Light: Very Bright");
  }
}

void getMoistureLevel(int moisture) {
  if (moisture < 5) {
    display.println("Moisture: Very Dry");
  } else if (moisture < 10) {
    display.println("Moisture: Dry");
  } else if (moisture < 15) {
    display.println("Moisture: Moist");
  } else if (moisture > 19) {
    display.println("Moisture: Wet");
  } else {
    display.println("Moisture: Very Wet");
  }
}

//TODO calibrate the moisture sensor by dry and wet values and use map function to get the percentage 
//_moisture = map(sensor_analog, dryValue, wetValue, 0, 100);
//_moisture = constrain(_moisture, 0, 100);

DHTesp dht;
void setup() {

  Serial.begin(115200);
  analogSetAttenuation(ADC_6db); // Set ADC attenuation to 11dB for better resolution
  adcAttachPin(pinMoisture); // Attach the moisture sensor pin to ADC

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  delay(2000);
  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  // Display static text
  display.println("plant pot stats");
  display.display(); 

  
  dht.setup(pinDHT, DHTesp::DHT11);
  
}
void loop() {

  TempAndHumidity data = dht.getTempAndHumidity();
  Serial.println("Temp: " + String(data.temperature, 1) + " C");
  Serial.println("Humidity: " + String(data.humidity, 1) + "%");
  Serial.println("---");

  sensor_analog = analogRead(pinMoisture);
  Serial.print("Analog Read Value = "); /* Print Analog Read Value on the serial window */
  Serial.println(sensor_analog);
  
  _moisture = ( 100 - ( (sensor_analog/3500.00) * 100 ) );
  Serial.print("Moisture = ");
  Serial.print(_moisture);  /* Print Temperature on the serial window */
  Serial.println("%");


  // int analogValue = analogRead(pinLight);
  // Serial.print("Analog Value = ");
  // Serial.print(analogValue);   // the raw analog reading

  analogValue = analogRead(pinLight);


  display.clearDisplay();
  display.setCursor(0,0);
  display.println("plant pot stats"); 
  display.setCursor(0,16);
  display.println("tempreture: " + String(data.temperature, 2) + "C");
  display.setCursor(0,16 + 10);
  display.println("humidity: " + String(data.humidity, 1) + "%");
  display.setCursor(0,16 + 20);
  display.println("moisture: " + String(_moisture) + "%");
  display.setCursor(0,16 + 30);
  getLightLevel(analogValue); // Call the function to get light level
  getMoistureLevel(_moisture); // Call the function to get moisture level

  display.setCursor(0,16 + 40);
  display.display(); 

  delay(1000);
}


