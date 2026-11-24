#!/usr/bin/env python3
"""Erzeugt die Bildbereiche im README (Übersicht + Programme) als Karten in Absätzen.
Aufruf im Repo-Wurzelverzeichnis: python3 tools/readme_galerie.py .   (Bilder vorher mit tools/render_screens.py erzeugen)"""
import sys
from pathlib import Path

ROOT = Path(sys.argv[1])
W = 260   # card width in the README: screen at exactly half of the 2x picture


V = '0.5.4.7'   # cache buster: GitHub and browsers fetch new pictures after a change


def row(base, names, alts, label=None):
    imgs = '\n'.join(f'<img src="{base}{n}?v={V}" width="{W}" alt="{alts[n]}">' for n in names)
    head = f'<sub><b>{label}</b></sub><br>\n' if label else ''
    return f'<p align="center">\n{head}{imgs}\n</p>\n'


def details(title, text, rows, after=''):
    return f'<details><summary><b>{title}</b></summary>\n\n{text}\n\n' + '\n'.join(rows) + (f'\n{after}\n' if after else '\n') + '</details>\n'


L = {}
L['de'] = dict(base='docs/screens/de/', alts={
    'kachel-abfahrten.png': 'Abfahrten', 'kachel-wetter.png': 'Wetter', 'kachel-vorhersage.png': '3-Tage-Vorhersage',
    'kachel-flugradar.png': 'Flugradar', 'kachel-flugtracker.png': 'Flug-Tracker', 'kachel-uhr.png': 'Uhr', 'kachel-countdown-tage.png': 'Countdown',
    'abfahrten.png': 'Abfahrten: Dresden Hbf mit Verspätung und Ausfall', 'abfahrten-ziele.png': 'Kurze Fahrtziele', 'abfahrten-seiten.gif': 'Zehn Abfahrten auf zwei Seiten', 'abfahrten-nicht-bedient.png': 'Haltestelle wird nicht bedient', 'abfahrten-fehler.png': 'VVO nicht erreichbar', 'radar-fehler.png': 'Abruf fehlgeschlagen', 'radar-abgelehnt.png': 'adsb.lol: Zugriff abgelehnt', 'flugtracker-ohne-key.png': 'Flug-Tracker ohne API-Key',
    'wetter.png': 'Wetter: Dresden am Tag', 'wetter-nacht.png': 'Wetter: Dresden nachts', 'vorhersage.png': '3-Tage-Vorhersage für Dresden',
    'flugradar.gif': 'Flugradar: Emirates A380 über Dresden', 'radar-steigflug.gif': 'Steigflug: ALROSA Tu-154M ab Mirny', 'radar-sinkflug.gif': 'Sinkflug: European Air Charter MD-82 nach Varna',
    'radar-name.gif': 'Langer Airline-Name im Wechsel', 'radar-boden.gif': 'Flugzeug am Boden', 'radar-leer.png': 'Kein Flugzeug im Radius',
    'radar-gelb.gif': 'Letzter Stand nach fehlgeschlagenem Abruf', 'radar-suche.png': 'Suche nach einer Pause', 'radar-suche-neu.png': 'Suche: neuer Versuch', 'countdown-ohne-nachleuchten.png': 'Countdown ohne Nachleuchten', 'radar-pause.gif': 'Abrufpause', 'radar-grau.gif': 'Keine Daten mehr',
    'radar-colone.gif': '4-stellige Codes (fiktiv)', 'radar-apollo.gif': 'Militär (fiktiv)', 'radar-karma.gif': 'Notfall (fiktiv)',
    'flugtracker.png': 'Flug-Tracker LH1178', 'uhr.png': 'Radiowecker mit Datum', 'abfahrten-unkonfiguriert.png': 'Noch keine Haltestelle gewählt', 'wetter-symbole.gif': 'Alle Wettersymbole nacheinander', 'wetter-fehler.png': 'Open-Meteo nicht erreichbar', 'radar-sq26.gif': 'Singapore Airlines SIN – FRA – JFK', 'radar-islandhopper.gif': 'United Island Hopper mit 7 Flughäfen', 'radar-red2.gif': 'Funkausfall, Squawk 7600 (fiktiv)', 'radar-galactica.gif': 'Entführung, Squawk 7500 (fiktiv)', 'flugtracker-ohne-flug.png': 'Flug-Tracker ohne Flugnummer', 'flugtracker-status.gif': 'Status geplant, in der Luft, gelandet', 'flugtracker-annulliert.png': 'Flug annulliert', 'flugtracker-umgeleitet.png': 'Flug umgeleitet', 'flugtracker-nicht-gefunden.png': 'Flug nicht gefunden', 'flugtracker-fehler.png': 'Tracker-Fehler', 'uhr-blinken.gif': 'Radiowecker mit blinkendem Doppelpunkt', 'uhr-12h.png': 'Radiowecker im 12-Stunden-Format', 'uhr-modern.gif': 'Uhr im Stil Modern', 'uhr-pixel.png': 'Uhr im Stil Pixel', 'setup.png': 'Setup-Hotspot', 'start.gif': 'Start und WLAN-Verbindung', 'rettung.png': 'Rettungsmodus',
    'countdown-tage.png': 'Countdown in Tagen', 'countdown-timer.gif': 'Countdown-Timer', 'countdown-plus.gif': 'Countdown nach dem Ziel',
    'countdown-tage-modern.png': 'Modern in Tagen', 'countdown-modern.gif': 'Modern als Timer', 'countdown-modern-null.png': 'Modern am Ziel ohne Vorzeichen'})
