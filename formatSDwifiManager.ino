#include <WiFiManager.h>
#include<FS.h>
#include<SPIFFS.h>
void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  SPIFFS.begin();
  delay(1000);
  Serial.println("Beggining format");
  if(SPIFFS.format()){
    Serial.println("Format complete");
  }
  else{
    Serial.println("unable to Format");
  }
  WiFiManager wifiManager;
  wifiManager.resetSettings();
}

void loop() {
  // put your main code here, to run repeatedly:

}
