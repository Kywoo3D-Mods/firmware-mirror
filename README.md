# Kywoo3D Mods - Firmware mirror

Mirrors the official firmware source code, in an easier way.

**Important:** this repository tries to organize the official firmware source code.<br>
**But it's reorganized by the community!**

For releases by Kywoo3D themselves, check the [download page](https://www.kywoo3d.com/pages/kywoo-3d-printer-download-page) or their [Github](https://github.com/Kywoo3D).

# Overview

The `main` branch is to keep tools/docs that help maintain this mirror.<br>
[`mks-fork`](https://github.com/Kywoo3D-Mods/firmware-mirror/tree/mks-fork) is the last commit before forking from MKS. From there we have the following branches.

**Legend**
|Color|Description|
|---|---|
|🟢 Public | Released officially, available on Kywoo3D's download page.
|🟠 Private | Kywoo3D shared it (probably support email) but doesn't share it publicly.
|⚪ Missing | Where is it? Who knows?
|🔴 Unavailable | Kywoo3D confirmed it isn't available.

## Tycoon

|Version|Mods|Binary|Source|
|---|---|---|---|
|v1.1.0 | - | ⚪ _missing_ | 🟢 [`tycoon-v1`][tycoon-v1-source]
|v1.1.2 | - | 🟢 [Downloads][tycoon-dl] | ⚪ _missing_
|v1.1.2 | Auto shutdown<sup>[?][as-mod]</sup> | 🟢 [Downloads][tycoon-dl] | ⚪ _missing_

|Version|Mods|Binary|Source|
|---|---|---|---|
|v2.0.1 | - | 🟢 [Downloads][tycoon-dl] | 🟢 [`tycoon-v2`][tycoon-v2-source]
|v2.0.1 | High temperature<sup>[?][ht-mod]</sup> | 🟢 [Downloads][tycoon-dl] | 🟠 [`tycoon-v2-hightemp`][tycoon-v2-hightemp-source] <sup>[see #1][email-1]</sup>
|v2.1.0 | Auto shutdown<sup>[?][as-mod]</sup> | 🟢 [Downloads][tycoon-dl] | ⚪ _missing_
|v2.1.3 | High temp & shutdown<sup>[3](#notes)</sup> | 🟢 [Downloads][tycoon-dl] | 🟢 [`tycoon-v2.1.3`][tycoon-v2.1.3-source]

## Tycoon Max

|Version|Mods|Binary|Source|
|---|---|---|---|
|v1.1.0 | - | ⚪ _missing_ | 🟢 [`tycoon-max-v1`][tycoon-max-v1-source]
|v1.1.2 | - | 🟢 [Downloads][tycoon-max-dl] | ⚪ _missing_
|v1.1.2 | Auto shutdown<sup>[?][as-mod]</sup> | 🟢 [Downloads][tycoon-max-dl] | ⚪ _missing_

|Version|Mods|Binary|Source|
|---|---|---|---|
|v2.0.1 | - | 🟢 [Downloads][tycoon-max-dl] | 🟠 [`tycoon-max-v2`][tycoon-max-v2-source] <sup>[see #1][email-1]</sup>
|v2.0.1 | High temperature<sup>[?][ht-mod]</sup> | 🟢 [Downloads][tycoon-max-dl] | 🟠 [`tycoon-max-v2-hightemp`][tycoon-max-v2-hightemp-source] <sup>[see #1][email-1]</sup>
|v2.1.0 | Auto shutdown<sup>[?][as-mod]</sup> | 🟢 [Downloads][tycoon-max-dl] | ⚪ _missing_
|v2.1.3 | High temp & shutdown<sup>[3](#notes)</sup> | 🟢 [Downloads][tycoon-max-dl] | 🟢 [`tycoon-max-v2.1.3`][tycoon-max-v2.1.3-source]

## Tycoon Slim

|Version|Mods|Binary|Source|
|---|---|---|---|
|v2.1.0 | - | 🟢 [Downloads][tycoon-slim-dl] | ⚪ _missing_
|v2.1.3 | High temp & shutdown<sup>[3](#notes)</sup> | 🟢 [Downloads][tycoon-slim-dl] | 🟢 [`tycoon-slim-v2.1.3`][tycoon-slim-v2.1.3-source]

## Tycoon IDEX

|Version|Mods|Binary|Source|
|---|---|---|---|
|v2.1.0 | (Auto shutdown<sup>[1](#notes)</sup>) | 🟢 [Downloads][tycoon-idex-dl] | 🟠 [`tycoon-idex-v2`][tycoon-idex-v2-source] <sup>[see #1][email-1]</sup>
|v2.2.0 | (Auto shutdown<sup>[1](#notes)</sup>) | 🟢 [Downloads][tycoon-idex-dl] | 🔴 Closed<sup>[2](#notes)</sup>
|v2.2.0 | High temp & shutdown<sup>[1](#notes)</sup> | 🟢 [Downloads][tycoon-idex-dl] | 🔴 Closed<sup>[2](#notes)</sup>

## Wifi Module<sup>[?][wifi-mod]</sup>

|Version|Mods|Binary|Source|
|---|---|---|---|
|v2.0.1 Firmware | - | 🟢 [Downloads][wifi-dl] | 🔴 Closed<sup>[2](#notes)</sup>
|v2.0.1 Web UI | - | 🟢 [Downloads][wifi-dl] | 🟢 [Downloads][wifi-dl]
|v2.1.3 Firmware<sup>[4](#notes)</sup> | - | 🟢 [Direct download][wifi-v2.1.3-dl] | 🔴 Closed<sup>[2](#notes)</sup>

## Kywoo Camera<sup>[?][kw-cam]</sup>

_Their [release blog](https://www.kywoo3d.com/blogs/3d-printer-news/kywoo-camera-3d-printer-remote-control-and-monitoring) under `Kywoo Camera | Disadvantages` says while you can't customize it, closed source offers "higher security"._

|Version|Mods|Binary|Source|
|---|---|---|---|
|Android App | - | 🟢 [Downloads](https://www.kywoo3d.com/pages/accessories-manual-guideboook-instruction-download) / [Gplay](https://play.google.com/store/apps/details?id=kywoo.printer.client) | 🔴 Closed
|Firmware | - | 🔴 | 🔴 Closed

## Notes

1. IDEX comes with auto shutdown functionality included. Should be in the base firmware.
2. Support emailed me saying, "The firmware of IDEX and WiFi is currently closed source and will be announced on the official website in the future." Which I _assume_ means the source will be published later.
3. The v2.1.3 firmware version no longer needs different firmware for different mods.
   The auto shutdown module should be detected and used if present. While high temperature prints can be enabled through the touchscreen settings.
4. The v2.1.3 firmware releases, included a new `KWWifi.bin` file. But hasn't been shared on a separate download page.
   Kywoo3D hasn't 'named' this v2.1.3, but I'm using this as it came bundled with the v2.1.3 printer firmware releases.

# History

The v1 firmware is a fork of: https://github.com/makerbase-mks/Mks-Robin-Nano-Marlin2.0-Firmware<br>
Specifically from commit [`7d256c6d8a2b4a727f881ccdf48852e271d9fe60`](https://github.com/makerbase-mks/Mks-Robin-Nano-Marlin2.0-Firmware/commit/7d256c6d8a2b4a727f881ccdf48852e271d9fe60)<br>
Based on a `bugfix-2.0.x` Marlin version a little newer than the 2.0.6 release.

---

For v2 firmware, Kywoo3D seems to have upgraded to a newer Marlin 2.0.9 base.<br>
Notably they switched the UI from MKS's lvgl based UI, to Marlin's "Color UI".

```diff
 //#define TFT_CLASSIC_UI
-//#define TFT_COLOR_UI
+#define TFT_COLOR_UI
-#define TFT_LVGL_UI
+//#define TFT_LVGL_UI
```

---

As of v2.1.3 firmware, Kywoo3D switch to a different fork: https://github.com/Andy-Big/Marlin_FB_Reborn_Old<br>
Specifically commit [`b9a461b59f45fcb10d202b77cc17223166d61b93`](https://github.com/Andy-Big/Marlin_FB_Reborn_Old/commit/b9a461b59f45fcb10d202b77cc17223166d61b93)<br>
Based on a `2.0.9.2` Marlin version, modified by [Flyingbear][fb-site], modified by Kywoo3D.

Some interesting examples Kywoo3D hasn't mentioned in [their release notes][notes-213], thanks to the Flyingbear fork,
both [Linear Advance][lin-advance] and [Firmware Retract][fwretract] are now enabled.


[ht-mod]: https://www.kywoo3d.com/collections/accessories/products/300-high-temperature-printing-kit "High temperature kit"
[as-mod]: https://www.kywoo3d.com/products/buy-kywoo-3d-auto-shut-off-modular-for-3d-printers "Auto shutdown mod"
[wifi-mod]: https://www.kywoo3d.com/products/wifi-module "Wifi module"
[kw-cam]: https://www.kywoo3d.com/products/kywoo-camera-3d-printer-remote-control-monitoring "Kywoo Camera"
[tycoon-dl]: https://www.kywoo3d.com/pages/tycoon-firmware-and-manual-download "Tycoon downloads"
[tycoon-max-dl]: https://www.kywoo3d.com/pages/tycoon-max-manual-and-firmware-download "Tycoon Max downloads"
[tycoon-slim-dl]: https://www.kywoo3d.com/pages/tycoon-slim-firmware-and-manual-download "Tycoon Slim downloads"
[tycoon-idex-dl]: https://www.kywoo3d.com/pages/tycoon-idex-firmware-and-manual-download "Tycoon IDEX downloads"
[wifi-dl]: https://www.kywoo3d.com/pages/wifi-driver-instruction-download "Wifi downloads"
[wifi-v2.1.3-dl]: https://github.com/Kywoo3D/Tycoon-V2.1.3-Multi-functional-Firmware/raw/main/KWWifi.bin "Wifi v2.1.3* Firmware download"
[tycoon-v1-source]: https://github.com/Kywoo3D-Mods/firmware-mirror/tree/tycoon-v1 "tycoon-v1 source"
[tycoon-v2-source]: https://github.com/Kywoo3D-Mods/firmware-mirror/tree/tycoon-v2 "tycoon-v2 source"
[tycoon-v2-hightemp-source]: https://github.com/Kywoo3D-Mods/firmware-mirror/tree/tycoon-v2-hightemp "tycoon-v2-hightemp source"
[tycoon-v2.1.3-source]: https://github.com/Kywoo3D-Mods/firmware-mirror/tree/tycoon-v2.1.3 "tycoon-v2.1.3 source"
[tycoon-max-v1-source]: https://github.com/Kywoo3D-Mods/firmware-mirror/tree/tycoon-max-v1 "tycoon-max-v1 source"
[tycoon-max-v2-source]: https://github.com/Kywoo3D-Mods/firmware-mirror/tree/tycoon-max-v2-hightemp "tycoon-max-v2-hightemp source"
[tycoon-max-v2-hightemp-source]: https://github.com/Kywoo3D-Mods/firmware-mirror/tree/tycoon-max-v2 "tycoon-max-v2 source"
[tycoon-max-v2.1.3-source]: https://github.com/Kywoo3D-Mods/firmware-mirror/tree/tycoon-max-v2.1.3 "tycoon-max-v2.1.3 source"
[tycoon-idex-v2-source]: https://github.com/Kywoo3D-Mods/firmware-mirror/tree/tycoon-idex-v2 "tycoon-idex-v2 source"
[tycoon-slim-v2.1.3-source]: https://github.com/Kywoo3D-Mods/firmware-mirror/tree/tycoon-slim-v2.1.3 "tycoon-slim-v2.1.3 source"
[email-1]: https://github.com/Kywoo3D-Mods/firmware-mirror/issues/1 "Emailed source #1"
[fb-site]: https://flyingbear3dprinter.com "Flyingbear 3D Printer site"
[lin-advance]: https://marlinfw.org/docs/features/lin_advance.html "Linear Advance | Marlin Firmware"
[fwretract]: https://marlinfw.org/docs/features/fwretract.html "Firmware Retract | Marlin Firmware"
[notes-213]: https://drive.google.com/file/d/1D1qJ6w92-7uhAWSLKu_PBEy0ppQUlESG/view "Kywoo Tycoon Max Slim V2.1.3 Multi-functional Firmware Upgrading and Optimization.pdf"
