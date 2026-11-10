# Änderungen

## v0.5.4.3-dev

- **Radar fragt wieder Positionen ab:** In v0.5.4 schob jede wiederholte Routenabfrage für dasselbe Flugzeug den nächsten Positionsabruf um ein ganzes Intervall nach hinten. Weil diese Wiederholungen alle 2–5 Minuten kamen und das Kontingent-Intervall länger war, kam nie wieder ein Positionsabruf – das Radar blieb beim alten Flugzeug stehen (nachgestellt: 1 Positionsabruf in 2 Stunden). Jetzt gilt: höchstens eine adsb.lol-Routenabfrage zwischen zwei Positionsabrufen, und nur für ein neues Flugzeug.
- **Wiederholte Abfragen für dasselbe Flugzeug** laufen nur noch über adsbdb (ohne adsb.lol-Kontingent), alle 10 statt alle 2–5 Minuten, und behalten Route und Airline, statt sie bis zur Antwort zu löschen.
- **Flug-Tracker im Kontingent:** Die Live-Abfragen bei adsb.lol (bisher fest alle 15 s) folgen derselben Planung wie das Radar.
- **Hochrechnung bis 10 Minuten** (vorher 4), passend zu den längeren Intervallen.
- **Neue Prüfung** `tools/test_radar_schedule.py`: simuliert zwei Stunden mit den echten Planungsfunktionen (gleiches Flugzeug, neue Flugzeuge, Rotation) und schlägt bei v0.5.4 an.

## v0.5.4.2-dev

- **Netzwerk-Wächter repariert:** In v0.5.4 setzte er sich jede Sekunde selbst zurück, sobald seit dem Start irgendeine Antwort kam. Löste er aus, verband er das WLAN jede Sekunde neu – der Würfel kam nie wieder ins Netz, und der geplante Neustart wurde nie erreicht. Jetzt beendet nur eine *neue* Antwort nach dem Neuverbinden den Alarm; bleibt sie aus, folgt nach 5 Minuten der Neustart. Logik in `src/NetWatch.h` mit Tests.
- **Start-Verbindung:** 30 s statt 18 s, bevor der Setup-Hotspot öffnet. Der Log hält fest, nach wie vielen ms die Funkverbindung und die IP-Adresse kamen (bzw. den WLAN-Status bei Fehler).
- **Anzeigename bei Ortswechsel** als Dialog („Der Anzeigename „…“ passt vielleicht nicht mehr zum neuen Ort.“ – Leeren/Behalten) statt als Hinweis zwischen den Feldern.

## v0.5.4.1-dev

- **WLAN wie in v0.5.3.1:** Das feste Setzen von PHY-Modus und Sendeleistung vor jeder Verbindung (v0.5.4) ist zurückgenommen. Damit fand der Würfel das WLAN beim Start oft nicht innerhalb von 18 s, landete im Setup-Hotspot und startete nach rund 83 s neu; Abrufe dauerten zwei- bis dreimal so lange, das Webinterface reagierte zäh.
- **Kein unnötiger Neustart beim Speichern:** Der Hostname wird beim Laden still kleingeschrieben; ein Unterschied nur in Groß-/Kleinschreibung löst keinen Neustart mehr aus. Vorher konnte das erste Speichern nach dem Update (z. B. ein Ortswechsel) einen Neustart auslösen.
- **Hinweis zum Anzeigenamen** verschwindet, sobald das Feld leer ist.

## v0.5.4-dev

