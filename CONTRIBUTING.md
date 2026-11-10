# Mitmachen bei PixelStop

Danke für Fehlerberichte und Verbesserungsvorschläge. PixelStop ist eine eigenständige Firmware für den GeekMagic SmallTV-Ultra; Hardware- und Displaygrundlage stammen aus [smalltv-mod](https://github.com/giovi321/smalltv-mod).

## Unterstützte Hardware

Getestet: **GeekMagic SmallTV-Ultra mit ESP8266 / ESP-12F, 4 MB Flash und 240 × 240 Pixel ST7789**. Andere Geräte und Displayvarianten sind nicht getestet.

## Projektaufbau

| Ort | Inhalt |
| --- | --- |
| `src/main.cpp` | Ansichten, Einstellungen, Webserver, WLAN |
| `src/webui.html` | Webinterface (Deutsch); `tools/webui_en.py` enthält die englischen Texte, `tools/webui_build.py` packt beides gzip-komprimiert nach `src/webui_html.h` |
| `src/*.h` ohne Arduino-Abhängigkeit | Regeln, die auf dem Rechner getestet werden (z. B. `AdsbQuota.h`, `RoutePolicy.h`, `WebGuard.h`, `AirlineName.h`) |
| `tools/test_*.py`, `tests/` | Prüfungen; viele kompilieren den echten Firmwarecode mit Platzhaltern für Netzwerk und Display |
| `tools/render_screens.py` | Bilder für die README aus dem echten Displaycode |

## Bauen und prüfen

```sh
pio run -e smalltv_ultra          # Firmware: .pio/build/smalltv_ultra/firmware.bin
python3 tools/webui_build.py      # nach Änderungen am Webinterface
```

Alle Prüfungen stehen in [`TESTPLAN.md`](TESTPLAN.md) unter „Lokale Prüfungen“; der Workflow [`build.yml`](.github/workflows/build.yml) führt sie bei jedem Pull Request aus und baut die Firmware. Der Build scheitert, wenn der statische RAM über 51.600 Byte steigt (`tools/check_ram.py`) – während TLS-Abrufen bleiben sonst zu wenig Kilobyte frei.

## Änderungen beitragen

- Pixelstil und funktionierende Standardansichten erhalten; neue Optionen mit kompatiblen Standardwerten.
- **Speicher:** feste Texte und Tabellen in den Flash (`PROGMEM`, `F()`, `PSTR()`), keine großen statischen Puffer.
- **Datenquellen schonen:** Grenzen und Bedingungen der Anbieter einhalten (adsb.lol-Kontingent in `src/AdsbQuota.h`), keine Routen, Zeiten oder Statuswerte erfinden.
- Neue Webtexte brauchen einen englischen Eintrag in `tools/webui_en.py`; `tools/check.py` meldet fehlende.
- Hardwaretests ehrlich kennzeichnen; README, `CHANGELOG.md` und `TESTPLAN.md` mit dem Verhalten aktualisieren.
- Größere Ideen zuerst in einem Issue besprechen.

## Fehler melden

Bitte Firmwareversion, betroffene Ansicht, Schritte zum Nachstellen, erwartetes und tatsächliches Verhalten sowie den Log (*System → Diagnose*, notfalls `http://<IP>/api/log`) angeben. Fotos helfen. **Keine WLAN-Passwörter, API-Schlüssel oder vollständigen Konfigurationsdateien veröffentlichen.**

## Lizenz und Datenquellen

Projektcode: [MIT](LICENSE). Für Bibliotheken, Dienste und Daten gelten ihre eigenen Bedingungen, siehe [`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md) und [`THIRD_PARTY_SERVICES.md`](THIRD_PARTY_SERVICES.md). Die Datenquellen sind nur für nicht-kommerzielle Nutzung gedacht.
