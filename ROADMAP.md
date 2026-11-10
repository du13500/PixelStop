# Roadmap

Stand: 10.11.2026. Erledigtes steht in [`CHANGELOG.md`](CHANGELOG.md).

## Nächste Prüfungen auf dem Gerät

Aktueller Hardwaretest und dauerhafte Prüfungen stehen in [`TESTPLAN.md`](TESTPLAN.md).

## Geplante Versionen

| Version | Inhalt |
| --- | --- |
| **v0.5.0** | PixelStop, Deutsch/Englisch, Fahrtziele, neue README – auf Hardware bestätigt |
| **v0.5.1** | Webinterface aufgeräumt, „Jetzt anzeigen“, 3-Tage-Vorhersage, Ort der Haltestelle aus der VVO-Suche, Texte im Flash – auf Hardware bestätigt |
| **v0.5.2** | Radar-Werte zwischen den Abrufen weitergerechnet, Statuspunkt, Hilfen zum adsb.lol-Schlüssel – auf Hardware bestätigt |
| **v0.5.3** | Radar-Optionen und Filter, Notfälle, Typenliste, kompakter Log, Sigma-Delta-Test – auf Hardware im Einsatz |
| **v0.5.4** | Kontingent-Wächter für adsb.lol, Schutz gegen fremde Webseiten, Rettungsmodus, Wiederverbinden nach Stromausfall, Netzwerk-Wächter, Reiter statt Menü, komprimiertes Webinterface, Airline-Namen, Routenprüfung im Sinkflug – in Arbeit; nach dem Hardwaretest **erstes Release-Tag** |
| **v0.5.5** | **Ausweichquellen fürs Radar:** airplanes.live und adsb.fi (ohne Schlüssel, 1 Anfrage/s, nicht-kommerziell) übernehmen, wenn das adsb.lol-Kontingent aufgebraucht ist; kleiner Radius gegen große Antworten. Vorher Antwortgrößen auf dem Würfel messen sowie FlyItalyADSB und hexdb.io prüfen. Anzeige der Quelle im Webinterface |
| **v0.5.6** | Warnungen: Bevölkerungsschutz (NINA) und DWD-Wetterwarnungen, einzeln aktivierbar, regional gefiltert, mehrere nacheinander; optional Unterbrechung der Rotation |
| **Varianten** | Mehrere Firmware-Varianten aus einem Repository (Sammelname offen): **PixelStop** (Abfahrten, Wetter, Uhr, Countdowns), **PixelFlights** (Flugradar und Tracker, mit großer Airline-Liste ~1.000 Einträge, OpenFlights/ODbL), eventuell eine Komplett-Variante. Gemeinsamer Code, getrennte Builds und Releases |
| **v0.6.0** | Abfahrten über Anbieter: VVO, Transitous (deutschlandweit inkl. Bahn), danach weitere Verbünde; Haltestellensuche im Browser |
| **v0.7.0** | Optionaler moderner Anzeigestil mit echten Schriften und neuem Layout, vorher als Entwurf mit `tools/render_screens.py` |
| **v1.0** | Erste stabile Version, siehe Kriterien unten |

## Kriterien für v1.0

- Alle Ansichten mehrere Wochen ohne Neustart im Dauerbetrieb, auch mit Radar in der Rotation.
- Kein 429 von adsb.lol im Dauerbetrieb; Ausweichquellen getestet.
- Aufblitzen der Hintergrundbeleuchtung gelöst (Sigma-Delta oder Alternative).
- Varianten getrennt baubar, jeweils mit Release, Prüfsumme und Begleitpaket.
- README, Testplan und CONTRIBUTING aktuell; Flug-Tracker vollständig auf Hardware getestet.

## Veröffentlichung

- Vorstellung in passenden Communities (SmallTV/GeekMagic, ESP8266, ADS-B).
- Anfrage, PixelStop in die Liste verwandter Projekte von smalltv-mod aufzunehmen.
- `FUNDING.yml` mit Buy Me a Coffee; Hinweis, dass die Datenquellen nur nicht-kommerziell genutzt werden dürfen.

## Ideen für später

- **Freitext und To-do-/Einkaufsliste** mit Erledigt-Funktion und automatischem Seitenwechsel (wenig Aufwand, kann jederzeit dazwischen).
- **Fotos** (einzelne JPGs aus dem Dateisystem), eventuell eigene Wetterbilder.
- **Eigene Ersetzungen für Fahrtziele** im Webinterface.
- **Englische Diagnosetexte.**
- **Variante „Wetter & Uhr“** aus demselben Code: ohne Abfahrten, Radar und Tracker, dafür eine moderne Wetterkachel mit Meteocons (MIT) losgelöst vom Pixel-Design.
- **Mini-Radarbild:** die nächsten Flugzeuge als Punkte auf einem Radarkreis (nutzt die Abfrage aller Flugzeuge aus v0.5.3).
- **Icons in den Reitern** des Webinterfaces (z. B. Lucide, ISC), nur zusammen mit Text – Feinschliff vor v1.0.
- **Eigener ADS-B-Empfänger als Datenquelle** im Heimnetz (Balkon oder Hellersiedlung): Aktualisierung jede Sekunde ohne Limits.
- **Titelbild für README und GitHub-Vorschau** (KI-Motiv war technisch nicht stimmig, vorerst zurückgestellt).
- **Helligkeit nach Sonnenauf- und -untergang** statt fester Nachtzeiten (Zeiten liefert Open-Meteo bereits).
- **Mockups** aus Foto des Würfels und eingesetztem Displaybild (später, nicht eingeplant).
- **ESP32-Umbau** des SmallTV mit vorhandenem Display als Experiment (Pinbelegung, Stromversorgung, Platz, Portierung).

## Bewusst verworfen

- **Flughafen-Ankunfts- und Abflugtafel:** keine passende freie Datenquelle; aviationstack ist im kostenlosen Tarif zu knapp.
- **Animiertes Regenradar:** Bilddaten sind für Speicher und Netzwerk des ESP8266 zu groß.
- **Eigenes DB-Programm:** geht in den Anbietern von v0.6.0 auf.
- **Zusätzliche Zugangssperre** fürs Webinterface.
- **Zweites, privates Repository:** nicht nötig, solange alles unter MIT auf freien Diensten aufbaut.