**Flugradar und adsb.lol**
- **Kontingent-Wächter:** Ein kostenloser adsb.lol-Schlüssel erlaubt 5 Anfragen pro 30 s, 20 pro Minute, 150 pro Stunde, 500 pro Tag, 2.000 pro Woche und 5.000 pro Monat; wiederholte 429 können ihn kosten. PixelStop zählt jede Anfrage in diesen Fenstern (UTC, feste Grenzen), plant mit Sicherheitsabstand und verteilt die Anfragen auf die Restzeit jedes Fensters – nur für die Zeit, in der das Radar angezeigt wird. Ohne Schlüssel gilt zusätzlich die bisherige Tagesgrenze. Ersetzt das bisherige Tagesbudget (`src/AdsbQuota.h`).
- **Zähler überstehen Neustarts** (RTC-Speicher) und Stromausfälle (Datei, höchstens alle 10 Minuten geschrieben).
- **Sparmodus:** Eine Routenabfrage bei adsb.lol ersetzt den nächsten Positionsabruf, statt übersprungen zu werden; sie hat Vorrang vor dem nächsten Positionsabruf.
- **Kleinstes Radar-Intervall 10 s** (5 s entfällt). Das gewählte Intervall ist ein Wunschwert; das tatsächliche steht im Status, ebenso der Verbrauch je Fenster.
- **Routenprüfung im Sinkflug:** Sinkt ein Flugzeug deutlich (mehr als 500 ft/min), tief (unter etwa 3.000 m) und nahe einem Flughafen, kann dieser nicht der Start sein (SKW4955 zeigte ORD>MKE statt LEX>ORD). Bewusst keine Regel für den Steigflug wegen Durchstartmanövern.
- **Log:** verworfene Routen mit Grund („verworfen:Anbieter/Position/Sinkflug/Rufzeichen“), ein Radarabruf ohne WLAN oder ohne Speicher steht als „kein WLAN“ bzw. „Heap-knapp“ statt „HTTP 0“ im Log.
- **Airline-Namen:** Markennamen nach Rufzeichen-Präfix haben Vorrang (z. B. KLM, SAS, SWISS, FedEx, UPS Airlines, Volaris, Envoy, SkyWest); andere lange Namen verlieren Zusätze wie „Airlines“ oder „Inc“, passen sie dann noch nicht, wechseln zwei Teile im 3-s-Takt, als letzte Lösung kleine Schrift (`src/AirlineName.h`).
- **Flugzeugtypen:** F-5, F-4, F-14, F-104, Rafale, Mirage 2000, Su-27, MiG-29 sowie Trainer, Privat- und Flugshow-Jets (L-39, L-159, Hawk, Alpha Jet, M-346, MiG-15/17/21, Hunter, P-51, Spitfire u. a.). Lange Typnamen ohne Tabelleneintrag werden allgemein auf „Hersteller Modell“ gekürzt (Northrop, Lockheed, Dassault, Gulfstream und weitere).

**Netz, Sicherheit, Stabilität**
- **Schutz gegen fremde Webseiten** ohne Passwort: Der Host muss den Würfel nennen (gegen DNS-Rebinding), Änderungen werden nur ohne fremden Origin angenommen (gegen CSRF). Gilt für alle POST-Anfragen, `/api/reboot` und ausdrücklich das **Firmware-Update**, das deshalb einen eigenen Handler hat. `curl` im Heimnetz funktioniert weiter.
- **WLAN-Namen** im Suchergebnis werden als Text dargestellt (ein präparierter Netzname konnte bisher HTML einschleusen); Leerzeichen am Ende bleiben erhalten und werden in der Liste als „␣“ sichtbar.
- ~~WLAN-Grundeinstellungen (PHY-Modus, Sendeleistung) vor jeder Verbindung~~ – in v0.5.4.1 zurückgenommen.
- **Nach Stromausfall:** Startet der Router langsamer, sucht der Setup-Hotspot jede Minute das gespeicherte WLAN – nicht, solange ein Gerät mit dem Hotspot verbunden ist – und startet bei Erfolg normal neu.
- **Rettungsmodus:** Nach drei Abstürzen in Folge startet PixelStop ohne Abrufe; Webinterface und Firmware-Update laufen, auf dem Display steht „RETTUNGSMODUS“. Ein normaler Neustart beendet ihn.
- **Netzwerk-Wächter:** 10 Minuten ohne eine einzige Antwort trotz Anfragen (oder ohne WLAN) führen zu einer neuen WLAN-Verbindung, 5 Minuten später zu einem Neustart mit erhaltenem RTC-Marker; nach solchen Neustarts verdoppelt sich die Wartezeit. WLAN-Abbrüche und Neuverbindungen stehen im Log, der Log endet mit dem aktuellen WLAN-Zustand.
- **Hostname** wird geprüft (a–z, 0–9, Bindestrich, 1–32 Zeichen).
- Startbildschirm „WLAN verbinden…“ auf Englisch, wenn Englisch eingestellt ist.

**Webinterface**
- **Reiter statt ☰-Menü:** Text-Reiter wie vor v0.5.3 direkt unter dem Titel, umbrechend statt wischend und mittig zentriert.
- **Komprimiert ausgeliefert:** Seite (60 → 19 KB) und englische Texte (20 → 8 KB) liegen gzip-komprimiert im Flash; während die Seite lädt, pausieren die Hintergrundabrufe. Die Seite steht jetzt in `src/webui.html`, `tools/webui_build.py` erzeugt `src/webui_html.h`.
- **Anzeigename bei Ortswechsel:** Nach einem Wechsel um mehr als 5 km fragt das Webinterface bei Wetter und Radar, ob der Anzeigename bleiben soll („Leeren“/„Behalten“).
- Statuswerte der Firmware erscheinen in der englischen Oberfläche übersetzt; Hinweis zu den adsb.lol-Grenzen aktualisiert.

**Anzeige**
- **„°C“ ohne Lücke:** Zahl und Einheit werden als Gruppe zentriert.

**Build und Doku**
- `test_geo_transfer.py` und `test_network_guard.py` laufen im Build; neue Tests für Kontingent, Schutz, Airline-Namen, Typen und Sinkflug-Regel.
- ROADMAP mit v0.5.5 (Ausweichquellen), v0.5.6 (Warnungen), Varianten, Kriterien für v1.0 und Veröffentlichung; CONTRIBUTING neu; Hinweise zu nicht-kommerzieller Nutzung der Datenquellen und zu HTTPS ohne Zertifikatsprüfung.

