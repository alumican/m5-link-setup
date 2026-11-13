// Joins the saved Wi-Fi at start. Tap the screen to open Wi-Fi setup, which saves the result, then connect.
// Typing c on Serial cancels the setup screen.

#include <M5Unified.h>
#include <WiFi.h>

#include "WifiLink.h"
#include "m5linksetup.h"
#include "m5ui.h"

static WifiLink wifi("example-wifi");

static void drawHome() {
  M5.Display.fillScreen(m5ui::theme.bg);
  M5.Display.setFont(&fonts::Font2);
  M5.Display.setTextColor(m5ui::theme.text);
  M5.Display.setTextDatum(middle_center);
  String s = WiFi.isConnected() ? "Wi-Fi: " + WiFi.SSID() + " " + WiFi.localIP().toString() : "Wi-Fi: none";
  M5.Display.drawString(s, 160, 100);
  M5.Display.drawString("Tap to set up Wi-Fi", 160, 140);
}

void setup() {
  M5.begin();
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  wifi.connect();
  drawHome();
}

void loop() {
  M5.update();
  if (M5.Touch.getDetail().wasClicked()) {
    m5linksetup::WifiOptions o;
    o.apSsid = "m5-link-setup";
    o.deviceName = "Stack-chan";
    o.cancel = [] { return Serial.available() && Serial.read() == 'c'; };
    o.store = &wifi;
    WifiLink::Network n;
    if (m5linksetup::setupWifi(n, o)) m5ui::events::add("wifi: %s", wifi.connect(n) ? "connected" : "failed");
    drawHome();
  }
  delay(10);
}
