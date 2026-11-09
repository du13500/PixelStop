# Testplan

Stand: v0.5.3-dev (09.11.2026). Dieser Plan wird mit jeder Version aktualisiert; frühere Pläne stehen in der Git-Historie, Änderungen in [`CHANGELOG.md`](CHANGELOG.md).

## Hardwaretest v0.5.3-dev

1. **Dimmverfahren:** Nachts bei 1–10 % unter *Anzeige → Dimmverfahren* zwischen Standard (PWM) und Sigma-Delta wechseln. Aufblitzen, Fiepen und saubere Helligkeitsstufen vergleichen; dabei auch Radar mit Abrufen und offenes Webinterface laufen lassen.
2. **Webinterface:** Menü hinter ☰ öffnen und schließen (Tippen daneben, Escape), Seitenname unter dem Titel; Auge-Symbol nur beim Tippen, nach dem Speichern wieder verdeckt und leer.
3. **3-Tage-Vorhersage:** nach dem Update als eigene Zeile unter „Wetter“ angehakt (wenn sie vorher aktiv war); eigene Dauer, ▶ Jetzt anzeigen; Wetter-Reiter ohne die alte Checkbox. Rotation in der Reihenfolge der Liste.
4. **Radar-Optionen:** Betreiber A/B/C bei einem Militärflug (z. B. RCH…), Geschwindigkeit in km/h und kt im Wechsel mit dem Kennzeichen, ▲▼ bei Steig-/Sinkflug, MIL-Markierung an und aus.
5. **Filter:** einzeln aktivieren (z. B. „am Boden“, „Hubschrauber“). Im Log Dauer und kleinsten freien Speicher der Radar-Zeile beobachten, im Status „… Flugzeuge geprüft“. Großen Radius (100 km) gegenprüfen.
6. **Routen:** Flüge zu kleinen Flugplätzen zeigen 4-stellige ICAO-Codes statt „N/A“; 3-stellige Routen unverändert.
7. **Log:** erfolgreiche Radarabrufe als eine Zeile; Zusatzabfragen mit „leer“ oder „übersprungen“. Offene Beobachtung: easyJet ohne Kennzeichen und Route – bei Auftreten Statuszeilen „Aktuelles Flugzeug“ und „Radar-Route“ sowie den Log sichern.
8. **Notfall** (nur falls zufällig zu sehen): Rufzeichen und Routenzeile rot, Route und „SQUAWK 7700“ im 3-s-Wechsel.

## Dauerhafte Prüfungen

- **Flugradar:** Langzeittest über mehrere Tage mit Rotation; Zähler „adsb.lol heute“ und Verbrauch im adsb.lol-Dashboard vergleichen (`src/RadarBudget.h`). Freien Speicher während der Abfrage im Blick behalten (ca. 5 KB bei 4-KB-TLS-Puffer).
- **Wetter:** Exception 29 beim Programmwechsel ist seit den Speicherkorrekturen nicht wieder aufgetreten; im Langzeittest weiter beobachten.
- **Zeitzone, Sommerzeit, Nachtmodus:** spätestens bei der Zeitumstellung Ende März prüfen.
- **Abfahrten:** zehn Abfahrten mit Seitenwechsel 5+5, Verhalten bei Verbindungsproblemen.
- **Flug-Tracker** mit gültigem aviationstack-Schlüssel vollständig testen.

**Bei Fehlern:** HTTP 429 mit Pause und anschließender Erholung ist kein Neustart. Nach einem Neustart sofort unter *System → Diagnose* den Log laden und die ELF-Datei des installierten Builds sichern (Begleitpaket des Builds). Gleicher Reset-Text und RTC-Marker in späteren Logs desselben Starts bedeuten keinen erneuten Absturz; der RTC-Marker ist ein Hinweis, kein Ursachenbeweis.

## Stand des letzten Builds

v0.5.3-dev, GitHub Actions, PlatformIO mit ESP8266-Framework 3.1.2 und gepinnten Bibliotheken:

| | belegt | von | Anteil |
| --- | --- | --- | --- |
| Statischer RAM | 52.144 Byte | 81.920 Byte | 63,7 % |
| Flash | 765.039 Byte | 1.044.464 Byte | 73,2 % |

Zum Vergleich v0.4.14.5.1: 53.288 Byte RAM, 709.007 Byte Flash – feste Texte und Tabellen liegen inzwischen im Flash.

## Lokale Prüfungen

Dieselben Prüfungen führt der Workflow [`build.yml`](.github/workflows/build.yml) bei jedem Push aus.

```sh
python3 tools/check.py
python3 tools/webui_en.py >/dev/null && git diff --exit-code src/webui_en.h
python3 tools/test_radar.py
python3 tools/test_motion.py
python3 tools/test_policy.py
python3 tools/test_chunked.py
python3 tools/test_destinations.py
python3 tools/test_display.py
python3 tools/test_brightness.py
python3 tools/test_diagnostics.py
python3 tools/test_enrichment.py
g++ -std=c++11 -Wall -Wextra -Werror tests/test_core.cpp -o /tmp/pixelstop-core-test && /tmp/pixelstop-core-test
npm install --ignore-scripts && npx playwright install webkit && npm run test:webui
```

Nach `pio pkg install -e smalltv_ultra` (gepinntes ArduinoJson):

```sh
python3 tools/test_network_json.py
python3 tools/test_radar_scan.py
python3 tools/test_airline_cache.py
python3 tools/test_vvo_response.py
python3 tools/test_geo_buffer.py
pio run -e smalltv_ultra
```

Die Bilder im README erzeugt `python3 tools/render_screens.py docs/screens --frame --sheet` aus dem echten Displaycode.
