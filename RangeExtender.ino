#include <ESP8266WiFi.h>
extern "C" {
  #include "lwip/napt.h"
}

#define REPEATER_SSID "FuckerESP"
#define REPEATER_PASS "espbyrni"

// Target router credentials
#define ROUTER_SSID "YourRouterSSID"
#define ROUTER_PASS "YourRouterPassword"

void setup() {
  Serial.begin(115200);

  // Connect to main router (STA mode)
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(ROUTER_SSID, ROUTER_PASS);

  Serial.print("Connecting to router");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to router");
  Serial.print("STA IP: "); Serial.println(WiFi.localIP());

  // Start AP
  WiFi.softAP(REPEATER_SSID, REPEATER_PASS);
  Serial.print("AP started: "); Serial.println(REPEATER_SSID);
  Serial.print("AP IP: "); Serial.println(WiFi.softAPIP());

  // Enable NAT (NAPT)
  ip_napt_init(1000, 10);
  ip_napt_enable_no(SOFTAP_IF, 1);
}

void loop() {
  // Nothing else needed — NAPT handles forwarding automatically
}
