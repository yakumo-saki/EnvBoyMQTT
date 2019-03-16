#include <MQTTClient.h>
#include <MQTT.h>


#include <BME280I2C.h>

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

#include <Wire.h>

#include <FS.h>
 
// I2C memo
// LCD = 0x3c  ADT = 0x48

ESP8266WebServer server(80);

// Wi-Fi設定保存ファイル
const char* settings = "/wifi_settings.txt";
const char* configured_file = "/config_ok.txt";

bool isNormal = true;

String product_short = "ebmq";
String product = "EnvBoyMQTT";
String product_long = product + " rev 2";

// setup時は、setup用SSID。 normal時は接続先SSID
String ssid = "";
String password = "";
String mDNS = "";
String mqttBroker = "";
String mqttName = "";

void setup()
{
  Serial.begin(115200);
  Serial.println(""); // 1行目にゴミが出るので改行しないと読めない
  Serial.println("");
  Serial.println("Starting " + product);

  // Init I2C Serial
  Wire.begin(5, 4);

  SPIFFS.begin();
  isNormal = SPIFFS.exists(configured_file);

  if (!isNormal) {
    Serial.println("Entering setup mode.");
    setup_setupmode(); 
  } else {
    Serial.println("Entering normal mode.");
    setup_normal(); 
  }
}
 
void loop() {
  if (!isNormal) {
    loop_setupmode(); 
  } else {
    loop_normal(); 
  }
}