## v0.5.3.1-dev

- **Radar: Speicher zurückgeholt.** v0.5.3 belegte 748 Byte mehr festen Arbeitsspeicher als v0.5.2; während einer TLS-Abfrage blieben dadurch zeitweise nur ~4,3 KB frei, und die Antwort ließ sich nicht mehr einlesen („JSON:NoMemory“, Radar ohne Daten). Die Militär-Tabelle liegt jetzt im Flash, ebenso einige kurze Anzeigetexte.
- **Build-Prüfung:** `tools/check_ram.py` lässt den Build scheitern, wenn der feste RAM-Verbrauch über 51.600 Byte steigt.

## v0.5.3-dev

- **Webinterface-Menü:** Die wischbare Reiterleiste ist ersetzt durch ein Menü hinter der Taste ☰ (ausklappend wie beim RavestickBoard). Unter dem Titel steht die aktuelle Seite.
- **Auge-Symbol** an WLAN-Passwort und API-Keys: zeigt die gerade getippte Eingabe. Gespeicherte Werte werden weiterhin nie an den Browser zurückgeschickt.
- **3-Tage-Vorhersage als eigene Ansicht** in der Rotation, direkt unter „Wetter“, mit eigener Dauer und ▶. Der feste 8-s-Wechsel in der Wetteransicht entfällt; der Wetter-Reiter weist auf die neue Ansicht hin. Wer die Vorhersage bisher eingeschaltet hatte, findet die neue Zeile nach dem Update angehakt. Die Rotation folgt jetzt der Reihenfolge der Liste (Flug-Tracker vor Uhr).
- **Flugzeugtypen:** eine einzige Tabelle im Flash mit 232 Typen weltweit (Airliner aller Varianten, Regionaljets, Frachter, Business-Jets, allgemeine Luftfahrt, Hubschrauber, Militär, Russland/GUS). Mit Hersteller, solange es in 18 Zeichen passt; ein Code ergibt genau einen Text; unbekannte Codes erscheinen als Code.
- **Radar-Optionen** im Flugradar-Reiter: Betreiber als Name zum Rufzeichen (Standard), Oberbegriff oder beides (für bekannte Militär-Rufzeichen, z. B. „US Air Force / AMC“); Geschwindigkeit in km/h oder Knoten im Wechsel mit dem Kennzeichen; ▲▼ für Steig-/Sinkflug ab 300 ft/min; „MIL“ neben dem Rufzeichen für Militär.
- **4-stellige Flughafencodes:** Routen zu Flugplätzen ohne IATA-Code wurden bisher komplett verworfen („N/A“), weil adsb.lol dort den ICAO-Code liefert. Jetzt gelten 3 Buchstaben (IATA) oder 4 Zeichen (ICAO, Buchstabe am Anfang, Ziffern erlaubt); adsbdb und aviationstack nutzen den ICAO-Code als Ersatz. Das „>“ bleibt fest in der Mitte: 3-stellige Routen sehen pixelgenau aus wie bisher, 4-stellige wachsen nach außen.
- **Notfälle:** Bei Squawk 7500/7600/7700 sind Rufzeichen und Routenzeile rot; in der Routenzeile wechseln Route und „SQUAWK 7700“ alle 3 s (ohne Route steht der Squawk dauerhaft).
- **Filter:** Hubschrauber, Kleinflugzeuge, Segler/Ballone/Ultraleicht/Drohnen und Flugzeuge am Boden ausblendbar. Mit Filter fragt PixelStop alle Flugzeuge im Radius ab und liest sie einzeln ein (`src/RadarScan.h`), damit der Speicherbedarf nicht mit der Zahl der Flugzeuge wächst.
- **Diagnose:** Erfolgreiche Radarabrufe stehen als eine Zeile im Log (mit dem kleinsten freien Speicher während des Abrufs), Fehler weiter mit allen Schritten; der Log reicht damit etwa fünfmal so lange. Zusatzabfragen melden „leer“ (keine Daten zum Flugzeug) oder „übersprungen“ mit Grund. Im Status: aktuelles Flugzeug und Zahl der geprüften Flugzeuge.
- **Dimmverfahren (Test):** Unter „Anzeige“ wählbar zwischen Standard (PWM) und Sigma-Delta. Sigma-Delta nutzt einen eigenen Baustein des Chips und soll das gelegentliche Aufblitzen bei geringer Helligkeit beseitigen.
- **README:** aufklappbare „Mehr Beispiele“ mit realistischen Daten unter der Galerie; neuer Abschnitt „Flugradar: Darstellung und Einstellungen“ mit Beispielen aus dem Battlestar-Galactica-Universum (Quelle „DRADIS“), der Notfall als GIF mit sechs Bildern aus dem echten Displaycode; der Renderer kann den Quellnamen in der Fußzeile dafür setzen.
- Workflow-Bausteine auf Node-24-Versionen (checkout v5, setup-node v5, setup-python v6, upload-artifact v6).

