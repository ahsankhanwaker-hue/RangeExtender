#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <EEPROM.h>

extern "C" {
  #include "lwip/napt.h"
}

#define AP_SSID "FuckerESP"
#define AP_PASS "espbyrni"

#define ADMIN_USER "rni"
#define ADMIN_PASS "rni@@007"

#define DNS_PORT 53
#define EEPROM_SIZE 96

ESP8266WebServer server(80);
DNSServer dnsServer;

bool loggedIn = false;
bool wifiConnected = false;

unsigned long lastBytes = 0;
unsigned long speedKbps = 0;

// -------- UI --------
String panelPage(){
return R"rawliteral(
<html><head>
<script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
<style>
body{background:#000;color:#0f0;font-family:monospace}
.card{border:1px solid #0f0;padding:10px;margin:10px}
</style>
</head>
<body>

<h2>ESP Hacker Panel</h2>

<div class=card>
<button onclick="scan()">Scan WiFi</button>
<div id=list></div>
</div>

<div class=card>
<h3>Speed (kbps)</h3>
<canvas id="chart"></canvas>
</div>

<div class=card>
<h3>Connected Devices</h3>
<div id=clients>Loading...</div>
</div>

<script>
let ctx=document.getElementById('chart').getContext('2d');
let chart=new Chart(ctx,{
type:'line',
data:{labels:[],datasets:[{label:'Speed',data:[]}]}
});

setInterval(()=>{
fetch('/speed').then(r=>r.text()).then(v=>{
chart.data.labels.push('');
chart.data.datasets[0].data.push(v);
if(chart.data.labels.length>20){
chart.data.labels.shift();
chart.data.datasets[0].data.shift();
}
chart.update();
});
},2000);

setInterval(()=>{
fetch('/clients').then(r=>r.text()).then(d=>clients.innerHTML=d);
},3000);

function scan(){
fetch('/scan').then(r=>r.json()).then(d=>{
let h='';
d.forEach(n=>{
h+=`<p>${n.ssid} (${n.rssi}) <button onclick="conn('${n.ssid}')">Connect</button></p>`
});
list.innerHTML=h;
});
}

function conn(s){
let p=prompt("Password for "+s);
fetch('/connect?ssid='+s+'&pass='+p);
}
</script>

</body></html>
)rawliteral";
}

// -------- Routes --------
void handleRoot(){
  server.send(200,"text/html",panelPage());
}

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

void handleSpeed(){
  speedKbps = random(50,500); // simulated (real not possible)
  server.send(200,"text/plain",String(speedKbps));
}

void handleClients(){
  int c = WiFi.softAPgetStationNum();
  server.send(200,"text/plain","Connected: "+String(c));
}

// -------- Setup --------
void setup(){
  Serial.begin(115200);

  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(AP_SSID, AP_PASS);

  dnsServer.start(DNS_PORT,"*",WiFi.softAPIP());

  ip_napt_init(1000,10);
  ip_napt_enable_no(SOFTAP_IF,1);

  server.on("/",handleRoot);
  server.on("/scan",handleScan);
  server.on("/connect",handleConnect);
  server.on("/speed",handleSpeed);
  server.on("/clients",handleClients);

  server.begin();
}

// -------- Loop --------
void loop(){
  dnsServer.processNextRequest();
  server.handleClient();

  if(WiFi.status()==WL_CONNECTED && !wifiConnected){
    wifiConnected=true;
    dnsServer.stop();

    WiFi.setSleepMode(WIFI_NONE_SLEEP); // BOOST SPEED
    Serial.println("Optimized for speed");
  }
}
