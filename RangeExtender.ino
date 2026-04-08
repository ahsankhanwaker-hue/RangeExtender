#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <WiFiUdp.h>
#include <EEPROM.h>

extern "C" {
  #include "lwip/napt.h"
}

#define AP_SSID "FuckerESP"
#define AP_PASS "espbyrni"

#define DNS_PORT 53

ESP8266WebServer server(80);
DNSServer dnsServer;
WiFiUDP udp;

bool wifiConnected = false;

// -------- DNS Proxy --------
IPAddress dnsServerIP(8,8,8,8);
WiFiUDP dnsUDP;

void handleDNSProxy() {
  int packetSize = dnsUDP.parsePacket();
  if (packetSize) {
    byte packet[512];
    dnsUDP.read(packet, 512);

    dnsUDP.beginPacket(dnsServerIP, 53);
    dnsUDP.write(packet, packetSize);
    dnsUDP.endPacket();

    delay(10);

    int len = dnsUDP.parsePacket();
    if (len) {
      dnsUDP.read(packet, 512);
      dnsUDP.beginPacket(dnsUDP.remoteIP(), dnsUDP.remotePort());
      dnsUDP.write(packet, len);
      dnsUDP.endPacket();
    }
  }
}

// -------- Web UI --------
String panel(){
return R"rawliteral(
<html><body style="background:black;color:lime;font-family:monospace">
<h2>ESP PRO Panel</h2>
<button onclick="scan()">Scan WiFi</button>
<div id=list></div>

<script>
function scan(){
fetch('/scan').then(r=>r.json()).then(d=>{
let h='';
d.forEach(n=>{
h+=`<p>${n.ssid} (${n.rssi}) 
<button onclick="c('${n.ssid}')">Connect</button></p>`
});
list.innerHTML=h;
});
}
function c(s){
let p=prompt("Password:");
fetch('/connect?ssid='+s+'&pass='+p);
}
</script>
</body></html>
)rawliteral";
}

// -------- Routes --------
void handleRoot(){ server.send(200,"text/html",panel()); }

void handleScan(){
  int n=WiFi.scanNetworks();
  String json="[";
  for(int i=0;i<n;i++){
    if(i) json+=",";
    json+="{\"ssid\":\""+WiFi.SSID(i)+"\",\"rssi\":"+String(WiFi.RSSI(i))+"}";
  }
  json+="]";
  server.send(200,"application/json",json);
}

void handleConnect(){
  WiFi.begin(server.arg("ssid").c_str(), server.arg("pass").c_str());
  server.send(200,"text/plain","Connecting...");
}

// -------- Setup --------
void setup(){
  Serial.begin(115200);

  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(AP_SSID, AP_PASS);

  // Captive portal only for setup
  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());

  // Enable NAT
  ip_napt_init(1000,10);
  ip_napt_enable_no(SOFTAP_IF,1);

  dnsUDP.begin(53);

  server.on("/",handleRoot);
  server.on("/scan",handleScan);
  server.on("/connect",handleConnect);

  server.begin();
}

// -------- Loop --------
void loop(){
  server.handleClient();

  if(WiFi.status() != WL_CONNECTED){
    dnsServer.processNextRequest(); // captive portal mode
  }

  if(WiFi.status() == WL_CONNECTED){
    if(!wifiConnected){
      wifiConnected = true;

      dnsServer.stop(); // stop fake DNS
      WiFi.setSleepMode(WIFI_NONE_SLEEP);

      Serial.println("Connected → PRO DNS active");
    }

    handleDNSProxy(); // 🔥 real DNS forwarding
  }
}
