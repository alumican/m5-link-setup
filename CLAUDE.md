# CLAUDE.md: m5-link-setup

Screens for joining networks by hand on M5Stack: Wi-Fi now, BLE pairing next. Usage: `README.md`. Planned work: `TODO.md`. Local-only notes: `CLAUDE.local.md` (not in git).

## Rules

- Only what a person does to join a network: reading a QR, the setup page, pairing screens, and their texts. Transports, saving and trying networks in order belong to m5-link.
- Depends on m5-link and m5-ui, never the other way round. library.json does not pin them: apps list them in lib_deps, so a version bump in either never builds two copies. Owns the camera only while a setup screen is open.
- No app-specific text, names or behavior. Texts, colors and timings come from options or `m5ui::theme`.
- Release with git tags (`v0.1.0`, ...). Apps pin a tag in `lib_deps` and switch to `symlink://` only while changing both. Breaking changes bump the major or minor version and are announced to users.
- English only, everywhere in this repository: code, comments, docs, commit messages, tags, issues and pull requests. Keep it concise and ASCII only.
- Public repository. Do not mention other projects, local paths or machine names.
- The name is `m5-link-setup`. `m5linksetup` is used only where an identifier is needed (namespace, header).
- Target: CoreS3, PlatformIO espressif32@6.9.0 (Arduino 2.0.17), M5Unified 0.2.29, M5GFX 0.2.36, C++11.
