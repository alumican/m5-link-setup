# TODO

In order.

## 1. Check on a device

- [x] Setup page on a CoreS3: scan the screen QR, pick a network, save to WifiLink, connect.
- [x] Phone share QR (iPhone Passwords app) on a CoreS3: read after retrying mirrored.
- [ ] Cancel, timeout, no camera.
- [x] Verify that `lib_deps = https://github.com/alumican/m5-link-setup.git#v0.1.0` builds in PlatformIO.

## 2. Tokens

- [ ] Move the screen's sizes and the label color into tokens when m5-ui has them.

## 3. BLE pairing

Once m5-link has BLE (HID keyboard, serial):

- [ ] Pairing screen: the name the host sees and, if asked, the passkey to confirm.
- [ ] Paired hosts: list and forget, as Panel content or a popup.
- [ ] State: advertising, connected, disconnected.

## Out of scope

- Transports, connecting, saving, trying networks in order: m5-link.
- General UI components: m5-ui.
- App logic and text.
