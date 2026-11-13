# TODO

In order.

## 1. Check on a device

- [ ] Run `examples/wifi` on a CoreS3: phone share QR, setup page, Cancel, timeout, no camera.
- [ ] Verify that `lib_deps = https://github.com/alumican/m5-link-setup.git#v0.1.0` builds in PlatformIO.

## 2. Follow m5-link

- [ ] Depend on m5-link once it is installable by git URL, and use its credential type.
- [ ] Hand the credentials to m5-link's save API instead of returning them only, if it gets one.

## 3. Tokens

- [ ] Move the screen's sizes and the label color into tokens when m5-ui has them.

## 4. BLE pairing

Once m5-link has BLE (HID keyboard, serial):

- [ ] Pairing screen: the name the host sees and, if asked, the passkey to confirm.
- [ ] Paired hosts: list and forget, as Panel content or a popup.
- [ ] State: advertising, connected, disconnected.

## Out of scope

- Transports, connecting, saving, trying networks in order: m5-link.
- General UI components: m5-ui.
- App logic and text.
