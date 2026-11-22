# 🚏 PixelStop

<a href="README.md"><b>← Deutsche Version</b></a>

**Departures, weather, flight radar, clock and countdowns for the GeekMagic SmallTV-Ultra.**
Bought as a small Wi-Fi cube with a rather modest stock firmware – and from day one with the plan to turn it into something of my own.

[![Build](https://github.com/du13500/pixelstop/actions/workflows/build.yml/badge.svg)](https://github.com/du13500/pixelstop/actions/workflows/build.yml)
![Version](https://img.shields.io/badge/version-v0.5.4.5--dev-orange)
![Hardware](https://img.shields.io/badge/hardware-ESP8266%20%C2%B7%20SmallTV--Ultra-blue)
![Hardware test](https://img.shields.io/badge/hardware%20test-v0.5.4.5%20pending-yellow)
![Hobby project](https://img.shields.io/badge/hobby%20project-non--commercial-lightgrey)
![License](https://img.shields.io/badge/license-MIT-green)
![Mission](https://img.shields.io/badge/mission-Know%20before%20you%20go-ff5d56)

| | | |
| :---: | :---: | :---: |
| <img src="docs/screens/en/abfahrten.png" width="240" alt="Departures"><br><sub><b>Departures</b></sub> | <img src="docs/screens/en/wetter.png" width="240" alt="Weather"><br><sub><b>Weather</b></sub> | <img src="docs/screens/en/vorhersage.png" width="240" alt="3-day forecast"><br><sub><b>3-day forecast</b></sub> |
| <img src="docs/screens/en/flugradar.png" width="240" alt="Flight radar"><br><sub><b>Flight radar</b></sub> | <img src="docs/screens/en/flugtracker.png" width="240" alt="Flight tracker"><br><sub><b>Flight tracker</b></sub> | <img src="docs/screens/en/uhr.png" width="240" alt="Clock"><br><sub><b>Clock</b></sub> |
| <img src="docs/screens/en/countdown-tage.png" width="240" alt="Countdown"><br><sub><b>Countdown</b></sub> |   |   |

<p align="center"><sub>Sample data, real firmware output. More images and explanations under <a href="#programs">Programs</a>.</sub></p>

## What is PixelStop?

PixelStop is a custom firmware in German and English for the **GeekMagic SmallTV-Ultra**, a 240 × 240 pixel cube with an ESP8266. Once set up it only needs USB power and Wi-Fi – **no server, no Raspberry Pi**. Everything is configured in a web interface in your browser.

In rotation or permanently it shows seven programs: departures, weather, 3-day forecast, flight radar, flight tracker, clock and up to four countdowns. Plus night mode, brightness down to 1 %, place search with a map, Greek and Cyrillic characters, and a diagnostics view.

## Programs

Expand a program to see what it shows, what you can set and all example images. The images are real firmware output with **sample data**; animated images run in real time, one second per frame.

<details><summary><b>🚏 Departures</b></summary>

Live departures of a stop of Verkehrsverbund Oberelbe (VVO, Dresden region): line, destination, minutes to departure, delay (yellow) and cancellation (✕). Three to five departures on one page, or ten on two pages switching every 5 seconds; filter by mode of transport. Destinations can be shown without the town of the stop and abbreviated. Minutes count down between requests – even while VVO cannot be reached.

| | | |
| :---: | :---: | :---: |
| <img src="docs/screens/en/abfahrten.png" width="240" alt="Departures"><br><sub><b>Departures</b> · Dresden Hauptbahnhof with a delay and a cancellation</sub> | <img src="docs/screens/en/abfahrten-ziele.png" width="240" alt="Destinations"><br><sub><b>Destinations</b> · town dropped, “Meißen T.” abbreviated</sub> | <img src="docs/screens/en/abfahrten-seiten.gif" width="240" alt="Two pages"><br><sub><b>Two pages</b> · ten departures, switching every 5 s</sub> |

</details>

<details><summary><b>🌦️ Weather</b></summary>

Current temperature, day/night weather icon, high and low, chance of rain, sunrise and sunset, in °C or °F. The place is chosen by search, on the map or by coordinates; a display name of your own is possible.

| | | |
| :---: | :---: | :---: |
| <img src="docs/screens/en/wetter.png" width="240" alt="Day"><br><sub><b>Day</b> · Berlin, light rain</sub> | <img src="docs/screens/en/wetter-nacht.png" width="240" alt="Night"><br><sub><b>Night</b> · Berlin, clear</sub> |   |

</details>

<details><summary><b>📅 3-day forecast</b></summary>

Weekday, weather icon, high and low and chance of rain for the next three days – as a view of its own in the rotation.

| | | |
| :---: | :---: | :---: |
| <img src="docs/screens/en/vorhersage.png" width="240" alt="Forecast"><br><sub><b>Forecast</b> · Berlin, the next three days</sub> |   |   |

</details>

<details><summary><b>✈️ Flight radar</b></summary>

The nearest aircraft within the chosen radius: callsign, airline, route, type, registration, distance and altitude. Between two requests the cube keeps calculating distance and altitude; the footer shows the age of the last request.

The **Flight radar** tab adjusts the view:

- **Operator:** name from the callsign (default), umbrella term or both, e.g. “US Air Force / AMC” – for known military callsigns.
- **Speed** in km/h or knots, alternating with the registration every 3 seconds.
- **▲▼** between distance and altitude when climbing or descending at 300 ft/min or more.
- **MIL** next to the callsign for military aircraft.
- **Hide:** helicopters, light aircraft, gliders/balloons/ultralights/drones, aircraft on the ground.
- **Always on:** emergencies (squawk 7500/7600/7700) with a red callsign and a red route line in which route and “SQUAWK 7700” alternate every 3 s; airline names that are too long alternate every 3 s as well.

| | | |
| :---: | :---: | :---: |
| <img src="docs/screens/en/flugradar.gif" width="240" alt="Normal"><br><sub><b>Normal</b> · Emirates A380 over Dresden</sub> | <img src="docs/screens/en/radar-steigflug.gif" width="240" alt="▲ Climbing"><br><sub><b>▲ Climbing</b> · ALROSA Tu-154M after take-off in Mirny</sub> | <img src="docs/screens/en/radar-sinkflug.gif" width="240" alt="▼ Descending"><br><sub><b>▼ Descending</b> · European Air Charter MD-82 approaching Varna</sub> |
| <img src="docs/screens/en/radar-name.gif" width="240" alt="Long name"><br><sub><b>Long name</b> · airline alternating, speed instead of registration</sub> | <img src="docs/screens/en/radar-gelb.gif" width="240" alt="🟡 Last data"><br><sub><b>🟡 Last data</b> · request failed, values estimated</sub> | <img src="docs/screens/en/radar-grau.gif" width="240" alt="⚫ No data"><br><sub><b>⚫ No data</b> · too long without a request, values frozen</sub> |
| <img src="docs/screens/en/radar-leer.png" width="240" alt="Empty"><br><sub><b>Empty</b> · no aircraft within the radius</sub> | <img src="docs/screens/en/radar-colone.gif" width="240" alt="Speed"><br><sub><b>Speed</b> · 4-character codes, “N/A” without altitude · fictional</sub> | <img src="docs/screens/en/radar-apollo.gif" width="240" alt="MIL"><br><sub><b>MIL</b> · military marking · fictional</sub> |
| <img src="docs/screens/en/radar-karma.gif" width="240" alt="Emergency"><br><sub><b>Emergency</b> · squawk 7700, route and squawk alternating · fictional</sub> |   |   |

The Tu-154M and the MD-82 recall real last flights: ALROSA RA-85757, Mirny – Novosibirsk on 28 Oct 2020 (the last civil Tu-154 scheduled flight in Russia), and European Air Charter LZ-LDP, Hurghada – Varna on 21 Oct 2023 (the last MD-80 flight in Europe); flight numbers and values are recreated. Military and emergency show **fictional data** so that no real flight is depicted this way; their footer reads “DRADIS” instead of “ADSB.LOL”.

| Radar footer | Meaning |
| --- | --- |
| `● ADSB.LOL LIVE 4s` | last successful request 4 seconds ago |
| `LAST DATA 16s` | request failed, the previous data is still shown |
| `PAUSED 46s` | adsb.lol asks for a pause; age of the last request |
| `NO AIRCRAFT` | request successful, but nothing within the radius |
| `ADSB.LOL / Access denied` | adsb.lol refuses (401/403/426); long pause instead of constant retries |

| Dot in the footer | Meaning |
| --- | --- |
| ⚪ briefly white | new data just received |
| 🟢 green | requests on schedule; distance and altitude are calculated in between |
| 🟡 yellow | request overdue, failed or provider pause; values are estimated |
| ⚫ grey | no data for too long; distance and altitude stand still |

</details>

<details><summary><b>🛫 Flight tracker</b></summary>

One specific flight with scheduled and actual times, status, flight time and live altitude and speed. Requires your own aviationstack key.

| | | |
| :---: | :---: | :---: |
| <img src="docs/screens/en/flugtracker.png" width="240" alt="Flight tracker"><br><sub><b>Flight tracker</b> · LH1178 Dresden – Frankfurt</sub> |   |   |

</details>

<details><summary><b>🕰️ Clock</b></summary>

Time and date with a selectable time zone; the time is also shown in the footer of the other views.

| | | |
| :---: | :---: | :---: |
| <img src="docs/screens/en/uhr.png" width="240" alt="Clock"><br><sub><b>Clock</b> · with date</sub> |   |   |

</details>

<details><summary><b>⏳ Countdown 1–4</b></summary>

Four independent countdowns in days, in hours : minutes : seconds or in minutes : seconds, each with its own title and colours. The digits look like an LED clock radio; unlit segments glow faintly.

- **Style:** “Classic” or “HUD” with frame, “T-MINUS”/“T-PLUS” and a progress bar.
- **Sign:** “−” before the target and “+” after it – or no sign; then the countdown stops at zero.
- **Start date** (days): sets the HUD progress bar; if left empty, today is filled in when the countdown is started.

| | | |
| :---: | :---: | :---: |
| <img src="docs/screens/en/countdown-tage.png" width="240" alt="Days"><br><sub><b>Days</b> · classic, with sign</sub> | <img src="docs/screens/en/countdown-timer.gif" width="240" alt="Timer"><br><sub><b>Timer</b> · minutes : seconds</sub> | <img src="docs/screens/en/countdown-hud.gif" width="240" alt="HUD"><br><sub><b>HUD</b> · hours : minutes : seconds with bar</sub> |
| <img src="docs/screens/en/countdown-tage-hud.png" width="240" alt="HUD in days"><br><sub><b>HUD in days</b> · without sign, bar from the start date</sub> |   |   |

</details>

## Status: v0.5.4.5-dev

| Area | Status |
| --- | --- |
| Departures (VVO), weather, clock, countdowns | ✅ stable in daily use |
| Flight radar over HTTP/1.1, pauses on rejected requests | ✅ confirmed on hardware (v0.4.14.7) |
| Radar only while visible | ✅ running on hardware (v0.4.14.8) |
| Quota guard for adsb.lol (30 s to month), counters across restarts | ⏳ hardware test pending (v0.5.4) |
| Backlight at 1 % without flashes | ⚠️ occasional flashes; the sigma-delta attempt stopped Wi-Fi and was removed, new approach open |
| German/English language switch, abbreviated destinations | ✅ confirmed on hardware (v0.5.0) |
| 3-day forecast, tidier web interface, “Show now”, town of the stop | ✅ confirmed on hardware (v0.5.1) |
| Radar: distance and altitude continued between requests, status dot | ✅ confirmed on hardware (v0.5.2) |
| Radar options and filters, aircraft types, forecast as a view | ✅ running on hardware (v0.5.3) |
| Protection against foreign web pages, rescue mode, reconnect after power cuts, network watchdog, compressed web interface | ⏳ hardware test pending (v0.5.4) |
| Flight tracker with your own aviationstack key | ⏳ not fully tested on hardware |
| Countdown in clock-radio style, “HUD” style, selectable sign | ⏳ hardware test pending (v0.5.4.5) |

All changes are listed in [`CHANGELOG.md`](CHANGELOG.md) (German), open checks in [`ROADMAP.md`](ROADMAP.md).

## How it works

```
  Browser (phone/PC)                     PixelStop cube (ESP8266)
 ┌──────────────────────┐  Wi-Fi   ┌──────────────────────────────────┐
 │ Web interface        │◀────────▶│ Web server · settings            │
 │ Place search + map ──┼─▶ OSM    │ Requests: VVO ───── departures   │
 └──────────────────────┘          │           Open-Meteo  weather    │
                                   │           adsb.lol ── radar      │
                                   │           aviationstack tracker  │
                                   │ Display 240 × 240 (ST7789)       │
                                   └──────────────────────────────────┘
```

The cube queries the services itself and draws the views directly. Place search runs in the browser so that the ESP8266's scarce memory stays free. The flight radar only polls while it is on screen (plus shortly before it appears) and budgets adsb.lol's daily quota by itself when no key is set. Between two requests it continues distance and altitude from speed, track and vertical rate, so the values stay alive even with rare requests.

## Supported device

**GeekMagic SmallTV-Ultra with ESP8266 / ESP-12F, 4 MB flash and ST7789 display (240 × 240).** Other SmallTV models are untested. The firmware is not meant for ESP32 devices.

## Installation and setup

1. Flash the firmware onto the SmallTV-Ultra. The first flashing method depends on the installed firmware; the USB power port alone is not a guaranteed programming interface.
2. On first start, join the Wi-Fi **PixelStop-Setup** and open **192.168.4.1** in your browser.
3. Under **System**, pick your Wi-Fi, enter the password and save.
4. After the restart, open the web interface via the IP address on the display, or via `pixelstop.local` depending on your network.
5. Under **System → Sprache / Language**, choose English.
6. Under **Display**, set views, rotation times, time zone, brightness and night mode, then the stop, places and timers in their tabs.

> **Note:** Weather and flight radar have independent locations (place search, map or coordinates). An optional display name replaces the place name on screen. An empty Wi-Fi password field keeps the stored password.

**Updates** go through **System → Firmware update** with the matching `.bin`. Settings are kept.

## Building the firmware

The workflow [`build.yml`](.github/workflows/build.yml) builds the firmware on every push and pull request; the result is available under **Actions** as the artifact `pixelstop-v<version>-firmware`. A tag such as `v0.5.0` (matching `FW_VERSION` in [`src/Version.h`](src/Version.h)) also creates a **GitHub release** with the BIN file, checksum and companion package. Locally, `pio run -e smalltv_ultra` is enough.

The screenshots in this README are produced from the real display code by `python3 tools/render_screens.py docs/screens --frame`.

## Data sources and limits

| Service | Used for | Note |
| --- | --- | --- |
| **VVO** | Departures | Dresden / Upper Elbe region |
| **Open-Meteo** | Weather | free API for non-commercial use |
| **adsb.lol** | Radar positions, routes | data under ODbL; fixed quotas per hour, day, week and month (see below) |
| **adsbdb** | Airline, aircraft type, routes | separate reuse terms for route data |
| **aviationstack** | Flight tracker | your own key, plan limits |
| **OpenStreetMap / Nominatim** | Map, place names | manual search only, via the browser |

The data sources are meant for non-commercial use only; this also applies to PixelStop as a hobby project. The ESP8266 does not verify certificates on HTTPS connections (not enough memory for certificate chains). The connections are encrypted, but someone with access to your own network could in theory sit in between and read, for example, the adsb.lol key.

Radar routes come from additional databases and can be wrong or incomplete despite a position check; unsuitable routes are not replaced by guessed airports. `N/A` means missing data, `GND` an aircraft explicitly reported on the ground. Altitudes are flight altitudes, not height above ground. More in [`THIRD_PARTY_SERVICES.md`](THIRD_PARTY_SERVICES.md).

### Key for adsb.lol

adsb.lol hands out free keys; with a key every device gets its own quota. Enter the key in the **Flight radar** tab, where the links are shown as well.

A free key allows **5 requests per 30 s, 20 per minute, 150 per hour, 500 per day, 2,000 per week and 5,000 per month** (windows in UTC: full hour, midnight, Monday, first of the month). Exceeding them repeatedly can cost the key. PixelStop therefore counts every request (also across restarts and power cuts), plans with a safety margin and only asks while the radar is shown. The chosen interval applies, but at most as often as the hour allows (135 requests, about every 27 seconds). Once the day, week or month is used up, the radar pauses until the next window – round-the-clock use is not possible with adsb.lol alone. Distance and altitude are continued in between. Usage and the actual interval are listed under **System → Status**.

| Way | For whom |
| --- | --- |
| [Anonymous key](https://www.adsb.lol/pow/challenge) | no account, no email: a small puzzle in the browser, valid for about a month, then simply get a new one |
| [Receiver key](https://www.adsb.lol/receiver) | if you run an ADS-B receiver that feeds adsb.lol; long-lived |
| [Key on request](https://www.adsb.lol/request) | for all other projects, reviewed by hand |

All ways are listed at [adsb.lol/mmb](https://www.adsb.lol/mmb). If adsb.lol rejects a key, it has usually expired; the web interface then points this out.

## Plans

- **More regions:** departures not only from VVO but selectable per transport association or Germany-wide (planned for v0.6.0).
- **Custom destination replacements** in the web interface.
- English diagnostics texts.
- **Honest radar** (v0.5.5): a pause screen when an adsb.lol limit is reached, how far the quota goes shown in the web interface, and an own data source in the home network. Then **fallback sources** (v0.5.6).

## Diagnostics and bug reports

Under **System → Diagnostics**, **Load log** shows the latest events. After a glitch, load the log right away; after a restart only the reset reason and the last marker remain. If the web interface does not respond, the address `http://<IP>/api/log` often still works.

**Self-help of the firmware:**

- **Power cut:** if the router starts more slowly than the cube, the cube waits in the setup hotspot and retries the stored Wi-Fi every minute – but not while a device is connected to the hotspot.
- **Network hang:** 10 minutes without a single answer despite requests lead to a new Wi-Fi connection, 5 minutes later to a restart (the marker is kept).
- **Rescue mode:** after three crashes in quick succession PixelStop starts without requests; the web interface and firmware update work, the display shows “RESCUE MODE”. A normal restart ends it.
- **Foreign web pages** can neither change settings nor install firmware; tools like `curl` on the home network keep working. Helpful for bug reports: firmware version, affected view, steps to reproduce and a photo. Please **never publish Wi-Fi passwords or API keys**.

## Credits and license

- Own project code and documentation: **MIT**, Copyright (c) 2026 du13500. Keep the copyright and license notice when redistributing.
- Hardware and display basis from [smalltv-mod](https://github.com/giovi321/smalltv-mod) by giovi321 (WTFPL); the original notice is included.
- Libraries and their licenses: see [`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md) and [`LICENSES/`](LICENSES).
- Language flag: [English_language.svg](https://commons.wikimedia.org/wiki/File:English_language.svg) by Rei-artur and Kjoonlee, Wikimedia Commons, public domain.
- Map data © OpenStreetMap contributors.

---

**Know before you go. 🚏**

<sub>PixelStop is an independent hobby project and is not affiliated with GeekMagic, VVO or any of the data providers named above.</sub>
