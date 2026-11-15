#pragma once
// m5-link-setup: screens for joining networks by hand on M5Stack CoreS3. See README.md.

#include <Arduino.h>

#include "WifiLink.h"

namespace m5linksetup {

// Texts, colors and timings for setupWifi(). The screen itself uses m5ui::theme.
struct WifiOptions {
  const char* apSsid = "m5-setup";        // the setup network the phone joins
  const char* deviceName = "the device";  // used on the phone page: "Connect <deviceName> to ..."
  const char* title = "Wi-Fi setup";      // screen heading
  const char* retryHint = "If it can't connect, start Wi-Fi setup again.";  // shown on the phone after saving
  uint32_t pageAccent = 0x0076d1;         // phone page button color, RGB888
  uint8_t apChannel = 6;
  uint32_t timeoutMs = 3UL * 60UL * 1000UL;
  uint32_t scanTimeoutMs = 10000;
  uint32_t gotHoldMs = 1500;              // keeps "Got <ssid>" on screen before returning
  bool useCamera = true;
  bool mirrorPreview = false;             // flips the camera preview left to right; reading tries both
  bool (*cancel)() = nullptr;             // polled; return true to stop, e.g. from Serial
  WifiLink* store = nullptr;              // if set, the result is saved there with WifiLink::save()
};

// Parses "WIFI:T:WPA;S:<ssid>;P:<pass>;;", the format phones use to share Wi-Fi.
bool parseWifiQr(const String& text, WifiLink::Network& out);

// Blocks while showing the Wi-Fi setup screen. The user either shows a phone's Wi-Fi share QR to the camera,
// or scans the QR on screen to join the setup network and picks a network on the page that opens.
// Returns true with the network; false if cancelled or timed out. Saves it to options.store if set, and does
// not connect; the app does, e.g. with WifiLink::connect().
// Starts and stops the camera itself, so the camera must not be in use. Leaves Wi-Fi in STA mode with
// auto-reconnect on. Redraw your screen afterwards.
bool setupWifi(WifiLink::Network& out, const WifiOptions& options = WifiOptions());

}  // namespace m5linksetup
