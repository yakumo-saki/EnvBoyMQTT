#include <Tsl2561.h>
#include <MHZ19_uart.h>

// using sodaq_lps22hb ver 1.0.0
#include <Sodaq_LPS22HB.h>

WiFiClient net;
MQTTClient mqttClient;

const int mhz_rx_pin = 12;  //Serial rx pin no
const int mhz_tx_pin = 14; //Serial tx pin no

MHZ19_uart mhz19;

Sodaq_LPS22HB lps22hb;
bool useLPS22HB = false;

int counter;

BME280I2C bme;   // Default : forced mode, standby time = 1000 ms
                 // Oversampling = pressure ×1, temperature ×1, humidity ×1, filter off,
                 // Address 0x76

Tsl2561 Tsl(Wire);

// 最後に取得した温度
float lastTemp;
float lastHumidity;
float lastPressure;
float lastLuxFull;
float lastLuxIr;
int lastPpm;

void readConfig() {
  // 設定を読んで
  File f = SPIFFS.open(settings, "r");
  ssid = f.readStringUntil('\n');
  password = f.readStringUntil('\n');
  mDNS = f.readStringUntil('\n');  
  mqttBroker = f.readStringUntil('\n');  
  mqttName = f.readStringUntil('\n');  
  f.close();

  ssid.trim();
  password.trim();
  mDNS.trim();
  mqttBroker.trim();
  mqttName.trim();

  Serial.println("SSID: " + ssid);
  Serial.println("PASS: " + password);
  Serial.println("mDNS: " + mDNS);
  Serial.println("MQTT Broker: " + mqttBroker);
  Serial.println("MQTT Name  : " + mqttName);  
}

void initializeWiFi() {
  WiFi.begin(ssid.c_str(), password.c_str());

  int retryCount = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    retryCount++;
    if (retryCount % 10 == 0) {
      Serial.println("");      
    }
  }

  Serial.println("");
  Serial.println("WiFi connected");

  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  setup_ota(mDNS);
  Serial.println("Arduino OTA Ready");

  Serial.println("MQTT Connect");
  mqttClient.begin(mqttBroker.c_str(), net);

  Serial.println("MQTT Begin");
  while (!mqttClient.connect(mDNS.c_str(), "", "")) { // username and password not support
    Serial.print(".");
    delay(1000);
  }

  Serial.println("MQTT Started successfully.");

}

void setup_normal() {
  readConfig();
  
  // setupモードに入りやすくするための処理
  Serial.println("==== Reset to reconfig start.");
  String filename = configured_file;
  filename = filename + "tmp";
  SPIFFS.rename(configured_file, filename);

  WiFi.softAPdisconnect(true);
  WiFi.enableAP(false);

  initializeWiFi();

  // init BME
  while(!bme.begin()){
    Serial.println("Could not find BME280 sensor!");
    delay(1000);
  }
  delay(500);

  // init LPS22HB if found (0x5d)
  if (lps22hb.init()) {
      Serial.println("Barometric sensor initialization succeeded!");
      lps22hb.enableSensor(Sodaq_LPS22HB::OdrOneShot);
      useLPS22HB = true;
  } else {
      useLPS22HB = false;
      Serial.println("Barometric sensor initialization failed!");
  }

  // init TSL2561
  Tsl.begin();
  if( !Tsl.available() ) {
    Serial.println("No Tsl2561 found. Tsl2561 disabled.");
  }

  // init LPS22HB if found (0x5d)
  if (lps22hb.init()) {
      Serial.println("Barometric sensor initialization succeeded!");
      lps22hb.enableSensor(Sodaq_LPS22HB::OdrOneShot);
      useLPS22HB = true;
  } else {
      useLPS22HB = false;
      Serial.println("Barometric sensor initialization failed!");
  }

  // init MH-Z19B
  mhz19.begin(mhz_rx_pin, mhz_tx_pin);
  mhz19.setAutoCalibration(false);

  int wait = 0;
  while( mhz19.isWarming() ) {
    Serial.print(wait);
    Serial.print(" MH-Z19 now warming up...  status:");
    Serial.println(mhz19.getStatus());
    delay(1000);
    wait++;
    if (wait > 3) {
      Serial.println("warmup timeout continue anyway");      
      break;
    }
  }

  // reconfig timeout
  SPIFFS.rename(filename, configured_file);
  Serial.println("=== reconfigure timeout. continue.");

}

