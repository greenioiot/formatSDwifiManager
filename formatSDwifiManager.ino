#include <WiFiManager.h>
#include<FS.h>
#include<SPIFFS.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>

#define HOSTNAME "TAT"
#define PASSWORD "7650"

TaskHandle_t Task1;
String deviceToken = "";
uint64_t chipId = 0;
String wifiName = "@WC";
bool shouldSaveConfig = false;
struct Settings
{
  char TOKEN[40] = "";
  char SERVER[40] = "mqttservice.smartfarmpro.com";
  int PORT = 1883;
  char MODE[60] = "Farm/Cloud/Device/Mode2";
  uint32_t ip;
} sett;
void getChipID() {
    chipId = ESP.getEfuseMac(); //The chip ID is essentially its MAC address(length: 6 bytes).
    Serial.printf("ESP32ChipID=%04X", (uint16_t)(chipId >> 32)); //print High 2 bytes
    Serial.printf("%08X\n", (uint32_t)chipId); //print Low 4bytes.

  }
String uint64ToString(uint64_t input) {
    String result = "";
    uint8_t base = 10;

    do {
      char c = input % base;
      input /= base;

      if (c < 10)
        c += '0';
      else
        c += 'A' - 10;
      result = c + result;
    } while (input);
    return result;
  }
 void setupOTA()
  {
    //Port defaults to 8266
    //ArduinoOTA.setPort(8266);

    //Hostname defaults to esp8266-[ChipID]
    ArduinoOTA.setHostname(uint64ToString(chipId).c_str());

    //No authentication by default
    ArduinoOTA.setPassword(PASSWORD);

    //Password can be set with it's md5 value as well
    //MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
    //ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

    ArduinoOTA.onStart([]()
    {
      Serial.println("Start Updating....");

      Serial.printf("Start Updating....Type:%s\n", (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem");
    });

    ArduinoOTA.onEnd([]()
    {

      Serial.println("Update Complete!");

      ESP.restart();
    });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
    {
      String pro = String(progress / (total / 100)) + "%";
      int progressbar = (progress / (total / 100));
      //int progressbar = (progress / 5) % 100;
      //int pro = progress / (total / 100);


      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    });

    ArduinoOTA.onError([](ota_error_t error)
    {
      Serial.printf("Error[%u]: ", error);
      String info = "Error Info:";
      switch (error)
      {
        case OTA_AUTH_ERROR:
          info += "Auth Failed";
          Serial.println("Auth Failed");
          break;

        case OTA_BEGIN_ERROR:
          info += "Begin Failed";
          Serial.println("Begin Failed");
          break;

        case OTA_CONNECT_ERROR:
          info += "Connect Failed";
          Serial.println("Connect Failed");
          break;

        case OTA_RECEIVE_ERROR:
          info += "Receive Failed";
          Serial.println("Receive Failed");
          break;

        case OTA_END_ERROR:
          info += "End Failed";
          Serial.println("End Failed");
          break;
      }


      Serial.println(info);
      ESP.restart();
    });

    ArduinoOTA.begin();
  }

 void setupWIFI()
  {
    WiFi.setHostname(uint64ToString(chipId).c_str());

    byte count = 0;
    while (WiFi.status() != WL_CONNECTED && count < 10)
    {
      count ++;
      delay(500);
      Serial.print(".");
    }


    if (WiFi.status() == WL_CONNECTED)
      Serial.println("Connecting...OK.");
    else
      Serial.println("Connecting...Failed");
  }

  void configModeCallback (WiFiManager * myWiFiManager) {
    Serial.println("Entered config mode");
    Serial.println(WiFi.softAPIP());
    //if you used auto generated SSID, print it
    Serial.println(myWiFiManager->getConfigPortalSSID());
  }
  
  void saveConfigCallback () {
    Serial.println("Should save config");
    shouldSaveConfig = true;
    Serial.print("saveConfigCallback:");
    Serial.println(sett.TOKEN);
  }

class IPAddressParameter : public WiFiManagerParameter
  {
    public:
      IPAddressParameter(const char *id, const char *placeholder, IPAddress address)
        : WiFiManagerParameter("")
      {
        init(id, placeholder, address.toString().c_str(), 16, "", WFM_LABEL_BEFORE);
      }

      bool getValue(IPAddress &ip)
      {
        return ip.fromString(WiFiManagerParameter::getValue());
      }
  };

  class IntParameter : public WiFiManagerParameter
  {
    public:
      IntParameter(const char *id, const char *placeholder, long value, const uint8_t length = 10)
        : WiFiManagerParameter("")
      {
        init(id, placeholder, String(value).c_str(), length, "", WFM_LABEL_BEFORE);
      }

      long getValue()
      {
        return String(WiFiManagerParameter::getValue()).toInt();
      }
  };
char baudrate[8];
  char dpsbits[5];
  char program[10];
  char mode_select[3];
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
  getChipID();
  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonDocument jsonBuffer(1024);
        deserializeJson(jsonBuffer, buf.get());
        serializeJson(jsonBuffer, Serial);
        if (!jsonBuffer.isNull()) {
          Serial.println("\nparsed json");
          //strcpy(output, json["output"]);
          if (jsonBuffer.containsKey("baudrate")) strcpy(baudrate, jsonBuffer["baudrate"]);
          if (jsonBuffer.containsKey("dpsbits")) strcpy(dpsbits, jsonBuffer["dpsbits"]);
          if (jsonBuffer.containsKey("program")) strcpy(program, jsonBuffer["program"]);
          if (jsonBuffer.containsKey("mode_select")) strcpy(mode_select, jsonBuffer["mode_select"]);
          if (jsonBuffer.containsKey("token")) strcpy(sett.TOKEN, jsonBuffer["token"]);
          if (jsonBuffer.containsKey("server")) strcpy(sett.SERVER, jsonBuffer["server"]);
          if (jsonBuffer.containsKey("port")) sett.PORT = jsonBuffer["port"];
        } else {
          Serial.println("failed to load json config");
        }
      }
    }
  } else {
    Serial.println("failed to mount FS");
  }
  WiFiManagerParameter baudrate_param("baudrate", "Serial Port : Baudrate", baudrate, 8);
    WiFiManagerParameter dpsbits_param("databits", "Serial Port : Data Bits, Parity Bits, Stop Bits", dpsbits, 5);
    WiFiManagerParameter program_param("program", "Program", program, 10);
    WiFiManagerParameter mode_param("mode", "Mode (1-10)", mode_select, 3);
    wifiManager.setTimeout(120);
    wifiManager.setAPCallback(configModeCallback);
    std::vector<const char *> menu = {"wifi", "info", "sep", "restart", "exit"};
    wifiManager.setMenu(menu);
    wifiManager.setClass("invert");
    wifiManager.setConfigPortalTimeout(120); // auto close configportal after n seconds
    wifiManager.setAPClientCheck(true); // avoid timeout if client connected to softap
    wifiManager.setBreakAfterConfig(true);   // always exit configportal even if wifi save fails

    WiFiManagerParameter blnk_Text("<b>Device setup.</b> <br>");
    sett.TOKEN[39] = '\0';   //add null terminator at the end cause overflow
    sett.SERVER[39] = '\0';   //add null terminator at the end cause overflow
    WiFiManagerParameter blynk_Token( "blynktoken", "device Token",  sett.TOKEN, 40);
    WiFiManagerParameter blynk_Server( "blynkserver", "Server",  sett.SERVER, 40);
    IntParameter blynk_Port( "blynkport", "Port",  sett.PORT);
    wifiManager.addParameter( &blnk_Text );
    wifiManager.addParameter( &blynk_Token );
    wifiManager.addParameter( &blynk_Server );
    wifiManager.addParameter( &blynk_Port );
    wifiManager.addParameter(&baudrate_param);
    wifiManager.addParameter(&dpsbits_param);
    wifiManager.addParameter(&program_param);
    wifiManager.addParameter(&mode_param);


    //set config save notify callback
    wifiManager.setSaveConfigCallback(saveConfigCallback);




    wifiName.concat(uint64ToString(chipId));
    if (!wifiManager.autoConnect(wifiName.c_str())) {
      Serial.println("failed to connect and hit timeout");


    }
    deviceToken = wifiName.c_str();
    if (baudrate_param.getValue() != "") strcpy(baudrate, baudrate_param.getValue());
    if (dpsbits_param.getValue() != "") strcpy(dpsbits, dpsbits_param.getValue());
    if (program_param.getValue() != "") strcpy(program, program_param.getValue());
    if (mode_param.getValue() != "") strcpy(mode_select, mode_param.getValue());
    if (blynk_Token.getValue() != "") strcpy(sett.TOKEN, blynk_Token.getValue());
    if (blynk_Server.getValue() != "") strcpy(sett.SERVER, blynk_Server.getValue());
    if (blynk_Port.getValue() != 0) sett.PORT =  blynk_Port.getValue();
    Serial.println("saving config");
    DynamicJsonDocument jsonBuffer(1024);
    jsonBuffer["baudrate"] = baudrate;
    jsonBuffer["dpsbits"] = dpsbits;
    jsonBuffer["program"] = program;
    jsonBuffer["mode_select"] = mode_select;
    jsonBuffer["token"] = sett.TOKEN;
    jsonBuffer["server"] = sett.SERVER;
    jsonBuffer["port"] = sett.PORT;
    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
    }

    serializeJson(jsonBuffer, Serial);
    serializeJson(jsonBuffer, configFile);
    configFile.close();
    setupWIFI();
    setupOTA();
}

void loop() {
  // put your main code here, to run repeatedly:
  ArduinoOTA.handle();
}
