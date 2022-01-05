# Kywoo3d Mods - Firmware mirror

Mirrors the official firmware source code, in an easy way.

## History

The v1 firmware is a fork of: https://github.com/makerbase-mks/Mks-Robin-Nano-Marlin2.0-Firmware<br>
Specifically from commit [`7d256c6d8a2b4a727f881ccdf48852e271d9fe60`](https://github.com/makerbase-mks/Mks-Robin-Nano-Marlin2.0-Firmware/commit/7d256c6d8a2b4a727f881ccdf48852e271d9fe60)<br>
Based on a `bugfix-2.0.x` Marlin version a little newer than the 2.0.6 release.

For v2 firmware, Kywoo3D seems to have upgraded to a newer Marlin 2.0.9 base.<br>
Notably they switched the UI from MKS's lvgl based UI, to Marlin's "Color UI".

```diff
 //#define TFT_CLASSIC_UI
-//#define TFT_COLOR_UI
+#define TFT_COLOR_UI
-#define TFT_LVGL_UI
+//#define TFT_LVGL_UI
```

## Overview

The `main` branch is to keep tools/docs that help maintain this mirror.<br>
[`mks-fork`](https://github.com/Kywoo3D-Mods/firmware-mirror/tree/mks-fork) is the last commit before forking from MKS. From there we have the following branches.

**Base firmware**

|Model|v1|v2|
|---|---|---|
|Tycoon|[`tycoon-v1`](https://github.com/Kywoo3D-Mods/firmware-mirror/tree/tycoon-v1)|[`tycoon-v2`](https://github.com/Kywoo3D-Mods/firmware-mirror/tree/tycoon-v2)|
|Tycoon Max|[`tycoon-max-v1`](https://github.com/Kywoo3D-Mods/firmware-mirror/tree/tycoon-max-v1)|_mod it yourself_<sup>2</sup>|
|Tycoon Slim|_mod it yourself_<sup>2</sup>|_mod it yourself_<sup>2</sup>|
|Tycoon IDEX|_missing_<sup>3</sup>|_missing_<sup>3</sup>|

**Wifi Module**

|Component|v2|
|---|---|
|Firmware|_missing_<sup>3</sup>|
|Web interface|_missing_<sup>3</sup>|

**Official upgrade kits**

|Upgrade|Tycoon|Max|Slim|IDEX|
|---|---|---|---|---|
|High temperature kit|_mod it yourself_<sup>2</sup>|_mod it yourself_<sup>2</sup>|_mod it yourself_<sup>2</sup>|_mod it yourself_<sup>2</sup>|
|Auto shutdown kit|_mod it yourself_<sup>2</sup>|_mod it yourself_<sup>2</sup>|_mod it yourself_<sup>2</sup>|N/A<sup>1</sup>|

**Kywoo Camera**

|Component||
|---|---|
|Firmware|_closed-source_<sup>4</sup>|

1. IDEX comes with auto shutdown functionality included. Should be in the base firmware.
2. Support emailed me saying, "It can be obtained by changing the source code of Tycoon or Tycoon MAX.", in other words I should modify the base Tycoon/Tycoon Max firmware to 'obtain' these variants.
3. Support emailed me saying, "The firmware of IDEX and WiFi is currently closed source and will be announced on the official website in the future." Which I _assume_ means the source will be published later.
4. The Kywoo Camera is closed-source. Their [release blog](https://www.kywoo3d.com/blogs/3d-printer-news/kywoo-camera-3d-printer-remote-control-and-monitoring) under `Kywoo Camera | Disadvantages` says while you can't customize it, closed source offers "higher security".
