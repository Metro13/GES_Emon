#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include "EmonLib.h"
#include "WiFi.h"
#include <driver/adc.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

  // This is the device name 
#define DEVICE_NAME "GES-Monitor1"

// The GPIO pin
#define ADC_INPUT 34

char jsonOutput[128];

#define HOME_VOLTAGE 230.0

// connection with Backend
#define API_ENDPOINT "http://192.168.1.155:5000/api/emonitor/readings"

// Force EmonLib to use 10bit ADC resolution
#define ADC_BITS    10
#define ADC_COUNTS  (1<<ADC_BITS)
#define READINGS;

EnergyMonitor emon1;

// Wifi credentials
const char *WIFI_SSID = "METRO WIFI";
const char *WIFI_PASSWORD = "Realmetro12";

// Create instance for LCD display on address 0x27
// (16 characters, 2 lines)
LiquidCrystal_I2C lcd(0x27, 16, 2);

unsigned long lastMeasurement = 0;

void ToDisplay(double watts, double amps){
  lcd.setCursor(3, 1);

  lcd.print((int)watts);
  lcd.print("W ");
  lcd.print(amps);
  lcd.print("A    ");
}

void printIPAddress(){
  lcd.setCursor(3,0);
  lcd.print(WiFi.localIP());
}

void connectToWiFi() {
  lcd.clear();
  lcd.setCursor(3, 0);
  lcd.print("WiFi...      ");

  WiFi.mode(WIFI_STA);
  WiFi.setHostname("Esp-32 Home meter");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  // Only try 15 times to connect to the WiFi
  int retries = 0;
  while (WiFi.status() != WL_CONNECTED && retries < 15){
    delay(500);
    Serial.print(".");
    retries++;
  }
  // If we get here, print the IP address on the LCD
  printIPAddress();
}

void setup()
{
  adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_DB_11);
  analogReadResolution(10);
  Serial.begin(115200);

  lcd.init();
  lcd.backlight();

  connectToWiFi();

  // Initialize emon library calibration
  emon1.current(ADC_INPUT, 2);


 
}

void loop(){

  
  unsigned long currentMillis = millis();
  double watt;

  // taking a measurement if it has been longer that 1ms
  if(currentMillis - lastMeasurement > 1000){
    double amps = emon1.calcIrms(1480); // Calculate Irms
    watt = amps * HOME_VOLTAGE;

 
    // Update the display
    ToDisplay(watt, amps);

    lastMeasurement = millis();


  //adding readings to the API
  

  if(WiFi.status() == WL_CONNECTED){

     HTTPClient client;
     client.begin("http://192.168.1.155:5000/api/emonitor/readings");
     client.addHeader("Content-Type", "Application/json");

     StaticJsonDocument<96> doc;

     doc["DeviceID"] = DEVICE_NAME;
     JsonArray data = doc.createNestedArray("Watts");
     data.add((int)watt);

    
    serializeJson(doc, jsonOutput);
  
     int httpCode = client.POST(String(jsonOutput));

     if(httpCode > 0){
      String payload = client.getString();
      Serial.println("\nStatuscode: " + String(httpCode));
      Serial.println(payload);

      char json[500];
      payload.replace(" ", "");
      payload.replace("\n", "");
      client.end();
     }
     else{
    Serial.println(String(httpCode));
    }
  }
  else{
    Serial.println("No connection");
    }

    delay(1000);
}
}
