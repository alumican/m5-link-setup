// Tap the screen to open Wi-Fi setup, then connect to the network it returns.
// Typing c on Serial cancels the setup screen.

#include <M5Unified.h>
#include <WiFi.h>

#include "m5linksetup.h"
#include "m5ui.h"

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
  drawHome();
}

void loop() {
  M5.update();
  if (M5.Touch.getDetail().wasClicked()) {
    m5linksetup::WifiOptions o;
    o.apSsid = "m5-link-setup";
    o.deviceName = "Stack-chan";
    o.cancel = [] { return Serial.available() && Serial.read() == 'c'; };
    m5linksetup::WifiCredentials c;
    if (m5linksetup::setupWifi(c, o)) {
      WiFi.begin(c.ssid.c_str(), c.pass.c_str());
      for (int i = 0; i < 100 && !WiFi.isConnected(); i++) delay(100);
      m5ui::events::add("wifi: %s", WiFi.isConnected() ? "connected" : "failed");
    }
    drawHome();
  }
  delay(10);
}