L['en'] = dict(base='docs/screens/en/', alts={
    'kachel-abfahrten.png': 'Departures', 'kachel-wetter.png': 'Weather', 'kachel-vorhersage.png': '3-day forecast',
    'kachel-flugradar.png': 'Flight radar', 'kachel-flugtracker.png': 'Flight tracker', 'kachel-uhr.png': 'Clock', 'kachel-countdown-tage.png': 'Countdown',
    'abfahrten.png': 'Departures: Dresden Hbf with a delay and a cancellation', 'abfahrten-ziele.png': 'Short destinations', 'abfahrten-seiten.gif': 'Ten departures on two pages', 'abfahrten-nicht-bedient.png': 'Stop not served', 'abfahrten-fehler.png': 'VVO not reachable', 'radar-fehler.png': 'Request failed', 'radar-abgelehnt.png': 'adsb.lol: access denied', 'flugtracker-ohne-key.png': 'Flight tracker without API key',
    'wetter.png': 'Weather: Berlin by day', 'wetter-nacht.png': 'Weather: Berlin at night', 'vorhersage.png': '3-day forecast for Berlin',
    'flugradar.gif': 'Flight radar: Emirates A380 over Dresden', 'radar-steigflug.gif': 'Climbing: ALROSA Tu-154M out of Mirny', 'radar-sinkflug.gif': 'Descending: European Air Charter MD-82 to Varna',
    'radar-name.gif': 'Long airline name alternating', 'radar-boden.gif': 'Aircraft on the ground', 'radar-leer.png': 'No aircraft within the radius',
    'radar-gelb.gif': 'Last data after a failed request', 'radar-suche.png': 'Searching after a pause', 'radar-suche-neu.png': 'Searching: retrying', 'countdown-ohne-nachleuchten.png': 'Countdown without afterglow', 'radar-pause.gif': 'Paused', 'radar-grau.gif': 'No data any more',
    'radar-colone.gif': '4-character codes (fictional)', 'radar-apollo.gif': 'Military (fictional)', 'radar-karma.gif': 'Emergency (fictional)',
    'flugtracker.png': 'Flight tracker LH1178', 'uhr.png': 'Clock radio with date', 'abfahrten-unkonfiguriert.png': 'No stop chosen yet', 'wetter-symbole.gif': 'All weather icons in turn', 'wetter-fehler.png': 'Open-Meteo not reachable', 'radar-sq26.gif': 'Singapore Airlines SIN – FRA – JFK', 'radar-islandhopper.gif': 'United Island Hopper with 7 airports', 'radar-red2.gif': 'Radio failure, squawk 7600 (fictional)', 'radar-galactica.gif': 'Hijack, squawk 7500 (fictional)', 'flugtracker-ohne-flug.png': 'Flight tracker without flight number', 'flugtracker-status.gif': 'Status scheduled, in the air, landed', 'flugtracker-annulliert.png': 'Flight cancelled', 'flugtracker-umgeleitet.png': 'Flight diverted', 'flugtracker-nicht-gefunden.png': 'Flight not found', 'flugtracker-fehler.png': 'Tracker error', 'uhr-blinken.gif': 'Clock radio with blinking colon', 'uhr-12h.png': 'Clock radio in 12-hour format', 'uhr-modern.gif': 'Clock in the Modern style', 'uhr-pixel.png': 'Clock in the Pixel style', 'setup.png': 'Setup hotspot', 'start.gif': 'Start and Wi-Fi connection', 'rettung.png': 'Rescue mode',
    'countdown-tage.png': 'Countdown in days', 'countdown-timer.gif': 'Countdown timer', 'countdown-plus.gif': 'Countdown after the target',
    'countdown-tage-modern.png': 'Modern in days', 'countdown-modern.gif': 'Modern timer', 'countdown-modern-null.png': 'Modern at the target without a sign'})

