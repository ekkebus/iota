#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>

#define WIFI_SSID               "XXXXX"
#define WIFI_PASSWORD           "XXXXX"

//apifier.com settings
#define USER_ID                 "XXXXX"
#define CRAWLER_ID              "XXXXX"
#define API_TOKEN               "XXXXX"

#define API_URL                 "https://api.apifier.com/v1/" 
//for https, make sure you add a fingerprint (create one at https://www.grc.com/fingerprints.htm)
#define API_HTTPS_FINGERPRINT   "17 67 4C 08 F1 22 7B B4 1C 6F 5B 01 22 AB 00 06 24 8E D8 6B" //*.apifier.com

#define RGB_LED_GREEN    12  // D7
#define RGB_LED_RED      13  // D6
#define RGB_LED_BLUE     14  // D13 (SCK)

//global variables
unsigned long lastCheckMs;

// the setup function runs once when you press reset or power the board
void setup() {
  Serial.begin(115200);
  Serial.println();
  delay(10);
  
  pinMode(RGB_LED_RED, OUTPUT);
  pinMode(RGB_LED_GREEN, OUTPUT);
  pinMode(RGB_LED_BLUE, OUTPUT);

  connectToWifi();
  callService();
}

// the loop function runs over and over again forever
void loop() {
  connectToWifi();

  if((millis() - lastCheckMs) > 1000*60*60){
    
    callService();
  }
}

void connectToWifi(){
  int attemptCount = 0;
  if(WiFi.status() != WL_CONNECTED){
      WiFi.mode(WIFI_STA);
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
      Serial.print("[Wifi] verbinden met " + String(WIFI_SSID));
    
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

void callService(){
  DynamicJsonBuffer jsonBuffer;
  
  String url = API_URL USER_ID "/crawlers/" CRAWLER_ID "/execute?token=" API_TOKEN ;
  String httpResponse = sendHttpRequest(url);
          
  JsonObject& crawlerResponse = jsonBuffer.parseObject(httpResponse);

  if (!crawlerResponse.success()) {
   Serial.println("JSON parsing failed!");
   return;
  }
  String resultUrl = crawlerResponse["resultsUrl"];

  delay(4000);  //crawler will be started as Asyn task, just wait for it until it is finished

  httpResponse = sendHttpRequest(resultUrl);
  
  for(int attempts=0;attempts< 5 && (httpResponse == "[]"); attempts++){//in some cases we need to be a bit more patient
    httpResponse = sendHttpRequest(resultUrl);
    delay(2000);
  }
  
  //*hack* the used Json parser does not fully support the returned Json string strip away the array
  httpResponse.remove(0,1);
  httpResponse.remove(httpResponse.length()-1,1);
  
  JsonObject& crawlerResult = jsonBuffer.parseObject(httpResponse);
  
  if (!crawlerResult.success()) {
    Serial.println("JSON parsing failed!");
    return;
  }  
   
  String weatherCode = crawlerResult["pageFunctionResult"]["weatherCode"];

  Serial.println("[JSON] weatherCode = " + weatherCode);
  
  //set the LEDS
  textToColor(weatherCode);
  
  //set the time of the last succesful check
  lastCheckMs = millis();  
}

/**
 * function that will perfor the GET request, returns a String with the contents of the Body
 */
String sendHttpRequest(String url){
 String httpResponse;
  
  if(WiFi.status() == WL_CONNECTED){  // when there is a Wifi connection
    Serial.println("[HTTP] Open connection to: " + url);
     
    HTTPClient http;
    int httpCode;

      http.begin(url,API_HTTPS_FINGERPRINT);
      http.addHeader("Content-Type", "application/json");
      httpCode = http.GET();
      
      Serial.println("[HTTP] GET " + String(httpCode));
      
      if(httpCode > 0) {
          if(httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_CREATED) {
              httpResponse = http.getString();
              
              Serial.print("[HTTP] Response size = ");
              Serial.println(http.getSize());
              //Serial.print("[HTTP] Response ");
              //Serial.println(httpResponse);
              
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
  return httpResponse;
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
