# Kywoo3d Mods - Firmware mirror

Mirrors the official firmware source code, in an easy way.

## History

The v1 firmware is a fork of: https://github.com/makerbase-mks/Mks-Robin-Nano-Marlin2.0-Firmware
Specifically from commit https://github.com/makerbase-mks/Mks-Robin-Nano-Marlin2.0-Firmware/commit/7d256c6d8a2b4a727f881ccdf48852e271d9fe60
Based on a `bugfix-2.0.x` Marlin version a little newer than the 2.0.6 release.

For v2 firmware, Kywoo3D seems to have upgraded to a newer Marlin 2.0.9 base.
Notably they switched the UI from MKS's lvgl based UI, to Marlin's "Color UI".

```diff
 //#define TFT_CLASSIC_UI
-//#define TFT_COLOR_UI
+#define TFT_COLOR_UI
-#define TFT_LVGL_UI
+//#define TFT_LVGL_UI
```

## Structure

The `main` branch is to keep tools/docs that help maintain this mirror.