OV = ['kachel-abfahrten.png', 'kachel-wetter.png', 'kachel-vorhersage.png', 'kachel-flugradar.png', 'kachel-flugtracker.png', 'kachel-uhr.png', 'kachel-countdown-tage.png']

T = {
 'de': dict(
  note='<p align="center"><sub>Echte Ausgabe der Firmware mit Beispieldaten · mehr Bilder und Erklärungen unter <a href="#programme">Programme</a></sub></p>',
  head='## Programme\n\nZum Aufklappen: was jedes Programm zeigt, was sich einstellen lässt und alle Beispielbilder. Die Bilder sind echte Ausgaben der Firmware mit **Beispieldaten**; bewegte Bilder laufen in Echtzeit, eine Sekunde pro Bild.\n',
  dep=('🚏 Abfahrten', 'Live-Abfahrten einer Haltestelle des Verkehrsverbunds Oberelbe (VVO): Linie, Ziel, Minuten bis zur Abfahrt, Verspätung (gelb) und Ausfall (✕). Drei bis fünf Abfahrten auf einer Seite oder zehn auf zwei Seiten im Wechsel alle 5 Sekunden, Filter nach Verkehrsmittel. Fahrtziele lassen sich ohne den eigenen Ort und abgekürzt anzeigen. Die Minuten zählen zwischen den Abrufen herunter – auch dann, wenn der VVO gerade nicht erreichbar ist.'),
  wx=('🌦️ Wetter', 'Aktuelle Temperatur, Wetterbild für Tag und Nacht, Höchst- und Tiefstwert, Regenwahrscheinlichkeit sowie Sonnenauf- und -untergang, in °C oder °F. Der Ort wird über eine Suche, die Karte oder Koordinaten gewählt; ein eigener Anzeigename ist möglich.'),
  fc=('📅 3-Tage-Vorhersage', 'Wochentag, Wetterbild, Höchst- und Tiefstwert und Regenwahrscheinlichkeit für die nächsten drei Tage – als eigene Ansicht in der Rotation.'),
  radar=('✈️ Flugradar', 'Das nächste Flugzeug im gewählten Umkreis: Rufzeichen, Airline, Route, Typ, Kennzeichen, Entfernung und Höhe. Zwischen zwei Abrufen rechnet der Würfel Entfernung und Höhe weiter; die Fußzeile zeigt, wie alt der letzte Abruf ist. Kommt das Radar nach einer Pause in der Rotation zurück und ist der letzte Stand älter als 30 Sekunden, zeigt PixelStop kein altes Flugzeug, sondern **SUCHE FLUGZEUGE**, bis der erste frische Abruf da ist; scheitert er, bleibt die Suche stehen („NEUER VERSUCH“), und erst nach drei Fehlschlägen in Folge erscheint „Abruf fehlgeschlagen“.\n\n'
         'Im Reiter **Flugradar** lässt sich die Anzeige anpassen:\n\n'
         '- **Betreiber:** Name zum Rufzeichen (Standard), Oberbegriff oder beides, z. B. „US Air Force / AMC“ – für bekannte Militär-Rufzeichen.\n'
         '- **Geschwindigkeit** in km/h oder Knoten, alle 3 Sekunden im Wechsel mit dem Kennzeichen.\n'
         '- **▲▼** zwischen Entfernung und Höhe bei Steig- oder Sinkflug ab 300 ft/min.\n'
         '- **MIL** neben dem Rufzeichen für Militärflugzeuge.\n'
         '- **Routen mit Zwischenstopps:** drei Flughäfen in kleinerer Schrift, längere Ketten auf zwei Zeilen.\n'
         '- **Hochrechnung im Webinterface:** Unter *Flugradar → Radius und Abruf* zeigt ein Satz, wie lange die Kontingente von adsb.lol beim gewählten Takt reichen, dazu eine aufklappbare Tabelle (Stunde, Tag, Woche, Monat). Die Werte sind eine Schätzung (Routenabfragen ca. +10 %) und kosten keinen Speicher in der Firmware.\n'
         '- **Ausblenden:** Hubschrauber, Kleinflugzeuge, Segler/Ballone/Ultraleicht/Drohnen, Flugzeuge am Boden.\n'
         '- **Immer aktiv:** Notfälle (Squawk 7500/7600/7700) mit rotem Rufzeichen und roter Routenzeile, in der Route und „SQUAWK 7700“ alle 3 s wechseln; zu lange Airline-Namen wechseln ebenfalls alle 3 s.'),
  groups=['Flugbewegungen', 'Anzeige', 'Notfälle · fiktive Daten', 'Datenstand', 'Ohne Flugzeug'], trgroups=['Im Flug', 'Sonderfälle', 'Einrichtung'],
  radar_after=         '| Radar-Fußzeile | Bedeutung |\n| --- | --- |\n'
         '| `● ADSB.LOL LIVE 4s` | letzter erfolgreicher Abruf vor 4 Sekunden |\n'
         '| `LETZTER STAND 16s` | Abruf fehlgeschlagen, vorheriger Stand wird weiter gezeigt |\n'
         '| `ABRUFPAUSE 46s` | adsb.lol bittet um eine Pause; Alter des letzten Abrufs |\n'
         '| `KEIN FLUGZEUG` | Abfrage erfolgreich, aber nichts im Umkreis |\n'
         '| `SUCHE` | nach einer Pause: noch kein frischer Abruf, das alte Flugzeug bleibt verborgen |\n'
         '| `NEUER VERSUCH` | der erste Abruf nach der Pause ist fehlgeschlagen; nächster Versuch folgt |\n'
         '| `ADSB.LOL / Zugriff abgelehnt` | adsb.lol lehnt ab (401/403/426); lange Pause statt ständiger Wiederholung |\n\n'
         '| Punkt in der Fußzeile | Bedeutung |\n| --- | --- |\n'
         '| ⚪ kurz weiß | gerade neue Daten empfangen |\n'
         '| 🟢 grün | Abrufe im normalen Takt; Entfernung und Höhe werden dazwischen weitergerechnet |\n'
         '| 🟡 gelb | Abruf überfällig, fehlgeschlagen oder Anbieterpause; Werte sind geschätzt |\n'
         '| ⚫ grau | zu lange keine Daten; Entfernung und Höhe stehen still |\n',
  tr=('🛫 Flug-Tracker', 'Ein bestimmter Flug mit Plan- und Ist-Zeiten, Status, Flugzeit sowie Live-Höhe und -Geschwindigkeit. Dafür ist ein eigener aviationstack-Schlüssel nötig.'),
  clock=('🕰️ Uhr', 'Uhrzeit und Datum in der gewählten Zeitzone – als **Pixel**, als **Radiowecker** mit Segmentziffern oder als **Modern** mit Rahmen und Tagesbalken. Einstellbar sind Farbe, Sekunden, blinkender Doppelpunkt, Datum und das Nachleuchten der Segmente. Das Format **12 oder 24 Stunden** gilt für alle Ansichten, auch für die Fußzeilen, den Flug-Tracker und die Sonnenzeiten.'),
  system=('⚙️ Einrichtung und System', 'Beim ersten Start öffnet PixelStop den WLAN-Hotspot **PixelStop-Setup**; unter 192.168.4.1 lassen sich WLAN und alles Weitere einstellen. Danach verbindet sich der Würfel und zeigt kurz seine Adresse. Stürzt die Firmware mehrmals kurz hintereinander ab, startet der **Rettungsmodus**: keine Abrufe, aber Webinterface und Firmware-Update bleiben erreichbar.'),
  cd=('⏳ Countdown 1–4', 'Vier unabhängige Countdowns in Tagen, in Stunden : Minuten : Sekunden oder in Minuten : Sekunden, jeweils mit eigenem Titel und eigenen Farben. Die Ziffern sehen aus wie bei einem LED-Radiowecker, nicht leuchtende Segmente glimmen schwach mit.\n\n'
      '- **Nachleuchten:** eine Einstellung für alle vier Countdowns und beide Darstellungen; ausgeschaltet bleiben die nicht leuchtenden Segmente dunkel.\n'
      '- **Darstellung:** „Klassisch“ oder „Modern“ mit Rahmen, „T-MINUS“/„T-PLUS“ und Fortschrittsbalken.\n'
      '- **Vorzeichen:** „−“ vor dem Ziel und „+“ danach – oder ohne Vorzeichen; dann bleibt der Countdown bei null stehen.\n'
      '- **Startdatum** (Tage): bestimmt den Fortschrittsbalken der Darstellung „Modern“; bleibt es leer, wird beim Start der heutige Tag eingetragen.'),
  cdgroups=['Klassisch', 'Modern']),
 'en': dict(
  note='<p align="center"><sub>Real firmware output with sample data · more images and explanations under <a href="#programs">Programs</a></sub></p>',
  head='## Programs\n\nExpand a program to see what it shows, what you can set and all example images. The images are real firmware output with **sample data**; moving images run in real time, one second per frame.\n',
  dep=('🚏 Departures', 'Live departures of a stop of Verkehrsverbund Oberelbe (VVO, Dresden region): line, destination, minutes to departure, delay (yellow) and cancellation (✕). Three to five departures on one page, or ten on two pages switching every 5 seconds; filter by mode of transport. Destinations can be shown without the town of the stop and abbreviated. Minutes count down between requests – even while VVO cannot be reached.'),
  wx=('🌦️ Weather', 'Current temperature, day/night weather icon, high and low, chance of rain, sunrise and sunset, in °C or °F. The place is chosen by search, on the map or by coordinates; a display name of your own is possible.'),
  fc=('📅 3-day forecast', 'Weekday, weather icon, high and low and chance of rain for the next three days – as a view of its own in the rotation.'),
  radar=('✈️ Flight radar', 'The nearest aircraft within the chosen radius: callsign, airline, route, type, registration, distance and altitude. Between two requests the cube keeps calculating distance and altitude; the footer shows the age of the last request. When the radar comes back in the rotation after a pause and the last data are older than 30 seconds, PixelStop shows no old aircraft but **SEARCHING AIRCRAFT** until the first fresh request is in; if it fails, the search stays on screen (“RETRYING”), and “Request failed” appears only after three failed requests in a row.\n\n'
         'The **Flight radar** tab adjusts the view:\n\n'
         '- **Operator:** name from the callsign (default), umbrella term or both, e.g. “US Air Force / AMC” – for known military callsigns.\n'
         '- **Speed** in km/h or knots, alternating with the registration every 3 seconds.\n'
         '- **▲▼** between distance and altitude when climbing or descending at 300 ft/min or more.\n'
         '- **MIL** next to the callsign for military aircraft.\n'
         '- **Routes with stops:** three airports in smaller type, longer chains on two lines.\n'
         '- **Forecast in the web interface:** under *Flight radar → Radius and refresh* one sentence says how long the adsb.lol quotas last at the chosen interval, plus a collapsible table (hour, day, week, month). The values are an estimate (route requests about +10 %) and cost no memory in the firmware.\n'
         '- **Hide:** helicopters, light aircraft, gliders/balloons/ultralights/drones, aircraft on the ground.\n'
         '- **Always on:** emergencies (squawk 7500/7600/7700) with a red callsign and a red route line in which route and “SQUAWK 7700” alternate every 3 s; airline names that are too long alternate every 3 s as well.'),
  groups=['Flight movements', 'Display', 'Emergencies · fictional data', 'Data state', 'No aircraft'], trgroups=['In flight', 'Special cases', 'Setup'],
  radar_after=         '| Radar footer | Meaning |\n| --- | --- |\n'
         '| `● ADSB.LOL LIVE 4s` | last successful request 4 seconds ago |\n'
         '| `LAST DATA 16s` | request failed, the previous data is still shown |\n'
         '| `PAUSED 46s` | adsb.lol asks for a pause; age of the last request |\n'
         '| `NO AIRCRAFT` | request successful, but nothing within the radius |\n'
         '| `SEARCHING` | after a pause: no fresh request yet, the old aircraft stays hidden |\n'
         '| `RETRYING` | the first request after the pause failed; the next attempt follows |\n'
         '| `ADSB.LOL / Access denied` | adsb.lol refuses (401/403/426); long pause instead of constant retries |\n\n'
         '| Dot in the footer | Meaning |\n| --- | --- |\n'
         '| ⚪ briefly white | new data just received |\n'
         '| 🟢 green | requests on schedule; distance and altitude are calculated in between |\n'
         '| 🟡 yellow | request overdue, failed or provider pause; values are estimated |\n'
         '| ⚫ grey | no data for too long; distance and altitude stand still |\n',
  tr=('🛫 Flight tracker', 'One specific flight with scheduled and actual times, status, flight time and live altitude and speed. Requires your own aviationstack key.'),
  clock=('🕰️ Clock', 'Time and date in the chosen time zone – as **Pixel**, as a **clock radio** with segment digits or as **Modern** with frame and day bar. Colour, seconds, blinking colon, date and the afterglow of the segments can be set. The **12 or 24 hour** format applies to all views, including the footers, the flight tracker and the sun times.'),
  system=('⚙️ Setup and system', 'On first start PixelStop opens the Wi-Fi hotspot **PixelStop-Setup**; at 192.168.4.1 you set up Wi-Fi and everything else. Then the cube connects and briefly shows its address. If the firmware crashes several times in a row, **rescue mode** starts: no requests, but the web interface and firmware update stay reachable.'),
  cd=('⏳ Countdown 1–4', 'Four independent countdowns in days, in hours : minutes : seconds or in minutes : seconds, each with its own title and colours. The digits look like an LED clock radio; unlit segments glow faintly.\n\n'
      '- **Afterglow:** one setting for all four countdowns and both styles; switched off, unlit segments stay dark.\n'
      '- **Style:** “Classic” or “Modern” with frame, “T-MINUS”/“T-PLUS” and a progress bar.\n'
      '- **Sign:** “−” before the target and “+” after it – or no sign; then the countdown stops at zero.\n'
      '- **Start date** (days): sets the progress bar of the “Modern” style; if left empty, today is filled in when the countdown is started.'),
  cdgroups=['Classic', 'Modern']),
}