## v0.5.2-dev

Auf Hardware bestätigt: Weiterrechnen von Entfernung und Höhe, Statuspunkt, Meldung bei „Jetzt anzeigen“, adsb.lol-Schlüssel.


- **Radar weitergerechnet:** Entfernung und Höhe werden zwischen zwei Abrufen jede Sekunde aus Geschwindigkeit über Grund, Kurs und Steig-/Sinkrate fortgeschrieben (`src/RadarMotion.h`). Jeder echte Abruf setzt die Werte neu. Die Höhe folgt der Steigrate höchstens 45 s, die Position höchstens bis kurz nach dem nächsten erwarteten Abruf (mindestens 1, höchstens 4 Minuten); danach bleiben die Werte stehen. Am Boden wird nicht gerechnet. Die auswählbaren Abrufintervalle bleiben unverändert.
- **Statuspunkt in der Radar-Fußzeile:** kurz weiß bei neuen Daten, grün im normalen Takt, gelb bei überfälligem oder fehlgeschlagenem Abruf und bei Anbieterpausen, grau, wenn die Werte stehen. Text und Sekundenzähler rücken dafür 10 Pixel nach rechts.
- **„Jetzt anzeigen“:** Die Meldung nennt jetzt die Anzeigedauer, z. B. „Wird für 30 s angezeigt · danach geht die Rotation weiter“; ist nur eine Ansicht aktiv, bleibt sie bis zum nächsten Speichern stehen.
- **adsb.lol-Schlüssel:** Unter dem Eingabefeld stehen Quellenangabe (ODbL) und Links zum kostenlosen anonymen Schlüssel und zu den weiteren Wegen. Lehnt adsb.lol ab, erscheint ein Hinweis: mit Schlüssel „vermutlich abgelaufen“, ohne Schlüssel „mit kostenlosem Key geht es weiter“. README mit Abschnitt zu den Schlüsseln.

## v0.5.1-dev

Auf Hardware bestätigt: 3-Tage-Vorhersage, „Jetzt anzeigen“, Hinweis auf ungespeicherte Änderungen, Fahrtziele am Dresdner Hauptbahnhof („Flughafen“, „Leipzig, Hbf“).


- **Ortsname bei Fahrtzielen:** Der Ort einer Haltestelle kommt jetzt aus der VVO-Suche und wird gespeichert. Bisher galt das erste Wort des Haltestellennamens als Ort; bei „S-Bahnhof Reick“ wurde so „S-Bahnhof“ als Ort angenommen, „Dresden Flughafen“ blieb ungekürzt und der Titel lautete nur „Reick“. Ohne gespeicherten Ort gilt weiter Dresden. Wer die Haltestelle vor v0.5.1 gewählt hat, sucht sie einmal neu aus, damit auch Haltestellen außerhalb Dresdens den richtigen Ort bekommen.
- **3-Tage-Vorhersage:** Die Wetteransicht wechselt alle 8 Sekunden zwischen heute und den nächsten drei Tagen (Wochentag, Symbol, Höchst-/Tiefstwert, Regen). Abschaltbar im Wetter-Reiter.
- **Webinterface aufgeräumt:** Reiter in einer wischbaren Zeile, Rotation einzeilig mit Knopf ▶ „Jetzt anzeigen“, roter Punkt am Speichern-Knopf bei ungespeicherten Änderungen, Koordinaten und lange Hinweise eingeklappt, inaktive Countdowns eingeklappt, Links im Akzentgelb.
- **Arbeitsspeicher:** etwa 3 KB fester Texte und Tabellen (Webserver-Antworten, Airline- und Flugzeugtypnamen, deutsche Wetterbezeichnungen) liegen jetzt im Flash.

## v0.5.0-dev

- **Neuer Name: PixelStop.** Repository, Firmware-Name, Setup-WLAN (`PixelStop-Setup`), Standard-Hostname (`pixelstop`), User-Agent und Dateinamen. Gespeicherte Hostnamen bleiben erhalten.
- **Deutsch/Englisch:** umschaltbar unter System → Sprache / Language, für Display und Webinterface. Display-Texte liegen beidsprachig im Flash; die englischen Webtexte werden nur bei Bedarf geladen.
- **Fahrtziele:** optional abkürzen (Hbf, Bf, Str., Pl., St.) und Ortsnamen der Haltestelle weglassen, beides standardmäßig an. Gekürzte Ziele enden nicht mehr mit Leerzeichen vor dem Punkt.
- Das bisher fest eingebaute Entfernen von „Dresden “ aus allen Anzeigetexten entfällt; Wetter-, Radar- und Countdown-Titel bleiben unverändert.
- `src/Version.h` als einzige Stelle für Name und Version.
- `tools/render_screens.py` erzeugt die Screenshots der README aus dem echten Displaycode.
- Neue README auf Deutsch und Englisch.


