# m5-link-setup

Screens for joining networks by hand on M5Stack CoreS3. Tested with M5Unified 0.2.29.

- **setupWifi()**: Wi-Fi setup. Show a phone's Wi-Fi share QR to the camera, or scan the QR on screen to open a setup page on the phone.
- **parseWifiQr()**: reads the `WIFI:` text of a Wi-Fi share QR.

Connecting, saving and trying networks in order belong to the app, or to m5-link once it has them. This library only gets the credentials from the user.

## Install

```ini
lib_deps =
  https://github.com/alumican/m5-link-setup.git#v0.1.0
```

It pulls in m5-ui (screen colors and the event log) and ESP32QRCodeReader (for its quirc QR decoder).

## Wi-Fi

```cpp
#include "m5linksetup.h"

m5linksetup::WifiOptions o;
o.apSsid = "mydevice-setup";  // the setup network the phone joins
o.deviceName = "My device";   // used on the phone page
m5linksetup::WifiCredentials c;
if (m5linksetup::setupWifi(c, o)) {
  WiFi.begin(c.ssid.c_str(), c.pass.c_str());  // and save it if you like
}
// redraw your screen
```

The screen shows the camera preview on the left and a QR on the right.

- **Phone's Wi-Fi share QR**: Android shows one under Share in Wi-Fi settings; on iPhone, use the Passwords app. Hold it up to the camera.
- **QR on screen**: joins the setup network. The setup page opens by itself and lists the networks the device can see, 2.4 GHz only on ESP32-S3. The setup network's password is new each time and appears only in that QR.

Notes:

- `setupWifi()` blocks until it gets credentials, Cancel is tapped, `options.cancel()` returns true or `options.timeoutMs` passes (3 min).
- While it runs, Wi-Fi is in AP mode only. Reconnecting as a station moves the AP's channel and phones fail to join.
- It starts and stops the camera itself, on the internal I2C that M5Unified already drives. Stop your own camera use first, or set `options.useCamera = false`.
- Afterwards Wi-Fi is in STA mode with auto-reconnect on, and nothing is connected or saved.
- Texts, the page's accent color and timings are in `WifiOptions`. Screen colors come from `m5ui::theme`.
- Events such as `setup: phone joined` go to `m5ui::events`.

## Example

`examples/wifi`: tap to open the setup screen, then connect.

```sh
cd examples/wifi
pio run -t upload
```

## License

[MIT](LICENSE). The page uses Lucide icons (eye, eye-off, chevron-down; ISC).