void loop_normal() {

  // WiFiが繋がってなければ意味がないので接続チェック
  checkAndReconnectWifi();

  loop_ota();

  // MQTT
  mqttClient.loop();
  delay(10);  // <- fixes some issues with WiFi stability

  readData();

  delay(10000);
}

void mqttPublish(String topic, String value) {
  mqttClient.publish("/" + mqttName + "/" + topic, value);
}

void readData() {

  if (true) {
    readDataBme280();

    if (useLPS22HB) {
      readDataLPS22HB();
    }
  
    // MQTT
    char buf[24] = "";
    if (lastTemp != NAN) {
      sprintf(buf, "%2f", lastTemp);
      mqttPublish("temp", buf);
    }
    
    if (lastHumidity != NAN) {
      sprintf(buf, "%2f", lastHumidity);
      mqttPublish("humi", buf);
    }
  
    if (lastPressure != NAN) {
      sprintf(buf, "%2f", lastPressure);
      mqttPublish("pres", buf);
    }
  } else {
    lastTemp = 0;
    lastHumidity = 0;
    lastPressure = 0;    
  }

  if (Tsl.available()) {
    readDataTsl2561();
  
    // MQTT
    char buf[24] = "";
    sprintf(buf, "%2f", lastLuxFull);
    mqttPublish("lux", buf);
  
    sprintf(buf, "%2f", lastLuxIr);
    mqttPublish("luxIr", buf);
  } else {
    lastLuxFull = 0;
    lastLuxIr = 0;
  }

  if (true) {
    lastPpm = 0;
    lastPpm = mhz19.getPPM();
    Serial.printf("MH-Z19B: PPM=");
    Serial.print(lastPpm);
    Serial.print("\n");

    // MQTT: if ppm == -1 , MH-Z19 error.
    if (lastPpm > 0) {
      char buf[24] = "";
      sprintf(buf, "%2f", lastPpm);
      mqttPublish("ppm", buf);
    }
  }
  
}

void readDataLPS22HB() {
  float tempPres(NAN);
  tempPres = lps22hb.readPressureHPA();
  if (tempPres != 0) {
    lastPressure = tempPres;      
    Serial.print("Pressure (by LPS22HB): ");
    Serial.print(tempPres, 2);
    Serial.println("hPa");
  }  
}

void readDataBme280() {
  float temp(NAN), hum(NAN), pres(NAN);

  // unit: B000 = Pa, B001 = hPa, B010 = Hg, B011 = atm, B100 = bar, B101 = torr, B110 = N/m^2, B111 = psi
  BME280::TempUnit tempUnit(BME280::TempUnit_Celsius);
  BME280::PresUnit presUnit(BME280::PresUnit_Pa);
   
  // Parameters: (float& pressure, float& temp, float& humidity, bool celsius = false, uint8_t pressureUnit = 0x0)
  bme.read(pres, temp, hum, tempUnit, presUnit); 

  pres = pres / 100;
  
  Serial.print("BME280: Temp: ");
  Serial.print(temp, 2);
  Serial.print("C");
  Serial.print("\t\tHumidity: ");
  Serial.print(hum, 2);
  Serial.print("%");
  Serial.print("\t\tPressure: ");
  Serial.print(pres, 2);
  Serial.println("hPa");

  lastTemp = temp;
  lastHumidity = hum;
  lastPressure = pres;

}

void readDataTsl2561() {
  uint8_t id;
  uint16_t full = 0;
  uint16_t ir = 0;

  Tsl.on();

  Tsl.setSensitivity(false, Tsl2561::EXP_402);
  //delay(16);
  //delay(110);
  delay(402 + 10);

  Tsl.id(id);
  Tsl.fullLuminosity(full);
  Tsl.irLuminosity(ir);

  Serial.printf("TSL2561: at 0x%02x(id=0x%02x) luminosity is %5u (full) and %5u (ir)\n", Tsl.address(), id, full, ir);

  lastLuxFull = full;
  lastLuxIr = ir;

  Tsl.off(); 
}

void checkAndReconnectWifi() {
  if (WiFi.status() == WL_CONNECTED) {
    return;
  }

  Serial.print("WiFi is down. restart");
  WiFi.disconnect();

  int retryCount = 0;
  WiFi.begin(ssid.c_str(), password.c_str());
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
    retryCount++;
    if (retryCount % 15 == 0) {
      retryCount = 0;
      WiFi.disconnect();   
      WiFi.begin(ssid.c_str(), password.c_str());
      Serial.println("");
      Serial.print("Still reconnecting WiFi");
    }
  }

  Serial.println("");
  Serial.println("WiFi reconnected.");

}