## v0.4.14.8

- Radar- und Routenabfragen nur, solange das Radar sichtbar ist, plus Vorabruf 10 s vor dem Wechsel.
- Tagesbudget (450 Anfragen/UTC-Tag) ohne Schlüssel; Intervall wird anhand des sichtbaren Radaranteils gestreckt, Routen bei knappem Budget übersprungen.
- Anfragezähler und Budget-Intervall im Systemstatus.

## v0.4.14.7

- adsb.lol-Abfragen über HTTP/1.1 statt HTTP/1.0 (Server antwortete mit 426); Chunked-Antworten werden beim Lesen entpackt.
- 401/403/426 lösen eine lange Pause (10/20/40 min) statt schneller Wiederholungen aus; eigene Displaymeldung.
- Optionaler adsb.lol-Schlüssel (`x-api-key`); Radar-Intervalle 1 und 3 min.

## v0.4.14.6

- Hintergrundbeleuchtung: PWM 300 Hz statt 1 kHz und 1023 statt 255 Stufen; längere Einschaltpulse bei 1 % gegen Aufblitzen.
- Quellcode direkt im Repository; Releases über Versions-Tags.

## v0.4.14.5.1

- Exakte VVO-Antwort ServiceError / no serving lines found neutral darstellen; übrige Fehler unverändert.
- Phasensynchrone PWM-Ansteuerung des vorhandenen Frameworks zum Hardwaretest gegen Aufblitzen.
- Gemeinsamer Build-Workflow mit gezielten Auslösern und Abbruch überholter Builds.
- README-Versionsgeschichte ergänzt; README einzeln liefern.

## v0.4.14.5

- Unveränderte Helligkeitswerte setzen die PWM nicht erneut.
- Airline-Cache speichert nur erfolgreich geladene Namen; Cache vor Speicher- und Pausenprüfung.
- Doppelten Kartenhinweis entfernt, Offlinefeedback ergänzt.
- Ortsdienst eingeklappt unter erweiterten Einstellungen; Dienste und Bedingungen unter Wartung.
- Dokumentation auf Browser-Ortssuche, Anbieterpausen, Counter und Fehleranzeigen aktualisiert.

## v0.4.14.4.2

- Einheitlicher optionaler Abrufcounter für Live, letzten Stand und Anbieterpause, standardmäßig aktiv.

## v0.4.14.4

- Letztes Flugzeug bleibt während bestätigter Anbieterpause mit Altersanzeige erhalten.

## v0.4.14.3

- Ortssuche und Kartenauflösung in den Browser verlagert.

## v0.4.13

- Geo prüft den freien Heap und den größten Speicherblock erneut unmittelbar vor dem HTTP-Abruf, nach Vorbereitung von LittleFS-Datei, URL, HTTP-Client und Headern. Eine nicht ausreichende Reserve führt zum kontrollierten Abbruch vor dem Verbindungsaufbau.
- Lokale URL-Kopien und die interne Suchtextkopie werden nach Übernahme durch HTTPClient freigegeben. Die Suche fordert keine ungenutzten Addressdetails mehr an; acht Treffer und deutsche Namensvarianten bleiben erhalten. Die Kartenauflösung fordert ihre benötigten Adressdaten weiterhin an.
- Feste Diagnosebezeichnungen liegen im Flash. Der statische RAM-Bedarf sinkt gegenüber v0.4.12 um 484 Bytes. Das ist ein begrenzter Gewinn, keine Garantie für erfolgreiche Ortsabfragen in jedem Betriebszustand.
- Neue Geo-Marker: Geo:Vorbereitung, Geo:Verbinden, Geo:Empfang und Geo:TLS-frei. Normale Fortschrittsmarker werden nicht als letzter Fehler gespeichert; sie bleiben als RTC-Marker auswertbar.
- Der Empfangsschutz aus v0.4.12 bleibt bestehen: mindestens 3 KiB Rest-Heap, höchstens 48 KiB Antwort, begrenzte Schreibschritte und JSON erst nach TLS-Freigabe.

Hardwaretest offen. Der v0.4.12-Absturz ist durch die passende ELF als ungültiger Schreibzugriff innerhalb der ROM-Speicherkopierfunktion eingegrenzt, aber nicht vollständig erklärt. Wetter-/Radar-Abrufe, Anbieterpausen, Typanzeige und Programme bleiben erhalten.

## v0.4.12

