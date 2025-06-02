# LunaTranslator Fixes + LunaHook

### Notes

- Changed app [icon](https://github.com/HIllya51/LunaTranslator/discussions/1109)
- Disabled check for updates
- Disabled auto open website
- **\[important\]** While in text selection mode drag window by toolbar only (fixed toolbar height in v7.18.5 and later)
- Relaxed some loop sleep times to ease on CPU load (v7.15.2 and later)
- **\[important\]** Disabled playtime telemetry, it was hammering disk writes (v7.15.2 and later)
- Added colon after translator name in Webview2 (v7.18.2 and later)
- Allow extensions in browser translation (DeepL CDP) (v7.18.3 and later)
- **\[upstream\]** v7.18.6 changed Local OCR
- Keep `--remote-allow-origins=* --disable-gpu` in browser translation (DeepL CDP) (v7.18.8.1 and later)
- **\[note\]** To use portable Webview2 set environment variable `WEBVIEW2_BROWSER_EXECUTABLE_FOLDER=C:\portable\webview2`
- Added tray tooltip (v7.23.3.3 and later)
- **\[important\]** Restored LunaHook plugin support (v8.7.1.1 and later)
- **\[upstream\]** Removed LunaHook CLI (v9.0.0.2 and later)
- **\[upstream\]** Removed Favorites feature (v9.3.1 and later)
- **\[upstream\]** Removed built-in OCR which had a lot of errors (v9.4.5)
- **\[upstream\]** Introduced new built-in OCR (v10.0.0.5 and later)
- Disabled random nagging in settings dialog (10.3.2.4 and later)

### Download

LunaTranslator: https://github.com/setsumi/LT-Fixes/releases \
LunaHook: https://github.com/setsumi/LT-Fixes/releases?q=LunaHook&expanded=true

Legacy LunaHook v5.0.0 + Plugins: https://github.com/setsumi/LunaHook/releases
