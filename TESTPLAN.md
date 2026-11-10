# Testplan

Stand: v0.5.4.1-dev (10.11.2026). Dieser Plan wird mit jeder Version aktualisiert; frühere Pläne stehen in der Git-Historie, Änderungen in [`CHANGELOG.md`](CHANGELOG.md).

## Hardwaretest v0.5.4.1-dev

0. **v0.5.4.1:** Nach dem Flashen und nach mehreren Neustarts verbindet sich der Würfel wieder in wenigen Sekunden (kein Setup-Hotspot, kein Neustart nach ~83 s); Radarabrufe wieder um 1–2 s; Speichern ohne geänderten Hostnamen startet nicht neu.

1. **Radar-Intervall danach selbst auf 10 s stellen** (5 s gibt es nicht mehr). Unter *System → Status* stehen „Radar-Intervall“ und „adsb.lol: Stunde … · Tag … · Woche … · Monat …“. Die Zahlen mit dem adsb.lol-Schlüsselportal vergleichen; **kein 429** im Log, auch nicht über Stunden mit Radar in der Rotation.
2. **Zähler über Neustart:** *Neustart* drücken, Status erneut ansehen – die Zähler laufen weiter. Nach Stecker ziehen höchstens die letzten 10 Minuten verloren.
3. **Webinterface:** Reiter unter dem Titel, auf dem Handy zwei zentrierte Reihen; Seite lädt auch während laufender Radarabrufe vollständig (mehrmals neu laden, auch auf Englisch).
4. **Anzeigename:** Radar- oder Wetterort um mehr als 5 km verschieben → Hinweis „Leeren/Behalten“; kleine Korrekturen ohne Hinweis.
5. **Firmware-Update** über *System → Firmware-Update* (eigene Seite, jetzt zweisprachig). Danach startet der Würfel normal, nicht im Rettungsmodus.
6. **Schutz:** `curl http://<IP>/api/status` funktioniert; ein Aufruf über einen fremden Namen (z. B. per `curl -H "Host: evil.example" http://<IP>/api/status`) liefert 403 und im Log „Web | abgelehnt“.
7. **Stromausfall:** WLAN der FRITZ!Box kurz ausschalten, Würfel neu starten → Setup-Hotspot mit „sucht gespeichertes WLAN“; WLAN wieder einschalten → nach spätestens etwa einer Minute normaler Start. Mit einem Gerät im Hotspot bleibt er dort.
8. **Netzwerk-Hänger** (falls er wieder auftritt): vor dem Stecker ziehen in der FRITZ!Box nachsehen, ob „pixelstop“ aktiv ist, und `ping <IP>` versuchen. Nach 10 Minuten sollte die Firmware selbst neu verbinden („WLAN | neu verbinden“ im Log), nach weiteren 5 Minuten neu starten.
9. **Routen:** Flüge im Sinkflug nahe ORD dürfen ORD nicht als Start zeigen; verworfene Routen stehen mit Grund im Log.
10. **Airline-Namen:** lange Namen wechseln im 3-s-Takt (z. B. „Air Mobility“ / „Command“), Marken wie KLM, SAS, UPS Airlines, FedEx, Volaris erscheinen kurz.
11. **Wetter:** „5°C“ ohne Lücke zwischen Zahl und Einheit; zweistellige und negative Werte bleiben mittig.
12. **Dimmverfahren:** Sigma-Delta nachts bei 1–10 % weiter beobachten.

## Dauerhafte Prüfungen

- **Flugradar:** Langzeittest über mehrere Tage mit Rotation; Verbrauch je Fenster im Status mit dem adsb.lol-Schlüsselportal vergleichen (`src/AdsbQuota.h`). Freien Speicher während der Abfrage im Blick behalten (ca. 5 KB bei 4-KB-TLS-Puffer).
- **Wetter:** Exception 29 beim Programmwechsel ist seit den Speicherkorrekturen nicht wieder aufgetreten; im Langzeittest weiter beobachten.
- **Zeitzone, Sommerzeit, Nachtmodus:** spätestens bei der Zeitumstellung Ende März prüfen.
- **Abfahrten:** zehn Abfahrten mit Seitenwechsel 5+5, Verhalten bei Verbindungsproblemen.
- **Flug-Tracker** mit gültigem aviationstack-Schlüssel vollständig testen.

**Bei Fehlern:** HTTP 429 mit Pause und anschließender Erholung ist kein Neustart. Nach einem Neustart sofort unter *System → Diagnose* den Log laden und die ELF-Datei des installierten Builds sichern (Begleitpaket des Builds). Gleicher Reset-Text und RTC-Marker in späteren Logs desselben Starts bedeuten keinen erneuten Absturz; der RTC-Marker ist ein Hinweis, kein Ursachenbeweis.

## Stand des letzten Builds

v0.5.4-dev, GitHub Actions, PlatformIO mit ESP8266-Framework 3.1.2 und gepinnten Bibliotheken:

| | belegt | von | Anteil |
| --- | --- | --- | --- |
| Statischer RAM | 51.328 Byte | 81.920 Byte | 62,7 % |
| Flash | 723.775 Byte | 1.044.464 Byte | 69,3 % |

Das komprimierte Webinterface spart gegenüber v0.5.3.1 rund 42 KB Flash. `tools/check_ram.py` lässt den Build ab 51.600 Byte statischem RAM scheitern, damit für TLS-Abrufe genug Speicher frei bleibt.

## Lokale Prüfungen

Dieselben Prüfungen führt der Workflow [`build.yml`](.github/workflows/build.yml) bei jedem Push aus.

```sh
python3 tools/check.py
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
python3 tools/test_geo_transfer.py
python3 tools/test_network_guard.py
pio run -e smalltv_ultra
```

Nach Änderungen an `src/webui.html` oder `tools/webui_en.py`: `python3 tools/webui_build.py` (erzeugt `src/webui_html.h`; `check.py` vergleicht).

Die Bilder im README erzeugt `python3 tools/render_screens.py docs/screens --frame --sheet` aus dem echten Displaycode.