for lang in ('de', 'en'):
    d, t = L[lang], T[lang]
    b, a = d['base'], d['alts']
    p = ROOT / ('README.md' if lang == 'de' else 'README.en.md')
    s = p.read_text()
    G = lambda label, names: row(b, names, a, label)
    prog = '\n'.join([
        t['head'],
        details(t['dep'][0], t['dep'][1], [G(None, ['abfahrten.png', 'abfahrten-ziele.png', 'abfahrten-seiten.gif', 'abfahrten-nicht-bedient.png', 'abfahrten-fehler.png', 'abfahrten-unkonfiguriert.png'])]),
        details(t['wx'][0], t['wx'][1], [G(None, ['wetter.png', 'wetter-nacht.png', 'wetter-symbole.gif', 'wetter-fehler.png'])]),
        details(t['fc'][0], t['fc'][1], [G(None, ['vorhersage.png'])]),
        details(t['radar'][0], t['radar'][1], [
            G(t['groups'][0], ['flugradar.gif', 'radar-steigflug.gif', 'radar-sinkflug.gif']),
            G(t['groups'][1], ['radar-name.gif', 'radar-colone.gif', 'radar-boden.gif', 'radar-sq26.gif', 'radar-islandhopper.gif', 'radar-apollo.gif']),
            G(t['groups'][2], ['radar-galactica.gif', 'radar-red2.gif', 'radar-karma.gif']),
            G(t['groups'][3], ['radar-gelb.gif', 'radar-pause.gif', 'radar-grau.gif']),
            G(t['groups'][4], ['radar-leer.png', 'radar-suche.png', 'radar-suche-neu.png', 'radar-fehler.png', 'radar-abgelehnt.png'])], t['radar_after']),
        details(t['tr'][0], t['tr'][1], [
            G(t['trgroups'][0], ['flugtracker.png', 'flugtracker-status.gif', 'flugtracker-umgeleitet.png']),
            G(t['trgroups'][1], ['flugtracker-annulliert.png', 'flugtracker-nicht-gefunden.png', 'flugtracker-fehler.png']),
            G(t['trgroups'][2], ['flugtracker-ohne-flug.png', 'flugtracker-ohne-key.png'])]),
        details(t['clock'][0], t['clock'][1], [G(None, ['uhr.png', 'uhr-blinken.gif', 'uhr-12h.png', 'uhr-modern.gif', 'uhr-pixel.png'])]),
        details(t['cd'][0], t['cd'][1], [
            G(t['cdgroups'][0], ['countdown-tage.png', 'countdown-ohne-nachleuchten.png', 'countdown-timer.gif', 'countdown-plus.gif']),
            G(t['cdgroups'][1], ['countdown-tage-modern.png', 'countdown-modern.gif', 'countdown-modern-null.png'])]),
        details(t['system'][0], t['system'][1], [G(None, ['setup.png', 'start.gif', 'rettung.png'])]),
    ])
    # overview: from the end of the badges to "What is"
    head = '## Was ist PixelStop?' if lang == 'de' else '## What is PixelStop?'
    i0 = s.index('\n\n', s.index('![Mission]')) + 2
    i1 = s.index(head)
    s = s[:i0] + row(b, OV, a) + t['note'] + '\n\n' + s[i1:]
    # programs: from the programs heading to the status section
    ph = '## Programme' if lang == 'de' else '## Programs'
    nx = '## Entwicklungsstand' if lang == 'de' else '## Status:'
    i0 = s.index(ph); i1 = s.index(nx)
    s = s[:i0] + prog + '\n' + s[i1:]
    p.write_text(s)
    print(p, 'ok')
