/**
  Based on https://github.com/topicusonderwijs/iota
  */
#include <Arduino.h>
#include <math.h>

#include <ESP8266WiFi.h>

#include <ESP8266HTTPClient.h>


#include "OneButton.h"
#include <Wire.h>
#include <LM75.h>       //meatures -55 C to +125 C
#include <FastLED.h>


/**
   Config, pas onderstaande waardes aan!
*/
#define WIFI_SSID               "***"
#define WIFI_PASSWORD           "***"
#define RGB_LED_BRIGHTNESS      4 // 0-255
#define THINKSPEAK_EP           "http://api.thingspeak.com/update.json"
#define THINKSPEAK_API_KEY      "***"

#define IFTTT_URL               "http://maker.ifttt.com"
#define IFTTT_KEY               "***";
#define IFTTT_EVENT             "button_pressed";         //https://ifttt.com/myrecipes/personal/37156363


/*
   Constanten
*/
#define BUTTON_PIN              0   // D3
#define LED_PIN                 2   // D4

#define RGB_LED_DATA_PIN        13  // D7
#define RGB_LED_CLOCK_PIN       14  // D5
#define RGB_NUM_LEDS            2
#define RGB_LED_TYPE            APA102
#define RGB_COLOR_ORDER         BGR

#define CHANGE_MODE_WINDOW_MS   30000
#define CONFIRM_MODE_WINDOW_MS  3000

/*
     Runtime variabelen
*/
OneButton button(BUTTON_PIN, true);
LM75 temp_sensor;
CRGB leds[RGB_NUM_LEDS];

String ipAddress;
long lastCheckMs = millis();

boolean recording = false;  //is recording mode on?
boolean voteMode = true;    //true means positive
long confirmationTimer = 0;

/*
   Functies om gebruik van de knop af te handelen
*/
void singleClick() {  
  boolean result;

  FastLED.setBrightness(RGB_LED_BRIGHTNESS + 64);
  
  if(voteMode){
    result = iftttMaker(ipAddress,String(millis()),"Green");
    if(result){
      setRgbLeds(CRGB::Green); //welcome another vote
      blinkLed(500);
    }
  }else{
    result = iftttMaker(ipAddress,String(millis()),"Red");
    if(result){
      setRgbLeds(CRGB::Red); //welcome another vote
      blinkLed(500);
    }
  }
  FastLED.setBrightness(RGB_LED_BRIGHTNESS);
}

void doubleClick() {
  if(millis() < CHANGE_MODE_WINDOW_MS){ //you can only change the mode within 30 seconds after the device booted
    if(confirmationTimer == 0){
      confirmationTimer = millis(); //'start' the timer
    }else if((millis() - confirmationTimer) < CONFIRM_MODE_WINDOW_MS){
      //change the vote mode
      voteMode = !voteMode;
      Serial.println("[Double] Change vote mode to: " + voteMode);
      if (voteMode) {
        setRgbLeds(CRGB::Green);
      } else {
        setRgbLeds(CRGB::Red);
      }
      confirmationTimer = 0;  //reset the timer again
    }
  }else{
    //else let the double click act as a single click
    singleClick();
  }
}

void longPress() {
  //readSensors();
  if(recording | (millis() < CHANGE_MODE_WINDOW_MS)){ //you can only change the mode within 30 seconds after the device booted
    if(recording){
      recording = false;
      
      blinkRgbLeds(CRGB::Orange,500);
      
      Serial.println("STOP recording");
    }else{
      recording = true;
      blinkRgbLeds(CRGB::Green,500);
      delay(250);
      blinkRgbLeds(CRGB::Green,500);
      
      Serial.println("START recording");
    }
  }else{
    //else let the double click act as a single click
    singleClick();
  }
}

/**
 * POST values to ThinksPeak
 * 
   POST https://api.thingspeak.com/update.json
     api_key=A92BIG444NN0DPSY
     field1=getLuminance()
     field2=getTemperature()
 */
void postSensorValues(){
  setRgbLeds(CRGB::Black);
  
  if(WiFi.status() == WL_CONNECTED){  ////als we zijn verbonden
    String postValues = "api_key=" + String(THINKSPEAK_API_KEY) + "&field1=" + String(getLuminance()) + "&field2=" + String(getTemperature());
     
    HTTPClient http;
    int httpCode;

      Serial.println("[HTTP] Open connection to: " + String(THINKSPEAK_EP)+ "?" + postValues);
      http.begin(String(THINKSPEAK_EP) + "?" + postValues);
      http.addHeader("Content-Type", "application/json");
      httpCode = http.GET();
      
      Serial.println("[HTTP] GET " + String(httpCode));
      
      if(httpCode > 0) {
          if(httpCode == HTTP_CODE_OK) {
              String httpResponse = http.getString();
              Serial.print("[HTTP] Response ");
              Serial.println(httpResponse);
              setRgbLed(1,CRGB::Green);
          }else{
              Serial.println("[HTTP] Ooops: HTTP " + httpCode);
              setRgbLed(1,CRGB::Orange);
          }
      } else {
          Serial.printf("[HTTP] GET... mistlukt, error: %s\n", http.errorToString(httpCode).c_str());
          setRgbLed(1,CRGB::Red);
      }
        
      http.end();
    
  }else{
    //else geen verbinding met wifi
    for(int i=0;i<3;i++){
      blinkRgbLeds(CRGB::Red,500);
    }
  }
  
  //reset LEDS na delay
  setRgbLeds(CRGB::Black);
}