- Geo-Empfang gezielt repariert: Der LittleFS-Zwischenpuffer meldet nun seine Schreibkapazität. Ohne diese Angabe übertrug die ESP8266-Routine in v0.4.11 keine Bytes und wartete bis zum Timeout.
- Empfang in höchstens 256-Byte-Schritten. Unter 3 KiB freiem Heap, bei Schreibfehlern oder Antworten über 48 KiB wird kontrolliert abgebrochen. Der Dateicache wird vor der TLS-Verbindung vorbereitet; JSON wird weiterhin erst nach TLS-Freigabe gelesen.
- Diagnose unterscheidet Empfangsfehler, Dateifehler und zu große Antworten von JSON-Parserfehlern.
- Regressionstest verwendet den tatsächlichen Übertragungscode des installierten ESP8266-Frameworks statt eines vereinfachten Transfers.
- Bekannte Airbus-, Boeing- und Embraer-E-Jet-Typen erhalten stabile lesbare Displaynamen, auch bevor Metadaten eintreffen. E75L und E75S werden als Embraer E175 angezeigt.
- Abfahrten-Hinweis nennt jetzt den Verkehrsverbund Oberelbe (VVO) ausgeschrieben.

Gezielte Weiterentwicklung von v0.4.11. Radar-Abrufe, Anbieterpausen, Routenprüfung, Wetterabrufe und Konfiguration bleiben erhalten. Kein vollständiger Rückbau auf v0.4.9. Hardwaretest dieser Version offen.

## v0.4.11

- Ortssuche und Karten-Ortsnamen: TLS zuerst beenden, anschließend JSON lesen. Die Antwort wird vorübergehend in einer größenbegrenzten LittleFS-Datei gespeichert und danach gelöscht. Das vermeidet gleichzeitigen TLS-/JSON-Speicherbedarf, verursacht aber zusätzliche Flash-Schreibvorgänge bei Ortsabfragen. Keine Schreibvorgänge bei normalen Wetter- oder Radar-Aktualisierungen.
- Flugzeugtyp nur übernehmen, wenn Live-Typ oder unterstützte Live-Kategorie dazu passen. Bei unbekannter Kategorie bleiben zusätzliche Typangaben leer. Testfall N54561: gemeldete Kategorie A3 darf keinen ST75-Doppeldecker übernehmen.
- Streckendaten bleiben eine externe Datenbankzuordnung, kein belegter aktueller Flugplan. Falsche Herkunft mit gleichem Ziel kann die Positionsprüfung weiterhin bestehen.
- Leere HTTP-201-Antwort bei Radar.Route im Log als leer kenntlich machen.
- Log kopieren mit Auswahl-Fallback auf lokalem HTTP; gleiche Schrift für Schaltflächen. Letzten Fehler dieses Starts separat behalten, auch wenn die 24 Ereignisse überschrieben sind.
- Programmauswahl: Abfahrten, Wetter, Flugradar, Flug-Tracker, Uhr, Countdowns. Bestehende Programm-IDs bleiben erhalten.
- Standortname: Kein Wechsel (Wert 0), Wunschtext bleibt stehen.
- Workflow: Firmware und Diagnose-/Lizenz-/Quellenpaket als getrennte Downloads.

Hardwaretest für v0.4.11 offen. v0.4.10 lief im Nutzer-Belastungstest über 83 Minuten ohne Neustart, inklusive Programmwechseln und selbstständiger Erholung nach HTTP 429. Ortssuche in v0.4.10 blieb durch JSON:NoMemory defekt.

# Versionsgeschichte

### v0.4.10

- **Speicher bei Netzwerkabfragen:** JSON-Speicher wächst in kleineren Blöcken. Ein eigener Allocator hält 3 KiB freien Heap zurück; bei fehlendem Speicher wird die Antwort verworfen. Nicht mehr benötigte Anfrage-, Filter- und TLS-Daten werden vor der weiteren Auswertung freigegeben. Auch ein unvollständig aufgebauter JSON-Filter wird erkannt. Das soll die in v0.4.9 beobachteten Abstürze bei Karten- und VVO-Abfragen vermeiden; der Hardwaretest steht aus.
- **TLS-Puffer:** Wetter, VVO, Ortsabfragen und Radar-Metadaten prüfen einmal pro Start, ob der jeweilige Server kleinere TLS-Empfangsblöcke unterstützt. Nur bei bestätigter Unterstützung wird ein 512-Byte-Puffer verwendet. Andernfalls bleibt der bisherige größere Puffer.
- **Radar-Drosselung:** Ein erfolgreicher Abruf setzt nach HTTP 429 nicht mehr sofort alle Drosselungshinweise zurück. Positionsabrufe laufen vorübergehend mit mindestens 15 beziehungsweise 20 Sekunden Abstand; zusätzliche adsb.lol-Routenabfragen pausieren. Retry-After und Anbieterpausen bleiben maßgeblich. Kürzere HTTP-Antworttimeouts verkürzen einige Wartephasen, sind aber keine garantierte Obergrenze für die gesamte Verbindung.
- **Routen:** HTTP 201 wird beim adsb.lol-Routenendpunkt als mögliche erfolgreiche Antwort verarbeitet. Callsign, Antwortstruktur und Plausibilität der Flughäfen werden weiterhin geprüft.
- **Diagnose:** Der Log unterscheidet JSON-Speichermangel, unvollständige, leere, zu tief verschachtelte und ungültige Antworten. Der Systemstatus zeigt das vorübergehende Radar-Mindestintervall. Der Build enthält die passende ELF-Datei zur späteren Zuordnung von Absturzadressen.
- **Helligkeit:** Normal- und Nachtmodus erlauben mindestens 1 %. Zuvor gespeicherte 0 % werden beim Laden auf 1 % angehoben.
- **Lizenz:** Eigener Projektcode und Dokumentation stehen unter MIT. Copyright- und Lizenzhinweis bleiben bei Weitergabe verpflichtend. Herkunft aus smalltv-mod, Bibliothekslizenzen und externe Datenrechte werden getrennt aufgeführt. Dem Firmware-Build liegen Quellen, Lizenzhinweise und Buildobjekte bei.

