# 🚏 PixelStop

<a href="README.en.md"><img src="docs/flags/en.svg" height="14" alt=""> <b>English version →</b></a>

**Abfahrten, Wetter, Flugradar, Uhr und Countdowns für den GeekMagic SmallTV-Ultra.**
Gekauft als kleiner WLAN-Würfel mit eher bescheidener Werksfirmware – und von Anfang an mit dem Plan, etwas Eigenes daraus zu machen.

[![Build](https://github.com/du13500/pixelstop/actions/workflows/build.yml/badge.svg)](https://github.com/du13500/pixelstop/actions/workflows/build.yml)
![Version](https://img.shields.io/badge/version-v0.5.4.5--dev-orange)
![Hardware](https://img.shields.io/badge/hardware-ESP8266%20%C2%B7%20SmallTV--Ultra-blue)
![Hardwaretest](https://img.shields.io/badge/hardwaretest-v0.5.4.5%20offen-yellow)
![Hobbyprojekt](https://img.shields.io/badge/hobbyprojekt-nicht%20kommerziell-lightgrey)
![Lizenz](https://img.shields.io/badge/lizenz-MIT-green)
![Mission](https://img.shields.io/badge/mission-Know%20before%20you%20go-ff5d56)

<p align="center">
<img src="docs/screens/de/kachel-abfahrten.png" width="258" alt="Abfahrten">
<img src="docs/screens/de/kachel-wetter.png" width="258" alt="Wetter">
<img src="docs/screens/de/kachel-vorhersage.png" width="258" alt="3-Tage-Vorhersage">
<img src="docs/screens/de/kachel-flugradar.png" width="258" alt="Flugradar">
<img src="docs/screens/de/kachel-flugtracker.png" width="258" alt="Flug-Tracker">
<img src="docs/screens/de/kachel-uhr.png" width="258" alt="Uhr">
<img src="docs/screens/de/kachel-countdown-tage.png" width="258" alt="Countdown">
</p>
<p align="center"><sub>Echte Ausgabe der Firmware mit Beispieldaten · mehr Bilder und Erklärungen unter <a href="#programme">Programme</a></sub></p>

## Was ist PixelStop?

PixelStop ist eine eigene, deutschsprachige (und englische) Firmware für den **GeekMagic SmallTV-Ultra**, einen 240 × 240-Pixel-Würfel mit ESP8266. Nach der Einrichtung braucht er nur USB-Strom und WLAN – **kein eigener Server, kein Raspberry Pi**. Alle Einstellungen laufen über ein Webinterface im Browser.

Er zeigt im Wechsel oder dauerhaft sieben Programme: Abfahrten, Wetter, 3-Tage-Vorhersage, Flugradar, Flug-Tracker, Uhr und bis zu vier Countdowns. Dazu kommen Nachtmodus, Helligkeit ab 1 %, Ortssuche mit Karte, griechische und kyrillische Zeichen sowie eine Diagnoseansicht.

## Programme

Zum Aufklappen: was jedes Programm zeigt, was sich einstellen lässt und alle Beispielbilder. Die Bilder sind echte Ausgaben der Firmware mit **Beispieldaten**; bewegte Bilder laufen in Echtzeit, eine Sekunde pro Bild.

<details><summary><b>🚏 Abfahrten</b></summary>

Live-Abfahrten einer Haltestelle des Verkehrsverbunds Oberelbe (VVO): Linie, Ziel, Minuten bis zur Abfahrt, Verspätung (gelb) und Ausfall (✕). Drei bis fünf Abfahrten auf einer Seite oder zehn auf zwei Seiten im Wechsel alle 5 Sekunden, Filter nach Verkehrsmittel. Fahrtziele lassen sich ohne den eigenen Ort und abgekürzt anzeigen. Die Minuten zählen zwischen den Abrufen herunter – auch dann, wenn der VVO gerade nicht erreichbar ist.

<p align="center">
<img src="docs/screens/de/abfahrten.png" width="258" alt="Abfahrten: Dresden Hbf mit Verspätung und Ausfall">
<img src="docs/screens/de/abfahrten-ziele.png" width="258" alt="Kurze Fahrtziele">
<img src="docs/screens/de/abfahrten-seiten.gif" width="258" alt="Zehn Abfahrten auf zwei Seiten">
</p>

</details>

<details><summary><b>🌦️ Wetter</b></summary>

Aktuelle Temperatur, Wetterbild für Tag und Nacht, Höchst- und Tiefstwert, Regenwahrscheinlichkeit sowie Sonnenauf- und -untergang, in °C oder °F. Der Ort wird über eine Suche, die Karte oder Koordinaten gewählt; ein eigener Anzeigename ist möglich.

<p align="center">
<img src="docs/screens/de/wetter.png" width="258" alt="Wetter: Dresden am Tag">
<img src="docs/screens/de/wetter-nacht.png" width="258" alt="Wetter: Dresden nachts">
</p>

</details>

<details><summary><b>📅 3-Tage-Vorhersage</b></summary>

Wochentag, Wetterbild, Höchst- und Tiefstwert und Regenwahrscheinlichkeit für die nächsten drei Tage – als eigene Ansicht in der Rotation.

<p align="center">
<img src="docs/screens/de/vorhersage.png" width="258" alt="3-Tage-Vorhersage für Dresden">
</p>

</details>

<details><summary><b>✈️ Flugradar</b></summary>

Das nächste Flugzeug im gewählten Umkreis: Rufzeichen, Airline, Route, Typ, Kennzeichen, Entfernung und Höhe. Zwischen zwei Abrufen rechnet der Würfel Entfernung und Höhe weiter; die Fußzeile zeigt, wie alt der letzte Abruf ist.

Im Reiter **Flugradar** lässt sich die Anzeige anpassen:

- **Betreiber:** Name zum Rufzeichen (Standard), Oberbegriff oder beides, z. B. „US Air Force / AMC“ – für bekannte Militär-Rufzeichen.
- **Geschwindigkeit** in km/h oder Knoten, alle 3 Sekunden im Wechsel mit dem Kennzeichen.
- **▲▼** zwischen Entfernung und Höhe bei Steig- oder Sinkflug ab 300 ft/min.
- **MIL** neben dem Rufzeichen für Militärflugzeuge.
- **Ausblenden:** Hubschrauber, Kleinflugzeuge, Segler/Ballone/Ultraleicht/Drohnen, Flugzeuge am Boden.
- **Immer aktiv:** Notfälle (Squawk 7500/7600/7700) mit rotem Rufzeichen und roter Routenzeile, in der Route und „SQUAWK 7700“ alle 3 s wechseln; zu lange Airline-Namen wechseln ebenfalls alle 3 s.

<p align="center">
<sub><b>Flugbewegungen</b></sub><br>
<img src="docs/screens/de/flugradar.gif" width="258" alt="Flugradar: Emirates A380 über Dresden">
<img src="docs/screens/de/radar-steigflug.gif" width="258" alt="Steigflug: ALROSA Tu-154M ab Mirny">
<img src="docs/screens/de/radar-sinkflug.gif" width="258" alt="Sinkflug: European Air Charter MD-82 nach Varna">
</p>

<p align="center">
<sub><b>Anzeige</b></sub><br>
<img src="docs/screens/de/radar-name.gif" width="258" alt="Langer Airline-Name im Wechsel">
<img src="docs/screens/de/radar-boden.gif" width="258" alt="Flugzeug am Boden">
<img src="docs/screens/de/radar-leer.png" width="258" alt="Kein Flugzeug im Radius">
</p>

<p align="center">
<sub><b>Datenstand</b></sub><br>
<img src="docs/screens/de/radar-gelb.gif" width="258" alt="Letzter Stand nach fehlgeschlagenem Abruf">
<img src="docs/screens/de/radar-pause.gif" width="258" alt="Abrufpause">
<img src="docs/screens/de/radar-grau.gif" width="258" alt="Keine Daten mehr">
</p>

<p align="center">
<sub><b>Militär und Notfall · fiktive Daten</b></sub><br>
<img src="docs/screens/de/radar-colone.gif" width="258" alt="Geschwindigkeit (fiktiv)">
<img src="docs/screens/de/radar-apollo.gif" width="258" alt="Militär (fiktiv)">
<img src="docs/screens/de/radar-karma.gif" width="258" alt="Notfall (fiktiv)">
</p>

Die Tu-154M und die MD-82 erinnern an echte letzte Flüge: ALROSA RA-85757, Mirny – Nowosibirsk am 28.10.2020 (letzter ziviler Tu-154-Linienflug in Russland), und European Air Charter LZ-LDP, Hurghada – Varna am 21.10.2023 (letzter Flug der MD-82 bei European Air Charter); Flugnummern und Werte sind nachgestellt. Militär und Notfall zeigen **fiktive Daten**, damit kein echter Flug so dargestellt wird; dort steht in der Fußzeile „DRADIS“ statt „ADSB.LOL“.

| Radar-Fußzeile | Bedeutung |
| --- | --- |
| `● ADSB.LOL LIVE 4s` | letzter erfolgreicher Abruf vor 4 Sekunden |
| `LETZTER STAND 16s` | Abruf fehlgeschlagen, vorheriger Stand wird weiter gezeigt |
| `ABRUFPAUSE 46s` | adsb.lol bittet um eine Pause; Alter des letzten Abrufs |
| `KEIN FLUGZEUG` | Abfrage erfolgreich, aber nichts im Umkreis |
| `ADSB.LOL / Zugriff abgelehnt` | adsb.lol lehnt ab (401/403/426); lange Pause statt ständiger Wiederholung |

| Punkt in der Fußzeile | Bedeutung |
| --- | --- |
| ⚪ kurz weiß | gerade neue Daten empfangen |
| 🟢 grün | Abrufe im normalen Takt; Entfernung und Höhe werden dazwischen weitergerechnet |
| 🟡 gelb | Abruf überfällig, fehlgeschlagen oder Anbieterpause; Werte sind geschätzt |
| ⚫ grau | zu lange keine Daten; Entfernung und Höhe stehen still |

</details>

<details><summary><b>🛫 Flug-Tracker</b></summary>

Ein bestimmter Flug mit Plan- und Ist-Zeiten, Status, Flugzeit sowie Live-Höhe und -Geschwindigkeit. Dafür ist ein eigener aviationstack-Schlüssel nötig.

<p align="center">
<img src="docs/screens/de/flugtracker.png" width="258" alt="Flug-Tracker LH1178">
</p>

</details>

<details><summary><b>🕰️ Uhr</b></summary>

Uhrzeit und Datum mit wählbarer Zeitzone; die Uhrzeit steht außerdem in der Fußzeile der anderen Ansichten.

<p align="center">
<img src="docs/screens/de/uhr.png" width="258" alt="Uhr mit Datum">
</p>

</details>

<details><summary><b>⏳ Countdown 1–4</b></summary>

Vier unabhängige Countdowns in Tagen, in Stunden : Minuten : Sekunden oder in Minuten : Sekunden, jeweils mit eigenem Titel und eigenen Farben. Die Ziffern sehen aus wie bei einem LED-Radiowecker, nicht leuchtende Segmente glimmen schwach mit.

- **Darstellung:** „Klassisch“ oder „HUD“ mit Rahmen, „T-MINUS“/„T-PLUS“ und Fortschrittsbalken.
- **Vorzeichen:** „−“ vor dem Ziel und „+“ danach – oder ohne Vorzeichen; dann bleibt der Countdown bei null stehen.
- **Startdatum** (Tage): bestimmt den Fortschrittsbalken im HUD; bleibt es leer, wird beim Start der heutige Tag eingetragen.

<p align="center">
<sub><b>Klassisch</b></sub><br>
<img src="docs/screens/de/countdown-tage.png" width="258" alt="Countdown in Tagen">
<img src="docs/screens/de/countdown-timer.gif" width="258" alt="Countdown-Timer">
<img src="docs/screens/de/countdown-plus.gif" width="258" alt="Countdown nach dem Ziel">
</p>

<p align="center">
<sub><b>HUD</b></sub><br>
<img src="docs/screens/de/countdown-tage-hud.png" width="258" alt="HUD in Tagen">
<img src="docs/screens/de/countdown-hud.gif" width="258" alt="HUD-Timer">
<img src="docs/screens/de/countdown-hud-null.png" width="258" alt="HUD am Ziel ohne Vorzeichen">
</p>

</details>

## Entwicklungsstand: v0.5.4.5-dev

| Bereich | Stand |
| --- | --- |
| Abfahrten (VVO), Wetter, Uhr, Countdowns | ✅ im Alltag stabil |
| Flugradar über HTTP/1.1, Pausen bei abgelehnten Anfragen | ✅ auf Hardware bestätigt (v0.4.14.7) |
| Radar nur bei sichtbarer Ansicht | ✅ läuft auf Hardware (v0.4.14.8) |
| Kontingent-Wächter für adsb.lol (30 s bis Monat), Zähler über Neustarts | ⏳ Hardwaretest offen (v0.5.4) |
| Hintergrundbeleuchtung bei 1 % ohne Aufblitzen | ⚠️ gelegentliches Aufblitzen; der Versuch mit Sigma-Delta legte das WLAN lahm und ist entfernt, neuer Ansatz offen |
| Sprachumschaltung Deutsch/Englisch, abgekürzte Fahrtziele | ✅ auf Hardware bestätigt (v0.5.0) |
| 3-Tage-Vorhersage, aufgeräumtes Webinterface, „Jetzt anzeigen“, Ort der Haltestelle | ✅ auf Hardware bestätigt (v0.5.1) |
| Radar: Entfernung und Höhe zwischen den Abrufen weitergerechnet, Statuspunkt | ✅ auf Hardware bestätigt (v0.5.2) |
| Radar-Optionen und Filter, Flugzeugtypen, Vorhersage als Ansicht | ✅ läuft auf Hardware (v0.5.3) |
| Schutz gegen fremde Webseiten, Rettungsmodus, Wiederverbinden nach Stromausfall, Netzwerk-Wächter, komprimiertes Webinterface | ⏳ Hardwaretest offen (v0.5.4) |
| Flug-Tracker mit eigenem aviationstack-Schlüssel | ⏳ nicht vollständig auf Hardware getestet |
| Countdown im Radiowecker-Stil, Darstellung „HUD“, Vorzeichen wählbar | ⏳ Hardwaretest offen (v0.5.4.5) |

Alle Änderungen stehen in [`CHANGELOG.md`](CHANGELOG.md), offene Prüfungen in [`ROADMAP.md`](ROADMAP.md).

## So funktioniert es

```
  Browser (Handy/PC)                     PixelStop-Würfel (ESP8266)
 ┌──────────────────────┐   WLAN   ┌──────────────────────────────────┐
 │ Webinterface         │◀────────▶│ Webserver · Einstellungen        │
 │ Ortssuche + Karte ───┼─▶ OSM    │ Abrufe:  VVO ─────── Abfahrten   │
 └──────────────────────┘          │          Open-Meteo ─ Wetter     │
                                   │          adsb.lol ─── Flugradar  │
                                   │          aviationstack  Tracker  │
                                   │ Display 240 × 240 (ST7789)       │
                                   └──────────────────────────────────┘
```

Der Würfel fragt die Dienste selbst ab und zeichnet die Ansichten direkt. Die Ortssuche läuft über den Browser, damit der knappe Arbeitsspeicher des ESP8266 frei bleibt. Das Flugradar fragt nur, solange es auf dem Display steht (plus kurz vor dem Wechsel), und teilt sich ohne eigenen Schlüssel das Tageskontingent von adsb.lol selbst ein. Zwischen zwei Abrufen rechnet es Entfernung und Höhe aus Geschwindigkeit, Kurs und Steigrate weiter, sodass die Werte auch bei seltenen Abrufen lebendig bleiben.

## Unterstütztes Gerät

**GeekMagic SmallTV-Ultra mit ESP8266 / ESP-12F, 4 MB Flash und ST7789-Display (240 × 240).** Andere SmallTV-Modelle sind nicht getestet. Die Firmware ist nicht für ESP32-Geräte bestimmt.

## Installation und Einrichtung

1. Die Firmware auf den SmallTV-Ultra bringen. Der erste Flashweg hängt von der vorhandenen Firmware ab; der USB-Stromanschluss allein ist kein zugesicherter Programmierzugang.
2. Beim ersten Start mit dem WLAN **PixelStop-Setup** verbinden und im Browser **192.168.4.1** öffnen.
3. Unter **System** das WLAN wählen, Passwort eintragen und speichern.
4. Nach dem Neustart das Webinterface über die IP-Adresse auf dem Display öffnen, je nach Netzwerk auch über `pixelstop.local`.
5. Unter **System → Sprache / Language** Deutsch oder Englisch wählen.
6. Unter **Anzeige** Programme, Wechselzeiten, Zeitzone, Helligkeit und Nachtmodus einstellen, danach Haltestelle, Orte und Timer in den jeweiligen Reitern.

> **Hinweis:** Wetter und Flugradar haben unabhängige Standorte (Ortssuche, Karte oder Koordinaten). Ein optionaler Anzeigename ersetzt den Ortsnamen auf dem Display. Ein leeres WLAN-Passwortfeld behält das gespeicherte Passwort.

**Updates** laufen über **System → Firmware-Update** mit der passenden `.bin`. Die Einstellungen bleiben dabei erhalten.

## Firmware bauen

Der Workflow [`build.yml`](.github/workflows/build.yml) baut die Firmware bei jedem Push und Pull Request; das Ergebnis liegt unter **Actions** als Artefakt `pixelstop-v<Version>-firmware`. Ein Tag wie `v0.5.0` (passend zu `FW_VERSION` in [`src/Version.h`](src/Version.h)) erzeugt zusätzlich ein **GitHub Release** mit BIN-Datei, Prüfsumme und Begleitpaket. Lokal genügt `pio run -e smalltv_ultra`.

Die Screenshots dieser README erzeugt `python3 tools/render_screens.py docs/screens` aus dem echten Displaycode.

## Datenquellen und Grenzen

| Dienst | Wofür | Hinweis |
| --- | --- | --- |
| **VVO** | Abfahrten | Region Dresden/Oberelbe |
| **Open-Meteo** | Wetter | kostenlose API für nichtkommerzielle Nutzung |
| **adsb.lol** | Radarpositionen, Routen | Daten unter ODbL; feste Kontingente pro Stunde, Tag, Woche und Monat (siehe unten) |
| **adsbdb** | Airline, Flugzeugtyp, Routen | eigene Weiterverwendungsbedingungen für Routen |
| **aviationstack** | Flug-Tracker | eigener Schlüssel, Tarifgrenzen |
| **OpenStreetMap / Nominatim** | Karte, Ortsnamen | nur manuelle Suche, über den Browser |

Die Datenquellen sind nur für nicht-kommerzielle Nutzung gedacht; das gilt auch für PixelStop als Hobbyprojekt. Der ESP8266 prüft bei HTTPS-Verbindungen keine Zertifikate (zu wenig Speicher für Zertifikatsketten). Die Verbindungen sind verschlüsselt, aber jemand mit Zugriff auf das eigene Netz könnte sich theoretisch dazwischenschalten und z. B. den adsb.lol-Schlüssel mitlesen.

Radar-Routen stammen aus Zusatzdatenbanken und können trotz Positionsprüfung falsch oder unvollständig sein; unpassende Routen werden nicht durch geratene Flughäfen ersetzt. `N/A` steht für fehlende Daten, `GND` für ein ausdrücklich gemeldetes Flugzeug am Boden. Höhen sind Flughöhen, keine Höhe über Grund. Weitere Hinweise in [`THIRD_PARTY_SERVICES.md`](THIRD_PARTY_SERVICES.md).

### Schlüssel für adsb.lol

adsb.lol vergibt kostenlose Schlüssel; mit Schlüssel hat jedes Gerät sein eigenes Kontingent. Den Schlüssel im Reiter **Flugradar** eintragen, dort stehen auch die Links.

Ein kostenloser Schlüssel erlaubt **5 Anfragen pro 30 s, 20 pro Minute, 150 pro Stunde, 500 pro Tag, 2.000 pro Woche und 5.000 pro Monat** (Fenster in UTC: volle Stunde, Mitternacht, Montag, Monatsanfang). Wiederholtes Überschreiten kann den Schlüssel kosten. PixelStop zählt deshalb jede Anfrage (auch über Neustarts und Stromausfälle hinweg), plant mit Sicherheitsabstand und fragt nur, während das Radar angezeigt wird. Das gewählte Intervall gilt, höchstens aber so oft, wie es die Stunde erlaubt (135 Anfragen, etwa alle 27 Sekunden). Sind Tag, Woche oder Monat verbraucht, pausiert das Radar bis zum nächsten Fenster – Dauerbetrieb rund um die Uhr ist mit adsb.lol allein nicht möglich. Dazwischen werden Entfernung und Höhe weitergerechnet. Verbrauch und tatsächliches Intervall stehen unter **System → Status**.

| Weg | Für wen |
| --- | --- |
| [Anonymer Schlüssel](https://www.adsb.lol/pow/challenge) | ohne Konto und ohne Mail: kleine Rechenaufgabe im Browser, gilt etwa einen Monat, danach einfach neu holen |
| [Schlüssel für Empfänger](https://www.adsb.lol/receiver) | wer selbst einen ADS-B-Empfänger betreibt und an adsb.lol liefert; lange gültig |
| [Schlüssel auf Antrag](https://www.adsb.lol/request) | für alle anderen Projekte, wird von Hand geprüft |

Eine Übersicht aller Wege gibt es unter [adsb.lol/mmb](https://www.adsb.lol/mmb). Lehnt adsb.lol einen Schlüssel ab, ist er meist abgelaufen; das Webinterface weist dann darauf hin.

## Pläne

- **Mehr Regionen:** Abfahrten nicht nur vom VVO, sondern auswählbar je Verkehrsverbund bzw. deutschlandweit (geplant für v0.6.0).
- **Eigene Ersetzungen für Fahrtziele** im Webinterface.
- Englische Diagnosetexte.
- **Radar ehrlich** (v0.5.5): Pause-Bildschirm, wenn eine adsb.lol-Grenze erreicht ist, Anzeige der Reichweite des Kontingents im Webinterface und eine eigene Datenquelle im Heimnetz. Danach **Ausweichquellen** (v0.5.6).

## Diagnose und Fehlerberichte

Unter **System → Diagnose** zeigt **Log laden** die letzten Ereignisse. Nach einem Aussetzer den Log möglichst direkt laden; nach einem Neustart bleiben nur Reset-Grund und letzter Marker erhalten. Ist das Webinterface nicht erreichbar, hilft oft direkt die Adresse `http://<IP>/api/log`.

**Selbsthilfe der Firmware:**

- **Stromausfall:** Startet der Router langsamer als der Würfel, wartet dieser im Setup-Hotspot und versucht das gespeicherte WLAN jede Minute erneut – aber nicht, solange ein Gerät mit dem Hotspot verbunden ist.
- **Netzwerk-Hänger:** 10 Minuten ohne eine einzige Antwort trotz Anfragen führen zu einer neuen WLAN-Verbindung, 5 Minuten später zu einem Neustart (der Marker bleibt erhalten).
- **Rettungsmodus:** Nach drei Abstürzen kurz hintereinander startet PixelStop ohne Abrufe; Webinterface und Firmware-Update funktionieren, auf dem Display steht „RETTUNGSMODUS“. Ein normaler Neustart beendet ihn.
- **Fremde Webseiten** können weder Einstellungen ändern noch eine Firmware einspielen; Werkzeuge wie `curl` im Heimnetz funktionieren weiterhin. Hilfreich für Fehlerberichte sind Firmwareversion, betroffene Ansicht, Schritte zum Nachstellen und ein Foto. Bitte **keine WLAN-Passwörter oder API-Schlüssel** veröffentlichen.

## Credits und Lizenz

- Eigener Projektcode und Dokumentation: **MIT**, Copyright (c) 2026 du13500. Copyright- und Lizenzhinweis bei Weitergabe erhalten.
- Hardware- und Display-Grundlage aus [smalltv-mod](https://github.com/giovi321/smalltv-mod) von giovi321 (WTFPL); der ursprüngliche Hinweis liegt bei.
- Bibliotheken und ihre Lizenzen: siehe [`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md) und [`LICENSES/`](LICENSES).
- Sprachflagge: [English_language.svg](https://commons.wikimedia.org/wiki/File:English_language.svg) von Rei-artur und Kjoonlee, Wikimedia Commons, gemeinfrei.
- Kartendaten © OpenStreetMap-Mitwirkende.

---

**Know before you go. 🚏**

<sub>PixelStop is an independent hobby project and is not affiliated with GeekMagic, VVO or any of the data providers named above.</sub>
