#include <ESP8266WiFi.h>
#include <WiFiClient.h> 
#include <ESP8266WebServer.h>

/**
 * 初期化
 */
void setup_setupmode() {
  // ファイルシステム初期化
  setup_server();
}

void loop_setupmode() {
  server.handleClient();
}

/**
 * WiFi設定
 */
void handleRootGet() {
  String html = "";
  html += "<html>";
  html += "<head>";
  html += "<title>" + product + " setting</title>";
  html += "<meta charset='UTF-8'>";
  html += "<style>";
  html += "  input { width:200px; }";
  html += "</style>";
  html += "</head>";
  html += "<h1>" + product + " Settings</h1>";
  html += "<form method='post'>";
  html += "  <br>";
  html += "  接続先WiFiの情報です。面倒ですが正しく設定して下さい。<br>";
  html += "  2.4GHz帯のみ対応しています。<br>";
  html += "  <input type='text' name='ssid' placeholder='ssid' autofocus><br>";
  html += "  <input type='text' name='pass' placeholder='pass'><br>";
  html += "  mDNS(Rendezvous) の名前です。LAN内の他の端末等と重複しないようにして下さい。<br>";
  html += "  ハイフン、アンダースコアを使用すると名前解決に失敗する可能性があるため非推奨です。<br>";
  html += "  <input type='text' name='mdnsname' placeholder='mdnsname' value='" + ssid + "'><br>";
  html += "  <br>";
  html += "  MQTTブローカーのIPアドレスです。ホスト名も可能ですが、mDNSは使用出来ません。<br>";
  html += "  <input type='text' name='mqttbroker' placeholder='mqttbroker' value='""'><br>";
  html += "  <br>";
  html += "  MQTT名です。クライアント名とトピック名に使用されます。<br>";
  html += "  <input type='text' name='mqttname' placeholder='mqttname' value='""'><br>";
  html += "  <br>";
  html += "  <input type='submit' value='設定する'><br>";
  html += "</form>";
  html += "<br>";
  html += "<hr>";
  html += product_long + ", Copyright 2018-2019 ziomatrix.org.";
  html += "</html>";

  server.send(200, "text/html", html);
}

void handleRootPost() {
  String ssid = server.arg("ssid");
  String pass = server.arg("pass");
  String mdnsname = server.arg("mdnsname");
  String mqttbroker = server.arg("mqttbroker");
  String mqttname = server.arg("mqttname");

  SPIFFS.begin();

  // 設定ファイル
  File f = SPIFFS.open(settings, "w");
  f.println(ssid);
  f.println(pass);
  f.println(mdnsname);
  f.println(mqttbroker);
  f.println(mqttname); 
  f.close();

  // 設定済みフラグファイル
  File f2 = SPIFFS.open(configured_file, "w");
  f2.println("ok");
  f2.close();

  String html = "";
  html += "<html>";
  html += "<head>";
  html += "<title>" + product + " setting done. please restart.</title>";
  html += "<meta charset='UTF-8'>";
  html += "<style>";
  html += "  input { width:200px; }";
  html += "</style>";
  html += "</head>";

  html += "<h1>" + product + " setting done</h1>";
  html += "SSID " + ssid + "<br>";
  html += "PASS " + pass + "<br>";
  html += "mDNS " + mdnsname + "<br>";
  html += "MQTT broker " + mqttbroker + "<br>";
  html += "MQTT name   " + mqttname + "<br>";
  html += "<br>";
  html += "Restart (power off then power on) to use above setting.<br>";
  html += "設定が完了しました。リセットボタンを押して再起動して下さい。<br>";
  html += "<br>";
  html += "<a href='/'>setting again</a>";
  html += "</html>";
  server.send(200, "text/html", html);
}

/**
 * 初期化(サーバモード)
 */
void setup_server() {
  byte mac[6];
  WiFi.macAddress(mac);

  // SSID は macaddress をSUFFIXする
  ssid = product_short;
  for (int i = 0; i < 6; i++) {
    ssid += String(mac[i], HEX);
  }
  
  Serial.println("SSID: " + ssid);
  // Serial.println("PASS: " + pass);

  /* You can remove the password parameter if you want the AP to be open. */
  // WiFi.softAP(ssid.c_str(), pass.c_str());
  WiFi.softAP(ssid.c_str());

  server.on("/", HTTP_GET, handleRootGet);
  server.on("/", HTTP_POST, handleRootPost);
  server.begin();
  Serial.println("HTTP server started.");
}
