#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>

#define WIFI_SSID               "XXXXXXX"
#define WIFI_PASSWORD           "XXXXXXX"

#define RGB_LED_GREEN    12  // D7
#define RGB_LED_RED      13  // D6
#define RGB_LED_BLUE     14  // D13 (SCK)

//for http, make sure you add fingerprint (https://www.grc.com/fingerprints.htm)
#define IMPORT_IO               "https://extraction.import.io/query/extractor/XXXXXXX?_apikey=XXXXXX&url=http%3A%2F%2Fwww.knmi.nl%2Fnederland-nu%2Fweer%2Fwaarschuwingen%2Futrecht"
#define HTTPS_FINGERPRINT       "C5 C8 C8 B9 E4 EA E2 8A 34 D9 FA E7 7C BA EF 1D E7 47 C7 EF"

unsigned long lastCheckMs;

// the setup function runs once when you press reset or power the board
void setup() {
  Serial.begin(115200);
  delay(10);
  
  pinMode(RGB_LED_RED, OUTPUT);
  pinMode(RGB_LED_GREEN, OUTPUT);
  pinMode(RGB_LED_BLUE, OUTPUT);

  connectToWifi();
  fetchWeather();
}

// the loop function runs over and over again forever
void loop() {
  connectToWifi();

  if((millis() - lastCheckMs) > 1000*60*60){
    
    fetchWeather();
  }
}

void connectToWifi(){
  int attemptCount = 0;
  if(WiFi.status() != WL_CONNECTED){
      WiFi.mode(WIFI_STA);
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
      Serial.print("[Wifi] verbinden met " + String(WIFI_SSID));
    
      // Wachten tot wifi verbonden is
      while (WiFi.status() != WL_CONNECTED && attemptCount < 100) {
        Serial.print(".");
        delay(100);
        attemptCount++;
      }

      if(WiFi.status() == WL_CONNECTED){
        IPAddress myIp = WiFi.localIP();
        char myIpString[24];
        sprintf(myIpString, "%d.%d.%d.%d", myIp[0], myIp[1], myIp[2], myIp[3]);  //parse IP to readable String
        
        Serial.println("[Wifi] verbonden!");
        Serial.print("[Wifi] IP: ");
        Serial.println((char*)myIpString);
      }
  }
}

void fetchWeather(){
  if(WiFi.status() == WL_CONNECTED){  ////als we zijn verbonden
    
    String url = IMPORT_IO;
    Serial.println("[HTTP] Open connection to: " + url);
     
    HTTPClient http;
    int httpCode;

      http.begin(url,HTTPS_FINGERPRINT);
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
               JsonObject& jsonObject = jsonRoot["extractorData"];

               String text = jsonObject["data"][0]["group"][0]["Link"][0]["text"];   
               
               Serial.println("[JSON] text: " + text);
               
               //set the LEDS
               textToColor(text);
               
               //set the time of the last succesful check
               lastCheckMs = millis();
               
              }else{
               Serial.println("JSON parsing failed!");
              }
          }else{
              Serial.println("[HTTP] Ooops: HTTP " + httpCode);
          }
      } else {
          Serial.printf("[HTTP] GET... mistlukt, error: %s\n", http.errorToString(httpCode).c_str());
      }
      http.end();
  }else{
    Serial.println("No wifi");
  }
}

void textToColor(String text){
    //reset all LEDS
    digitalWrite(RGB_LED_RED, LOW);
    digitalWrite(RGB_LED_BLUE, LOW);
    digitalWrite(RGB_LED_GREEN, LOW);
    
   //parse Dutch String to color
   if(text == "Code groen") {
    digitalWrite(RGB_LED_GREEN, HIGH);
   }else if(text == "Code geel") {
    digitalWrite(RGB_LED_BLUE, HIGH);
    digitalWrite(RGB_LED_GREEN, HIGH);
   }else if(text == "Code oranje") {
    digitalWrite(RGB_LED_RED, HIGH);
    digitalWrite(RGB_LED_GREEN, HIGH);
   }else if(text == "Code rood") {
    digitalWrite(RGB_LED_RED, HIGH);
   }
}