### v0.4.9

Diagnose-Log, RTC-Marker, getrennte Radar-Abfragen, Fehlererholung für Wetter/VVO und Speichersperre während Ortsabfragen. Hardwaretests zeigten weiterhin HTTP-429-Pausen sowie Abstürze bei speicherintensiven JSON-Abfragen.

Ältere Versionen sind in der README zusammengefasst.

# Übersicht der Versionen bis v0.4.14.8

Aus der früheren README übernommen.

| Version | Änderungen |
| --- | --- |
| **v0.4.14.8**   | Radar-Abfragen nur bei sichtbarem Radar plus Vorabruf; Tagesbudget mit automatischer Intervallstreckung ohne Schlüssel |
| **v0.4.14.7**   | adsb.lol über HTTP/1.1 mit Chunk-Entpacker; lange Pause bei 401/403/426 statt Reboot-Schleife; optionaler adsb.lol-Schlüssel; Intervalle 1 und 3 min |
| **v0.4.14.6**   | Hintergrundbeleuchtung mit 300 Hz und 1023 Stufen gegen Aufblitzen bei 1 %; Quellcode direkt im Repository, Releases über Tags |
| **v0.4.14.5.1** | Neutrale VVO-Meldung für nicht bediente Haltestellen; Test einer phasensynchronen Hintergrundbeleuchtung; gemeinsamer Build-Workflow; Versionsgeschichte ergänzt |
| **v0.4.14.5** | Unveränderte PWM-Werte nicht erneut setzen; fehlerhafte leere Airline-Cacheeinträge vermeiden; Offlinehinweise; Ortsdienst einklappen und Anbieterinformationen unter System |
| **v0.4.14.4.2** | Einheitlicher optionaler Abrufcounter für LIVE, LETZTER STAND und ABRUFPAUSE; standardmäßig aktiviert |
| **v0.4.14.4.1** | Zeitangabe bei Anbieterpause links zum Statustext verschoben |
| **v0.4.14.4** | Letztes Flugzeug während bestätigter Anbieterpause erhalten; Alter seit letztem erfolgreichen Positionsabruf anzeigen |
| **v0.4.14.3** | Freitextsuche und Karten-Ortsauflösung in den Browser verlagert; Warteschlange, Cache und austauschbarer Ortsdienst |
| **v0.4.14.2** | Geo-Empfang und Speicherbudget getrennt geprüft; TLS-Objekte vor JSON-Auswertung vollständig freigegeben; Ortsabfragen auf Hardware noch nicht zuverlässig |
| **v0.4.14.1** | TLS-Speicherprüfung an tatsächliche Stackgröße und kleinere zusammenhängende Blockanforderung angepasst |
| **v0.4.14** | Weitere gezielte Korrekturen am Geo-Empfang auf Basis von v0.4.13; Karten- und Freitextsuche blieben in den Hardwaretests problematisch |
| **v0.4.13** | Zusätzliche Speicherprüfung vor Geo-Verbindung; Diagnosemarker erweitert; feste Diagnosebezeichnungen in Flash verlagert |
| **v0.4.12** | Schreibkapazität und Fehler des Geo-Zwischenpuffers korrekt melden; Empfang begrenzen; stabile lesbare Flugzeugtypen einschließlich Embraer; VVO-Hinweis ausgeschrieben |
| **v0.4.11** | Geo-Antwort zwischenspeichern und erst nach TLS-Freigabe auswerten; Flugzeugtyp plausibilisieren; letzter Fehler separat behalten; Programmauswahl neu geordnet; Standortname ohne Wechsel möglich |

Die Geo-Korrekturen v0.4.11 bis v0.4.14.2 beseitigten nicht alle auf Hardware beobachteten Probleme. Erst die Verlagerung der Ortssuche in den Browser ab v0.4.14.3 wurde im Gespräch als stabil bestätigt. Die Tabelle beschreibt Änderungen, keine zugesicherte Fehlerfreiheit aller historischen Builds.


