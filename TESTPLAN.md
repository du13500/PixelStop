# Testplan

Stand: v0.5.4.4-dev (16.11.2026). Dieser Plan wird mit jeder Version aktualisiert; frühere Pläne stehen in der Git-Historie, Änderungen in [`CHANGELOG.md`](CHANGELOG.md).

## Hardwaretest v0.5.4.4-dev

1. **Radar über mehrere Stunden:** Im Log kommen regelmäßig „Radar.Position | OK“-Zeilen. Das „Radar-Intervall“ im Status liegt bei 10–27 s (nicht mehr bei Minuten). Scheitert ein Abruf, kommt der nächste Versuch nach 45 s.
2. **Gescheiterte Abrufe** zeigen im Log eine Zeile „Adresse“ mit der Server-IP und danach „TLS-Fehler“. Erwartung, solange adsb.lol den Fehler hat: Fehler mit 159.195.60.218, Erfolge mit 159.195.81.163. Beide Adressen sind die von adsb.lol, nicht deine.
3. **Status:** `flightTlsRx` steht fest auf 4096; nach einem Neustart vergehen bis zum ersten Radarabruf keine 10 s Puffertest mehr.
4. **Keys löschen:** Unter *Flugradar* bzw. *Flug-Tracker* erscheint „Gespeicherten Key löschen“ nur mit gespeichertem Key; Abbrechen ändert nichts, Löschen entfernt nur den Key (ein vorher geändertes anderes Feld bleibt ungespeichert im Formular stehen). Danach im Status `adsbKey: false`.
5. **Kein Dimmverfahren mehr** unter *Anzeige*; Helligkeit 1–100 % per PWM, FRITZ!Box zeigt für „pixelstop“ wieder um 72/30 Mbit/s, `ping` ohne Ausfälle. Aufblitzen nachts bei geringer Helligkeit weiter beobachten (offen, neuer Ansatz nötig).
6. **Nach dem Einschalten:** keine Zeilen „WLAN | getrennt/verbunden“ in den ersten Sekunden. Kommt nach dem Start gar keine Antwort, steht nach etwa 2 Minuten „WLAN | neu verbinden“ im Log (nicht erst nach 10).
7. **Abfahrten:** WLAN der FRITZ!Box kurz abschalten – die Minuten zählen weiter herunter, abgefahrene Bahnen zeigen „-“, Fußzeile „LIVE / LETZTER STAND“.
8. **3-Tage-Vorhersage:** bei nicht erreichbarem Open-Meteo „LIVE / LETZTER STAND“ statt „OPEN-METEO LIVE“.
9. Aus v0.5.4.3 weiter offen: Zähler über Neustart (Status vor/nach *Neustart*), Kontingent mit dem adsb.lol-Schlüsselportal vergleichen, Namensdialog bei Ortswechsel, Speichern ohne geänderten Hostnamen startet nicht neu, Firmware-Update über die eigene Seite.

## Dauerhafte Prüfungen

- **Flugradar:** Langzeittest über mehrere Tage mit Rotation; Verbrauch je Fenster im Status mit dem adsb.lol-Schlüsselportal vergleichen (`src/AdsbQuota.h`). Freien Speicher während der Abfrage im Blick behalten (ca. 5 KB bei 4-KB-TLS-Puffer).
- **Wetter:** Exception 29 beim Programmwechsel ist seit den Speicherkorrekturen nicht wieder aufgetreten; im Langzeittest weiter beobachten.
- **Zeitzone, Sommerzeit, Nachtmodus:** spätestens bei der Zeitumstellung Ende März prüfen.
- **Abfahrten:** zehn Abfahrten mit Seitenwechsel 5+5, Verhalten bei Verbindungsproblemen.
- **Flug-Tracker** mit gültigem aviationstack-Schlüssel vollständig testen.

**Bei Fehlern:** HTTP 429 mit Pause und anschließender Erholung ist kein Neustart. Nach einem Neustart sofort unter *System → Diagnose* den Log laden und die ELF-Datei des installierten Builds sichern (Begleitpaket des Builds). Gleicher Reset-Text und RTC-Marker in späteren Logs desselben Starts bedeuten keinen erneuten Absturz; der RTC-Marker ist ein Hinweis, kein Ursachenbeweis.

## Stand des letzten Builds

v0.5.4.3-dev, GitHub Actions, PlatformIO mit ESP8266-Framework 3.1.2 und gepinnten Bibliotheken:

| | belegt | von | Anteil |
| --- | --- | --- | --- |
| Statischer RAM | 51.352 Byte | 81.920 Byte | 62,7 % |
| Flash | 724.771 Byte | 1.044.464 Byte | 69,4 % |

Das komprimierte Webinterface spart gegenüber v0.5.3.1 rund 42 KB Flash. `tools/check_ram.py` lässt den Build ab 51.600 Byte statischem RAM scheitern, damit für TLS-Abrufe genug Speicher frei bleibt.

## Lokale Prüfungen

Dieselben Prüfungen führt der Workflow [`build.yml`](.github/workflows/build.yml) bei jedem Push aus.

```sh
python3 tools/check.py
python3 tools/test_radar.py
python3 tools/test_radar_schedule.py
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
python3 tools/test_geo_transfer.py
python3 tools/test_network_guard.py
pio run -e smalltv_ultra
```

Nach Änderungen an `src/webui.html` oder `tools/webui_en.py`: `python3 tools/webui_build.py` (erzeugt `src/webui_html.h`; `check.py` vergleicht).

Die Bilder im README erzeugt `python3 tools/render_screens.py docs/screens --frame --sheet` aus dem echten Displaycode.
