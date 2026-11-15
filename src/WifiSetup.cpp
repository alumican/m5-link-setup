// Wi-Fi setup screen (m5linksetup.h).
//
// Left: camera preview for a phone's Wi-Fi share QR. Right: a QR that joins the setup network, where a page
// lists the networks the device can see (2.4 GHz only on ESP32-S3).

#include <DNSServer.h>
#include <M5Unified.h>
#include <WebServer.h>
#include <WiFi.h>
#include <esp_camera.h>
#include <quirc/quirc.h>

#include "m5linksetup.h"
#include "m5ui.h"

namespace m5linksetup {

// ---- QR text ----------------------------------------------------------------

bool parseWifiQr(const String& text, WifiLink::Network& out) {
  if (!text.startsWith("WIFI:")) return false;
  WifiLink::Network c;
  String key, val;
  bool inVal = false;
  for (size_t i = 5; i < text.length(); i++) {
    char ch = text[i];
    if (ch == '\\' && i + 1 < text.length()) {  // \; \: \, \\ \" stand for the character itself
      (inVal ? val : key) += text[++i];
    } else if (!inVal && ch == ':') {
      inVal = true;
    } else if (ch == ';') {
      if (key == "S") c.ssid = val;
      if (key == "P") c.password = val;
      key = val = "";
      inVal = false;
    } else {
      (inVal ? val : key) += ch;
    }
  }
  if (c.ssid.isEmpty()) return false;
  out = c;
  return true;
}

namespace {

// ---- Camera (CoreS3 GC0308) ---------------------------------------------------

bool startCamera() {
  camera_config_t c = {};
  c.pin_pwdn = -1;
  c.pin_reset = -1;
  c.pin_xclk = -1;
  // The camera is configured over the internal I2C (pins 12/11, shared with touch and power). Reuse the driver
  // M5Unified installed: releasing it and letting the camera install its own fails when M5Unified takes it back.
  c.pin_sccb_sda = -1;
  c.pin_sccb_scl = -1;
  c.pin_d7 = 47;
  c.pin_d6 = 48;
  c.pin_d5 = 16;
  c.pin_d4 = 15;
  c.pin_d3 = 42;
  c.pin_d2 = 41;
  c.pin_d1 = 40;
  c.pin_d0 = 39;
  c.pin_vsync = 46;
  c.pin_href = 38;
  c.pin_pclk = 45;
  c.xclk_freq_hz = 20000000;
  c.ledc_timer = LEDC_TIMER_0;
  c.ledc_channel = LEDC_CHANNEL_0;
  c.pixel_format = PIXFORMAT_GRAYSCALE;  // QR needs no color
  c.frame_size = FRAMESIZE_QVGA;
  c.fb_count = 2;
  c.fb_location = CAMERA_FB_IN_PSRAM;
  c.grab_mode = CAMERA_GRAB_LATEST;
  c.sccb_i2c_port = M5.In_I2C.getPort();

  esp_err_t err = esp_camera_init(&c);
  m5ui::events::add("setup: camera %s", esp_err_to_name(err));
  return err == ESP_OK;
}

// ---- Setup network and page ---------------------------------------------------

DNSServer dns;
WebServer web(80);
const WifiOptions* opt = nullptr;
String apPass;
String options;  // <option>s of the visible networks
bool gotFromWeb = false;
WifiLink::Network webCreds;

String htmlEscape(const String& s) {
  String out;
  for (char ch : s) {
    if (ch == '<') out += "&lt;";
    else if (ch == '>') out += "&gt;";
    else if (ch == '&') out += "&amp;";
    else if (ch == '\'') out += "&#39;";
    else if (ch == '"') out += "&quot;";
    else out += ch;
  }
  return out;
}

String pageHead() {
  char accent[8];
  snprintf(accent, sizeof(accent), "#%06lx", (unsigned long)(opt->pageAccent & 0xFFFFFF));
  return String(
             "<!doctype html><html><head><meta charset='utf-8'>"
             "<meta name='viewport' content='width=device-width,initial-scale=1'><title>") +
         htmlEscape(opt->title) +
         "</title><style>"
         "body{font-family:-apple-system,sans-serif;margin:24px auto;padding:0 20px;max-width:420px;color:#222}"
         "h1{font-size:24px;margin:0 0 8px}h2{font-size:16px;margin:24px 0 8px}p{line-height:1.5;color:#555;margin:0}"
         "select,input{display:block;font-size:17px;width:100%;margin:0 0 8px;padding:10px;box-sizing:border-box;"
         "border:1px solid #ccc;border-radius:8px;background:#fff}"
         // Hide the native arrow, which sits at the very edge, and draw Lucide's chevron-down 12 px from the right.
         "select{-webkit-appearance:none;appearance:none;padding-right:42px;background:#fff no-repeat right 12px "
         "center/20px url(\"data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24' "
         "fill='none' stroke='%23666' stroke-width='2' stroke-linecap='round' stroke-linejoin='round'%3E%3Cpath "
         "d='m6 9 6 6 6-6'/%3E%3C/svg%3E\")}"
         ".go{text-align:center;margin-top:24px}"
         "button{font-size:17px;padding:10px 28px;border:0;border-radius:20px;background:" +
         accent +
         ";color:#fff}"
         // The margin belongs to .pw so the eye button, top:0;bottom:0, lines up with the input.
         ".pw{position:relative;margin:0 0 8px}.pw input{margin:0;padding-right:48px}"
         ".pw button{position:absolute;top:0;bottom:0;right:0;width:46px;padding:0;border-radius:8px;"
         "background:none;color:#666;display:flex;align-items:center;justify-content:center}"
         "</style></head><body>";
}

// Show/hide password icons: Lucide eye and eye-off (https://lucide.dev, ISC).
#define SVG_OPEN                                                                                      \
  "<svg xmlns='http://www.w3.org/2000/svg' width='22' height='22' viewBox='0 0 24 24' fill='none' " \
  "stroke='currentColor' stroke-width='2' stroke-linecap='round' stroke-linejoin='round'>"
#define SVG_EYE                                                                                                \
  SVG_OPEN                                                                                                     \
  "<path d='M2.062 12.348a1 1 0 0 1 0-.696 10.75 10.75 0 0 1 19.876 0 1 1 0 0 1 0 .696 10.75 10.75 0 0 1-19.876 " \
  "0'/><circle cx='12' cy='12' r='3'/></svg>"
#define SVG_EYE_OFF                                                                                          \
  SVG_OPEN                                                                                                   \
  "<path d='M10.733 5.076a10.744 10.744 0 0 1 11.205 6.575 1 1 0 0 1 0 .696 10.747 10.747 0 0 1-1.444 2.49'/>" \
  "<path d='M14.084 14.158a3 3 0 0 1-4.242-4.242'/>"                                                         \
  "<path d='M17.479 17.499a10.75 10.75 0 0 1-15.417-5.151 1 1 0 0 1 0-.696 10.75 10.75 0 0 1 4.446-5.143'/>"  \
  "<path d='m2 2 20 20'/></svg>"

// Lists the n scanned networks, strongest first, each name once.
void collectNetworks(int n) {
  options = "";
  String seen = "\n";
  for (int k = 0; k < n; k++) {
    int best = -1;
    for (int i = 0; i < n; i++) {
      String s = WiFi.SSID(i);
      if (s.isEmpty() || seen.indexOf("\n" + s + "\n") >= 0) continue;
      if (best < 0 || WiFi.RSSI(i) > WiFi.RSSI(best)) best = i;
    }
    if (best < 0) break;
    String s = WiFi.SSID(best);
    seen += s + "\n";
    options += "<option value='" + htmlEscape(s) + "'>" + htmlEscape(s) + "</option>";
  }
  WiFi.scanDelete();
  m5ui::events::add("setup: %d networks", n);
}

void handleRoot() {
  web.send(200, "text/html",
           pageHead() + "<h1>" + htmlEscape(opt->title) + "</h1><p>Connect " + htmlEscape(opt->deviceName) +
               " to a 2.4GHz Wi-Fi network.</p>"
               "<form method='post' action='/save'>"
               "<h2>SSID (2.4GHz)</h2>"
               "<select name='ssid'>" +
               options +
               "</select>"
               "<input name='other' placeholder='or type the name' autocomplete='off' autocapitalize='off'>"
               "<h2>Password</h2>"
               "<div class='pw'><input id='pw' name='pass' type='password' autocomplete='off' autocapitalize='off'>"
               "<button type='button' id='eye' aria-label='Show password' onclick='eye()'>" SVG_EYE
               "</button></div>"
               "<div class='go'><button type='submit'>Connect</button></div>"
               "</form>"
               "<script>function eye(){var p=document.getElementById('pw'),b=document.getElementById('eye'),"
               "show=p.type=='password';p.type=show?'text':'password';"
               "b.innerHTML=show?\"" SVG_EYE_OFF "\":\"" SVG_EYE
               "\";"
               "b.setAttribute('aria-label',show?'Hide password':'Show password')}</script>"
               "</body></html>");
}

void handleSave() {
  webCreds.ssid = web.arg("other").length() ? web.arg("other") : web.arg("ssid");
  webCreds.password = web.arg("pass");
  gotFromWeb = webCreds.ssid.length() > 0;
  web.send(200, "text/html",
           pageHead() + "<h1>Saved</h1><p>" + htmlEscape(opt->deviceName) + " is connecting to <b>" +
               htmlEscape(webCreds.ssid) +
               "</b>.</p>"
               "<h2>What's next</h2>"
               "<p>You can close this page. The " +
               htmlEscape(opt->apSsid) +
               " network will turn off, so switch your phone back to your usual Wi-Fi "
               "(it may do this by itself).</p>"
               "<p style='margin-top:12px'>" +
               htmlEscape(opt->retryHint) + "</p></body></html>");
}

// One radio serves both modes. Reconnecting as a station moves the AP's channel and phones fail to join, so
// stop auto-reconnect and start an async scan; startAp() follows when it completes.
void startScan() {
  static bool watched = false;
  if (!watched) {
    WiFi.onEvent([](WiFiEvent_t, WiFiEventInfo_t) { m5ui::events::add("setup: phone joined"); },
                 ARDUINO_EVENT_WIFI_AP_STACONNECTED);
    WiFi.onEvent([](WiFiEvent_t, WiFiEventInfo_t) { m5ui::events::add("setup: phone left"); },
                 ARDUINO_EVENT_WIFI_AP_STADISCONNECTED);
    watched = true;
  }
  WiFi.setAutoReconnect(false);
  WiFi.disconnect(true);
  WiFi.mode(WIFI_STA);
  WiFi.scanNetworks(true);
}

// AP only while setting up.
void startAp(int n) {
  static bool routed = false;
  if (!routed) {
    web.on("/", HTTP_GET, handleRoot);
    web.on("/save", HTTP_POST, handleSave);
    web.onNotFound(handleRoot);  // every URL opens the page, so phones open it by themselves on joining
    routed = true;
  }
  collectNetworks(n);
  gotFromWeb = false;
  apPass = String(random(10000000, 99999999));  // new each time, shown only in the QR
  WiFi.mode(WIFI_AP);
  bool ok = WiFi.softAP(opt->apSsid, apPass.c_str(), opt->apChannel);
  m5ui::events::add("setup: ap %s %s ch%d", ok ? "ok" : "FAILED", WiFi.softAPIP().toString().c_str(),
                    opt->apChannel);
  dns.start(53, "*", WiFi.softAPIP());
  web.begin();
}

void stopPortal() {
  web.stop();
  dns.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
}

// ---- Screen ---------------------------------------------------------------------

// Sizes. The camera frame (1 px outside) and the QR's white border (5 px outside) share the top edge, y 51.
const int TITLE_Y = 6, LABEL_Y = 28, STATUS_Y = 188, STATUS_H = 20;
const int QR_X = 186, QR_Y = 56, QR_W = 116, QR_BORDER = 5;
const int PREVIEW_X = 8, PREVIEW_Y = QR_Y - 4, PREVIEW_W = 160, PREVIEW_H = 120;
const int CANCEL_X = 8, CANCEL_Y = 200, CANCEL_W = 304, CANCEL_H = 34, CANCEL_R = 6, CANCEL_SLOP = 8;
const uint16_t LABEL = 0xBDF7;
const uint32_t QR_LOG_MS = 2000;  // at most one "not read" log per this

void drawLoading() {
  auto& d = M5.Display;
  d.fillScreen(m5ui::theme.bg);
  d.setFont(&fonts::Font2);
  d.setTextColor(m5ui::theme.text);
  d.setTextDatum(top_center);
  d.drawString(opt->title, 160, TITLE_Y);
  d.setTextColor(m5ui::theme.sub);
  d.setTextDatum(middle_center);
  d.drawString("Now loading...", 160, (22 + CANCEL_Y) / 2);
  d.fillRoundRect(CANCEL_X, CANCEL_Y, CANCEL_W, CANCEL_H, CANCEL_R, m5ui::theme.buttonOff);
  d.setTextColor(m5ui::theme.text);
  d.drawString("Cancel", CANCEL_X + CANCEL_W / 2, CANCEL_Y + CANCEL_H / 2);
}

void drawLoaded() {
  auto& d = M5.Display;
  d.fillRect(0, 22, 320, CANCEL_Y - 22 - 2, m5ui::theme.bg);
  d.setFont(&fonts::Font2);
  d.setTextColor(LABEL);
  d.setTextDatum(top_center);
  if (opt->useCamera) {
    d.drawString("Show Wi-Fi QR here", PREVIEW_X + PREVIEW_W / 2, LABEL_Y);
    d.drawRect(PREVIEW_X - 1, PREVIEW_Y - 1, PREVIEW_W + 2, PREVIEW_H + 2, m5ui::theme.line);
  }
  d.drawString("Or scan to set up", QR_X + QR_W / 2, LABEL_Y);
  d.fillRect(QR_X - QR_BORDER, QR_Y - QR_BORDER, QR_W + QR_BORDER * 2, QR_W + QR_BORDER * 2, TFT_WHITE);
  d.qrcode(String("WIFI:T:WPA;S:") + opt->apSsid + ";P:" + apPass + ";;", QR_X, QR_Y, QR_W, 3);
}

void drawStatus(const String& text, uint16_t color) {
  auto& d = M5.Display;
  d.fillRect(0, STATUS_Y - STATUS_H / 2, 320, STATUS_H, m5ui::theme.bg);
  d.setFont(&fonts::Font2);
  d.setTextColor(color);
  d.setTextDatum(middle_center);
  d.drawString(text, 160, STATUS_Y);
}

// Half size; mirrored if options.mirrorPreview.
void drawPreview(const camera_fb_t* fb, uint8_t* buf) {
  for (int y = 0; y < PREVIEW_H; y++) {
    const uint8_t* row = fb->buf + (y * 2) * fb->width;
    for (int x = 0; x < PREVIEW_W; x++) buf[y * PREVIEW_W + (opt->mirrorPreview ? PREVIEW_W - 1 - x : x)] = row[x * 2];
  }
  M5.Display.pushGrayscaleImage(PREVIEW_X, PREVIEW_Y, PREVIEW_W, PREVIEW_H, buf, lgfx::grayscale_8bit, TFT_WHITE,
                                TFT_BLACK);
}

bool cancelled() {
  M5.update();
  auto t = M5.Touch.getDetail();
  return (t.wasReleased() && t.x >= CANCEL_X && t.x < CANCEL_X + CANCEL_W && t.y >= CANCEL_Y - CANCEL_SLOP) ||
         (opt->cancel && opt->cancel());
}

// Decodes the QRs quirc found in the image it holds. Returns the first error if none decodes.
quirc_decode_error_t decodeFound(quirc* q, String& text) {
  static quirc_code code;  // about 13 KB together; overflows the 8 KB loop() stack
  static quirc_data data;
  quirc_decode_error_t first = QUIRC_SUCCESS;
  for (int i = 0; i < quirc_count(q); i++) {
    quirc_extract(q, i, &code);
    quirc_decode_error_t err = quirc_decode(&code, &data);
    if (err == QUIRC_SUCCESS) {
      text = String((const char*)data.payload);
      return QUIRC_SUCCESS;
    }
    if (first == QUIRC_SUCCESS) first = err;
  }
  return first;
}

// Returns the text of the first QR decoded. A QR seen but not decoded is tried again mirrored, as some
// cameras deliver mirrored frames and this quirc has no quirc_flip().
bool decodeQr(quirc* q, const camera_fb_t* fb, String& text) {
  static uint32_t loggedAt = 0;
  int w, h;
  uint8_t* image = quirc_begin(q, &w, &h);
  if (w != (int)fb->width || h != (int)fb->height) return false;
  memcpy(image, fb->buf, w * h);
  quirc_end(q);
  if (quirc_count(q) == 0) return false;
  quirc_decode_error_t err = decodeFound(q, text);
  if (err == QUIRC_SUCCESS) return true;

  image = quirc_begin(q, &w, &h);
  for (int y = 0; y < h; y++) {
    const uint8_t* src = fb->buf + y * w;
    uint8_t* dst = image + y * w;
    for (int x = 0; x < w; x++) dst[w - 1 - x] = src[x];
  }
  quirc_end(q);
  quirc_decode_error_t errMirrored = decodeFound(q, text);
  if (errMirrored == QUIRC_SUCCESS) {
    m5ui::events::add("setup: qr read mirrored");
    return true;
  }
  if (millis() - loggedAt > QR_LOG_MS) {
    loggedAt = millis();
    m5ui::events::add("setup: qr seen, not read (%s / mirrored %s)", quirc_strerror(err),
                      quirc_strerror(errMirrored));
  }
  return false;
}

}  // namespace

bool setupWifi(WifiLink::Network& out, const WifiOptions& options) {
  opt = &options;
  m5ui::events::add("setup: start");
  drawLoading();  // first, so the slow parts (scan, camera) happen behind "Now loading..."

  startScan();
  int found = WIFI_SCAN_RUNNING;
  uint32_t scanAt = millis();
  while ((found = WiFi.scanComplete()) == WIFI_SCAN_RUNNING && millis() - scanAt < opt->scanTimeoutMs) {
    if (cancelled()) {
      m5ui::events::add("setup: cancel (loading)");
      WiFi.scanDelete();
      WiFi.mode(WIFI_STA);
      WiFi.setAutoReconnect(true);
      return false;
    }
    delay(10);
  }
  startAp(max(found, 0));
  drawLoaded();

  bool cam = opt->useCamera && startCamera();
  quirc* q = cam ? quirc_new() : nullptr;
  uint8_t* preview = q ? (uint8_t*)ps_malloc(PREVIEW_W * PREVIEW_H) : nullptr;
  if (q && quirc_resize(q, 320, 240) < 0) {
    quirc_destroy(q);
    q = nullptr;
  }
  // The status line stays empty while waiting; the live preview shows it is working.
  if (opt->useCamera && !q) drawStatus("No camera. Use the QR on the right.", m5ui::theme.wait);

  bool ok = false;
  int phones = 0;
  uint32_t start = millis();
  while (millis() - start < opt->timeoutMs) {
    dns.processNextRequest();
    web.handleClient();
    if (gotFromWeb) {
      out = webCreds;
      ok = true;
      break;
    }

    if (q) {
      camera_fb_t* fb = esp_camera_fb_get();
      if (fb) {
        if (preview) drawPreview(fb, preview);
        String text;
        bool decoded = decodeQr(q, fb, text);
        esp_camera_fb_return(fb);
        if (decoded) {
          m5ui::events::add("setup: qr %s", text.startsWith("WIFI:") ? "WIFI:..." : text.c_str());
          if (parseWifiQr(text, out)) {
            ok = true;
            break;
          }
          drawStatus("That QR is not a Wi-Fi QR.", m5ui::theme.wait);
        }
      }
    }

    int n = WiFi.softAPgetStationNum();
    if (n != phones) {
      phones = n;
      if (n > 0) drawStatus("Phone joined. Open the page on the phone.", m5ui::theme.ok);
    }

    if (cancelled()) {
      m5ui::events::add("setup: cancel");
      break;
    }
    delay(1);
  }

  if (ok && opt->store && !opt->store->save(out.ssid, out.password)) m5ui::events::add("setup: save failed");
  if (ok) {
    drawStatus("Got " + out.ssid + ". Connecting...", m5ui::theme.ok);
    uint32_t at = millis();
    while (millis() - at < opt->gotHoldMs) {  // lets the page reply reach the phone
      web.handleClient();
      delay(10);
    }
  }
  if (q) quirc_destroy(q);
  free(preview);
  if (cam) esp_camera_deinit();
  stopPortal();
  m5ui::events::add("setup: %s", ok ? "got wifi" : "no wifi");
  opt = nullptr;
  return ok;
}

}  // namespace m5linksetup