## v0.4.10

- **Speicher bei Netzwerkabfragen:** JSON-Speicher wächst in kleineren Blöcken. Ein eigener Allocator hält 3 KiB freien Heap zurück; bei fehlendem Speicher wird die Antwort verworfen. Nicht mehr benötigte Anfrage-, Filter- und TLS-Daten werden vor der weiteren Auswertung freigegeben. Auch ein unvollständig aufgebauter JSON-Filter wird erkannt. Das soll die in v0.4.9 beobachteten Abstürze bei Karten- und VVO-Abfragen vermeiden; der Hardwaretest steht aus.
- **TLS-Puffer:** Wetter, VVO, Ortsabfragen und Radar-Metadaten prüfen einmal pro Start, ob der jeweilige Server kleinere TLS-Empfangsblöcke unterstützt. Nur bei bestätigter Unterstützung wird ein 512-Byte-Puffer verwendet. Andernfalls bleibt der bisherige größere Puffer.
- **Radar-Drosselung:** Ein erfolgreicher Abruf setzt nach HTTP 429 nicht mehr sofort alle Drosselungshinweise zurück. Positionsabrufe laufen vorübergehend mit mindestens 15 beziehungsweise 20 Sekunden Abstand; zusätzliche adsb.lol-Routenabfragen pausieren. Retry-After und Anbieterpausen bleiben maßgeblich. Kürzere HTTP-Antworttimeouts verkürzen einige Wartephasen, sind aber keine garantierte Obergrenze für die gesamte Verbindung.
- **Routen:** HTTP 201 wird beim adsb.lol-Routenendpunkt als mögliche erfolgreiche Antwort verarbeitet. Callsign, Antwortstruktur und Plausibilität der Flughäfen werden weiterhin geprüft.
- **Diagnose:** Der Log unterscheidet JSON-Speichermangel, unvollständige, leere, zu tief verschachtelte und ungültige Antworten. Der Systemstatus zeigt das vorübergehende Radar-Mindestintervall. Der Build enthält die passende ELF-Datei zur späteren Zuordnung von Absturzadressen.
- **Helligkeit:** Normal- und Nachtmodus erlauben mindestens 1 %. Zuvor gespeicherte 0 % werden beim Laden auf 1 % angehoben.
- **Lizenz:** Eigener Projektcode und Dokumentation stehen unter MIT. Copyright- und Lizenzhinweis bleiben bei Weitergabe verpflichtend. Herkunft aus smalltv-mod, Bibliothekslizenzen und externe Datenrechte werden getrennt aufgeführt. Dem Firmware-Build liegen Quellen, Lizenzhinweise und Buildobjekte bei.

## Ältere Versionen

| Version | Wichtigste Neuerungen |
| --- | --- |
| **v0.4.9** | Diagnose-Log mit Neustart-Hinweisen, getrennte Radar-Zusatzabfragen, gestufte Wetter-/VVO-Wiederholungen und Speichersperre während der Ortsnamensabfrage |
| **v0.4.8** | Anbieterpausen, positionsgeprüfte Radar-Routen, Online-Ergänzung von Airline und Typ, Bodenstatus GND, Kartenkorrekturen und Teilaktualisierungen in weiteren Programmen |
| **v0.4.7** | Einstellbare Zeitzonen, griechische und kyrillische Zeichen, differenziertere Wetterbilder sowie Verbesserungen bei Radar-Abrufen und der Anzeige veralteter Daten |
| **v0.4.6** | Erweiterter lateinischer Zeichensatz, mehrteilige Flugrouten, Tracker-Footer und Verbesserungen am Webinterface |
| **v0.4.5** | Kompakte Abfahrtsziele, Verbesserungen an Karten- und Standortauswahl, Speichern der Einstellungen und Radar-Verbindungen |
| **v0.4.4** | Bis zu zehn Abfahrten mit Seitenwechsel, Temperatureinheiten, Uhr im Footer und robustere Radarfehlerbehandlung |
| **v0.4.3** | Überarbeitung der öffentlichen Projektangaben |
| **v0.4.2** | Karten-Picker, Rückmeldungen im Webinterface und ruhigere Uhr- und Countdown-Anzeige |
| **v0.4.1** | Programmrotation, WLAN-Scan, Ortssuche, deutsche Sonderzeichen und Verkehrsmittelfilter |
| **v0.4** | Flug-Tracker und vier speicherbare Countdowns |
| **v0.3** | Live-Wetter, Flugradar und weitere Hauptansichten |
| **v0.2** | VVO-Haltestellensuche und Abfahrtsmonitor |
| **v0.1** | WLAN-Einrichtung, Webinterface, automatische Zeitabfrage und Firmware-Updates |