boolean iftttMaker(String value1,String value2,String value3){
  setRgbLed(1,CRGB::Black);     //turn first RGB off, so the result can be displayed as feedback
  
  if(WiFi.status() == WL_CONNECTED){ 
    String url;
    url += IFTTT_URL;
    url += "/trigger/";
    url += IFTTT_EVENT;
    url += "/with/key/";
    url += IFTTT_KEY;

    String body;
    body += "{";
    body += "\"value1\" : \""+value1+"\"";
    body += ",\"value2\" : \""+value2+"\"";
    body += ",\"value3\" : \""+value3+"\"";
    body += "}";
 
    HTTPClient http;
    int httpCode;

      Serial.println("[HTTP] Open connection to: " + String(IFTTT_URL));
      http.begin(String(url));
      http.addHeader("Content-Type", "application/json");
      httpCode = http.POST(body);
      
      Serial.println("[HTTP] POST " + String(httpCode));

      if(httpCode > 0) {
          if(httpCode == HTTP_CODE_OK) {
              String httpResponse = http.getString();
              Serial.print("[HTTP] Response ");
              Serial.println(httpResponse);
              blinkRgbLed(1,CRGB::Green,1000);
              return true;     
          }else{
              Serial.println("[HTTP] Ooops: HTTP " + httpCode);
              blinkRgbLed(1,CRGB::Orange,1000);
          }
      } else {
          Serial.printf("[HTTP] POST... mistlukt, error: %s\n", http.errorToString(httpCode).c_str());
          for(int i=0;i<3;i++){
            blinkRgbLed(1,CRGB::Red,500);
          }
      }
        
      http.end();
    
  }else{
    //disconnected
    Serial.println("Lost connection with Wifi");
    for(int i=0;i<10;i++){
      blinkRgbLed(1,CRGB::Red,500);
    }
  }
  return false; //something whent wrong
}

/*
   Temperatuursensor
*/
float getTemperature() {
  // Sensor activeren
  temp_sensor.shutdown(false);
  delay(500);

  float temperature = temp_sensor.temp();

  // Sensor deactiveren
  temp_sensor.shutdown(true);

  return temperature;
}

/*
   Lichtsensor
*/
float getLuminance() {
  // Meet voltage op analoge input en reken dit om in het equivalente aantal lux
  float volts = analogRead(A0) * 3.3 / 1024.0;
  float amps = volts / 10000.0;  // 10k weerstand
  float microamps = amps * 1000000;
  float lux = microamps * 2.0;

  return lux;
}

/*
   Lees sensoren uit
*/
void readSensors() {
  Serial.print("Temperatuur: ");
  Serial.print(getTemperature());
  Serial.println("°C");

  Serial.print("Hoevelheid licht: ");
  Serial.print(getLuminance());
  Serial.println(" lux");
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
  if(brightness < 0 || brightness > 150){
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


/*
   Deze functie wordt eenmalig aangroepen bij opstarten
*/
void setup() {
  Serial.begin(115200);
  delay(10);
  Serial.println();

  // Knop instellen
  pinMode(BUTTON_PIN, INPUT);
  button.setClickTicks(300);
  button.setPressTicks(600);
  button.attachClick(singleClick);
  button.attachDoubleClick(doubleClick);
  button.attachLongPressStart(longPress);

  // Start I2C voor temperatuursensor
  Wire.begin();

  // Reguliere led instellen
  pinMode(LED_PIN, OUTPUT);
  setLed(false); // led uit

  // Initialiseer RGB leds
  FastLED.addLeds<RGB_LED_TYPE, RGB_LED_DATA_PIN, RGB_LED_CLOCK_PIN, RGB_COLOR_ORDER>(leds, RGB_NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(RGB_LED_BRIGHTNESS);
  setRgbLeds(CRGB::Black); // beide leds uit

  // Wifi instellen
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("[Wifi] verbinden met " + String(WIFI_SSID));

  // Wachten tot wifi verbonden is
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    blinkLed(100);
  }
  
  IPAddress myIp = WiFi.localIP();
  char myIpString[24];
  sprintf(myIpString, "%d.%d.%d.%d", myIp[0], myIp[1], myIp[2], myIp[3]);  //parse IP to readable String
  ipAddress += myIpString;
   
  Serial.println(".");
  Serial.println("[Wifi] verbonden!");
  Serial.println("[Wifi] IP: " + ipAddress);
  
  blinkLed(1000);
      
  // Lees beide sensoren uit
  readSensors();
}

/*
   Functie wordt continue aangroepen, voeg hier je eigen logica aan toe
*/
void loop() {
  // Laat knop controleren of er activiteit geweest is
  button.tick();

  
  if(!recording){
    //
    //vote loop
    //
    if(confirmationTimer != 0 && ((millis() - confirmationTimer) < CONFIRM_MODE_WINDOW_MS)){
        if((millis() - lastCheckMs) > 500){
          blinkRgbLeds(CRGB::Orange,200);
          lastCheckMs = millis();
        }
    }else if((millis() - confirmationTimer) >= CONFIRM_MODE_WINDOW_MS){
      //timer has expired, reset confirmation timer
      if (voteMode) {
        setRgbLeds(CRGB::Green);
      } else {
        setRgbLeds(CRGB::Red);
      }
      confirmationTimer = 0;
    }
  }else{
    //
    //recording loop
    //
    if((millis() - lastCheckMs) > 1000*60){
      lastCheckMs = millis();
      
      if(WiFi.status() == WL_CONNECTED){
        blinkLed(200);
      }
      
      if(recording){
        postSensorValues();
      }
    }  
  }
}//end loop()
