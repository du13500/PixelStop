# Roadmap

Stand: 09.11.2026. Erledigtes steht in [`CHANGELOG.md`](CHANGELOG.md).

## Nächste Prüfungen auf dem Gerät

Aktueller Hardwaretest und dauerhafte Prüfungen stehen in [`TESTPLAN.md`](TESTPLAN.md).

## Geplante Versionen

| Version | Inhalt |
| --- | --- |
| **v0.5.0** | PixelStop, Deutsch/Englisch, Fahrtziele, neue README – auf Hardware bestätigt |
| **v0.5.1** | Webinterface aufgeräumt, „Jetzt anzeigen“, 3-Tage-Vorhersage, Ort der Haltestelle aus der VVO-Suche, Texte im Flash – auf Hardware bestätigt |
| **v0.5.2** | Radar-Werte zwischen den Abrufen weitergerechnet, Statuspunkt, Hilfen zum adsb.lol-Schlüssel – auf Hardware bestätigt |
| **v0.5.3** | Menü, Vorhersage als Ansicht, Typenliste, Radar-Optionen und Filter, Notfälle, kompakter Log, Sigma-Delta-Test – in Arbeit, nach dem Hardwaretest als Release taggen |
| **v0.5.4** | Warnungen: Bevölkerungsschutz (NINA) und DWD-Wetterwarnungen, einzeln aktivierbar, regional gefiltert, mehrere nacheinander; optional Unterbrechung der Rotation |
| **v0.6.0** | Abfahrten über Anbieter: VVO, Transitous (deutschlandweit inkl. Bahn), danach weitere Verbünde; Haltestellensuche im Browser. Dazu Ausweichquellen fürs Radar (airplanes.live, adsb.fi), wenn adsb.lol pausiert oder das Kontingent aufgebraucht ist |
| **v0.7.0** | Optionaler moderner Anzeigestil mit echten Schriften und neuem Layout, vorher als Entwurf mit `tools/render_screens.py` |

## Ideen für später

- **Freitext und To-do-/Einkaufsliste** mit Erledigt-Funktion und automatischem Seitenwechsel (wenig Aufwand, kann jederzeit dazwischen).
- **Fotos** (einzelne JPGs aus dem Dateisystem), eventuell eigene Wetterbilder.
- **Eigene Ersetzungen für Fahrtziele** im Webinterface.
- **Englische Diagnosetexte.**
- **Variante „Wetter & Uhr“** aus demselben Code: ohne Abfahrten, Radar und Tracker, dafür eine moderne Wetterkachel mit Meteocons (MIT) losgelöst vom Pixel-Design.
- **Mini-Radarbild:** die nächsten Flugzeuge als Punkte auf einem Radarkreis (nutzt die Abfrage aller Flugzeuge aus v0.5.3).
- **Eigener ADS-B-Empfänger als Datenquelle** im Heimnetz (Balkon oder Hellersiedlung): Aktualisierung jede Sekunde ohne Limits.
- **Titelbild für README und GitHub-Vorschau** (KI-Motiv war technisch nicht stimmig, vorerst zurückgestellt).
- **ESP32-Umbau** des SmallTV mit vorhandenem Display als Experiment (Pinbelegung, Stromversorgung, Platz, Portierung).

## Bewusst verworfen

- **Flughafen-Ankunfts- und Abflugtafel:** keine passende freie Datenquelle; aviationstack ist im kostenlosen Tarif zu knapp.
- **Animiertes Regenradar:** Bilddaten sind für Speicher und Netzwerk des ESP8266 zu groß.
- **Eigenes DB-Programm:** geht in den Anbietern von v0.6.0 auf.
- **Zusätzliche Zugangssperre** fürs Webinterface.
- **Zweites, privates Repository:** nicht nötig, solange alles unter MIT auf freien Diensten aufbaut.
