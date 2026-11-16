# m5-link-setup

Screens for joining networks by hand on M5Stack CoreS3. Tested with M5Unified 0.2.29.

- **setupWifi()**: Wi-Fi setup. Show a phone's Wi-Fi share QR to the camera, or scan the QR on screen to open a setup page on the phone.
- **parseWifiQr()**: reads the `WIFI:` text of a Wi-Fi share QR.

Connecting, saving and trying networks in order belong to m5-link (`WifiLink`). This library only gets the network from the user.

## Install

```ini
lib_deps =
  https://github.com/alumican/m5-ui.git#v0.4.0
  https://github.com/alumican/m5-link.git#v0.4.0
  https://github.com/alumican/m5-link-setup.git#v0.4.2
```

It uses m5-link (`WifiLink`) and m5-ui (screen colors and the event log), which the app lists itself (before m5-link-setup) so that only one copy of each is built; library.json names them without a version. The versions above are the ones it was built with. It pulls in ESP32QRCodeReader (for its quirc QR decoder).

## Wi-Fi

```cpp
#include "WifiLink.h"
#include "m5linksetup.h"

WifiLink wifi("mydevice-wifi");

m5linksetup::WifiOptions o;
o.apSsid = "mydevice-setup";  // the setup network the phone joins
o.deviceName = "My device";   // used on the phone page
o.store = &wifi;              // saves the result; leave nullptr to only return it
WifiLink::Network n;
if (m5linksetup::setupWifi(n, o)) {
  wifi.connect(n);
}
// redraw your screen
```

The screen shows the camera preview on the left and a QR on the right.

- **Phone's Wi-Fi share QR**: Android shows one under Share in Wi-Fi settings; on iPhone, use the Passwords app. Hold it up to the camera.
- **QR on screen**: joins the setup network. The setup page opens by itself and lists the networks the device can see, 2.4 GHz only on ESP32-S3. The setup network's password is new each time and appears only in that QR.

Notes:

- `setupWifi()` blocks until it gets credentials, Cancel is tapped, `options.cancel()` returns true or `options.timeoutMs` passes (3 min).
- While it runs, Wi-Fi is in AP mode only. Reconnecting as a station moves the AP's channel and phones fail to join.
- The CoreS3 camera delivers mirrored frames. A QR seen but not read is tried again mirrored, so either way reads. The preview shows frames as delivered, which looks like a mirror; set `options.mirrorPreview` to flip it.
- It starts and stops the camera itself, on the internal I2C that M5Unified already drives. Stop your own camera use first, or set `options.useCamera = false`.
- Afterwards Wi-Fi is in STA mode with auto-reconnect on. Nothing is connected, and nothing is saved unless `options.store` is set.
- Texts, the page's accent color and timings are in `WifiOptions`. Screen colors come from `m5ui::theme`.
- Events such as `setup: phone joined` go to `m5ui::events`.

## Example

`examples/wifi`: joins the saved network at start; tap to open the setup screen, which saves the result, then connect.

```sh
cd examples/wifi
pio run -t upload
```

## Tests

`tests`: Unity tests that run on a connected CoreS3. They cover `parseWifiQr()`; the screens are checked by hand with the example.

```sh
cd tests
pio test                                        # build, upload and run
pio test --without-uploading --without-testing  # build only
```

## License

[MIT](LICENSE). The page uses Lucide icons (eye, eye-off, chevron-down; ISC).
