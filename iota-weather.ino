#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include "OneButton.h"
#include <FastLED.h>

/**
   Configuration, change XXXXXX values
*/
#define WIFI_SSID               "XXXXXX"
#define WIFI_PASSWORD           "XXXXXX"

//cnt limits the amount of returned data
#define OPENWEATHER_URL         "http://api.openweathermap.org/data/2.5/forecast/city?cnt=3&q=XXXXXX,NLD&APPID=XXXXXX"

#define BUTTON_PIN              0   // D3
#define LED_PIN                 2   // D4

#define RGB_LED_DATA_PIN        13  // D7
#define RGB_LED_CLOCK_PIN       14  // D5
#define RGB_NUM_LEDS            2
#define RGB_LED_TYPE            APA102
#define RGB_COLOR_ORDER         BGR
#define RGB_LED_BRIGHTNESS      150 // 0-255

/**
     Runtime variabelen
*/
OneButton button(BUTTON_PIN, true);
CRGB leds[RGB_NUM_LEDS];

String ipAddress;
unsigned long lastCheckMs;
unsigned long currentBrightness = RGB_LED_BRIGHTNESS; 
unsigned long currentUnixTimestamp;  //will be retrieved from the webservice call

void setup() {
  Serial.begin(115200);
  delay(10);

  // configure button
  pinMode(BUTTON_PIN, INPUT);
  button.setClickTicks(300);
  button.setPressTicks(600);
  button.attachClick(singleClick);
  button.attachDoubleClick(doubleClick);
  button.attachLongPressStart(longPress);

  // setup regular LED
  pinMode(LED_PIN, OUTPUT);
  setLed(false); // led uit

  // setup RGB leds
  FastLED.addLeds<RGB_LED_TYPE, RGB_LED_DATA_PIN, RGB_LED_CLOCK_PIN, RGB_COLOR_ORDER>(leds, RGB_NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(RGB_LED_BRIGHTNESS);
  setRgbLeds(CRGB::Black); // beide leds uit
  
  connectToWifi();
  colorDemo();
  
  blinkLed(100);
   
  fetchWeather(0);
  
}

void loop() {
  button.tick();  

  connectToWifi();
  
  if((millis() - lastCheckMs) > 1000*60*5){
    blinkLed(1000);
    fetchWeather(0);
  }
}

void connectToWifi(){
  int attemptCount = 0;
  if(WiFi.status() != WL_CONNECTED){
      WiFi.mode(WIFI_STA);
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
      Serial.print("[Wifi] connect to " + String(WIFI_SSID));
    
      // Wachten tot wifi verbonden is
      while (WiFi.status() != WL_CONNECTED && attemptCount < 100) {
        Serial.print(".");
        blinkLed(100);
        attemptCount++;
      }

      if(WiFi.status() == WL_CONNECTED){
        IPAddress myIp = WiFi.localIP();
        char myIpString[24];
        sprintf(myIpString, "%d.%d.%d.%d", myIp[0], myIp[1], myIp[2], myIp[3]);  //parse IP to readable String
        ipAddress += myIpString;
         
        Serial.println("[Wifi] connected!");
        Serial.println("[Wifi] IP: " + ipAddress);
      }
  }
}

/*
   Button methods
*/
void singleClick() {  
  fetchWeather(0);
}

void doubleClick() {
  singleClick();
}

void longPress() {
  singleClick();
}


/**
 * get json from OpenWeatherMap http://openweathermap.org/forecast5
 * 
 */
void fetchWeather(float period){
  setRgbLeds(CRGB::Black);
  Serial.println("" + WiFi.status());
  if(WiFi.status() == WL_CONNECTED){ 
    
    String url = OPENWEATHER_URL;
    Serial.println("[HTTP] Open connection to: " + url);
     
    HTTPClient http;
    int httpCode;

      http.begin(url);
      http.addHeader("Content-Type", "application/json");
      httpCode = http.GET();
      
      Serial.println("[HTTP] GET " + String(httpCode));
      
      if(httpCode > 0) {
          if(httpCode == HTTP_CODE_OK) {
              String httpResponse = http.getString();
              DynamicJsonBuffer jsonBuffer;
              
              Serial.print("[HTTP] Response ");
              Serial.println(httpResponse);
              
              JsonObject& jsonRoot = jsonBuffer.parseObject(httpResponse);

              if (jsonRoot.success()) {
               setRgbLed(1,CRGB::Green);
               JsonArray& weatherList = jsonRoot["list"];
               JsonObject& weatherObject = weatherList[period];  //[0] = now, [1] = 3 hours

               long timeOfForcasedWeather =  weatherObject["dt"]; //get the datetime UnixTimestamp
               currentUnixTimestamp = timeOfForcasedWeather;
               
               String weatherNowDescription = weatherObject["weather"][0]["description"]; 
               int weatherNowId = weatherObject["weather"][0]["id"]; //http://openweathermap.org/weather-conditions
               Serial.println(weatherNowDescription + " (" + weatherNowId + ") @ " + currentUnixTimestamp);
               
               //set the LEDS
               setRgbLeds(weatherCodeToColor(weatherNowId));
               
               //set the time of the last succesful check
               lastCheckMs = millis();

               //if it is a future forcast
               if(period > 0) setRgbLed(1,CRGB::Black);
               
              }else{
               Serial.println("JSON parsing failed!");
               setRgbLed(1,CRGB::Orange);
              }
          }else{
              Serial.println("[HTTP] Ooops: HTTP " + httpCode);
              setRgbLed(1,CRGB::Orange);
          }
      } else {
          Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
          setRgbLed(1,CRGB::Orange);
      }
        
      http.end();
  }else{
    Serial.println("No wifi");
    //else geen verbinding met wifi
    for(int i=0;i<3;i++){
       blinkLed(1000);
    }
  }
}

/*
   Reguliere led
*/
void setLed(bool state) {
  digitalWrite(LED_PIN, !state);
}

/*
   RGB leds
*/
void setRgbLeds(CRGB color) {
  for (int i = 0; i < RGB_NUM_LEDS; i++) {
    setRgbLed(i, color);
  }
}

void setRgbLed(int ledno, CRGB color) {
  leds[ledno] = color;
  FastLED.show();
}

void setRgbBrightness(int brightness){
  //0-255
  if(brightness < 0 || brightness > 255){
    //overwrite value if it is out of the boundary
    brightness = RGB_LED_BRIGHTNESS;
  }
  
  FastLED.setBrightness(brightness);
}

void blinkLed(long milliseconds){
  setLed(1);
  delay(milliseconds);
  setLed(0);
}

void blinkRgbLeds(CRGB color,long milliseconds){
  setRgbLeds(color);
  delay(milliseconds);
  setRgbLeds(CRGB::Black);
  delay(milliseconds);
}

void blinkRgbLed(int ledNumber, CRGB color,long milliseconds){
  setRgbLed(ledNumber,color);
  delay(milliseconds);
  setRgbLed(ledNumber,CRGB::Black);
  delay(milliseconds);
}

//Colors: https://github.com/FastLED/FastLED/wiki/Pixel-reference
CRGB weatherCodeToColor(int weatherCode){
   //2xx: Thunderstorm
   if(weatherCode < 300) return CRGB::FireBrick;
   //3xx: Drizzle
   else if(weatherCode < 500) return CRGB::Turquoise;
   //5xx: Rain
   else if(weatherCode < 600) return CRGB::DarkBlue;
   //6xx: Snow
   else if(weatherCode < 700) return CRGB::DarkBlue;
   //7xx: Atmosphere
   else if(weatherCode < 800) return CRGB::Green;
   //800: Clear
   else return CRGB::Green;
}

void colorDemo(){
  for(int i=100;i<900;i=i+100){
    setRgbLeds(weatherCodeToColor(i));
    delay(100);  
  }
  
  for(int i=0;i<24;i++){
    setRgbLeds(CRGB::Black);
    adjustRgbBrightness(i);
     setRgbLeds(weatherCodeToColor(900));
    delay(200);
  }
  setRgbLeds(CRGB::Black);
  setRgbBrightness(RGB_LED_BRIGHTNESS);
}
