# Benchy Racers – Version 1.5.0

Installierbares Windows-Rennspiel mit Unreal Engine 5.8.3. Drei Strecken, Einzelrennen gegen fünf Gegner und private Rennen mit bis zu acht Spielern über eine direkte Netzwerkverbindung. Kein Browser und kein Unreal Editor zum Spielen nötig.

## Installation und Start

BenchyRacers-Unreal-Setup.exe ausführen. Der Installer aktualisiert die separate Unreal-Version; die ältere Godot-Ausgabe bleibt erhalten. Alternativ die Portable-ZIP vollständig entpacken und BenchyRacers.exe starten. Windows 64 Bit und eine DirectX-12-Grafikkarte werden vorausgesetzt. Der Unreal-Laufzeitinstaller liegt unter Engine/Extras/Redist/en-us bei.

Im seitlichen Hauptmenü links Fahrername, Filamentfarbe und Strecke wählen. Rechts steht eine große, langsam drehende 3D-Vorschau des Benchys. Im Hintergrund zeigt eine erhöhte Panoramaansicht die gewählte Insel und den Streckenverlauf. Die Kartenauswahl lädt die tatsächliche Welt; in Online-Lobbys wählt der Host die Karte. Ein Einzelrennen füllt das Feld auf sechs Boote auf. Beim Farbwechsel wird das Vorschauboot in 2,8 Sekunden von unten aufgebaut. Eine bewegliche Nozzle folgt den horizontalen Konturen des tatsächlichen Benchy-Modells. Vor dem Rennen werden die Boote während der ersten Sekunden des Countdowns gedruckt; anschließend blendet die Kamera in die Fahransicht. Nach fünf Sekunden Countdown startet das Rennen über drei Runden. Nach dem persönlichen Zieleinlauf bleiben Platz und Zeit fest stehen. Das Benchy fährt selbstständig entlang der Strecke weiter; das Rennen läuft für die anderen Fahrer weiter. Rechts zeigt ein Ergebnisfenster alle Teilnehmer: fertige Rennzeiten, „läuft noch...“ für aktive Fahrer und „ausgeschieden“ bei einer getrennten Verbindung. Die eigene Zeile ist hervorgehoben. Fertige Boote sammeln keine Items mehr, können nicht getroffen werden und blockieren die anderen Boote nicht. Sobald alle Teilnehmer fertig sind oder die Verbindung verlassen haben, kann der Host ein neues Rennen starten oder die Karte wechseln. Es gibt kein Zeitlimit nach dem ersten Zieleinlauf.

## Drei Welten

- **Sonnendeck:** niedrige tropische Mittelinsel, gewundener Fluss, zwei verbundene Seitenkanäle und flache Nebeninseln.
- **Kupferhafen:** eigener Streckenverlauf zwischen bewachsenen Buchten, Holzstegen und kleinen Bootshäusern im warmen Abendlicht.
- **Frostfjord:** eigenständiger gewundener Wasserweg zwischen verschneiten Ufern, echten gescannten Eiswänden, nordischen Felsen und niedrigen Nebeninseln. Kaltes Wasser, winterliche Vegetation und kühles Fjordlicht; keine tropischen Palmen.

Sonnendeck verwendet 500 gescannte Palmen, 1.000 Bäume und 10.500 Unterwuchspflanzen. Kupferhafen setzt auf bewachsene Buchten, mehr Hafenarchitektur und warmes Licht; Frostfjord auf Schnee, Eis und sparsame winterliche Vegetation. Sieben zusätzliche Felsformen aus Rock Collection 04 sorgen für Abwechslung. Nordic Coastal Cliff ergänzt die Ufer, Ice Cliff und Fresh Windswept Snow prägen Frostfjord. Wooden Planks und Rusty Metal Sheet geben Stegen und Hafengebäuden passende Oberflächen. Die Felsen verwenden deckende Materialien, sitzen im Gelände und halten den Fahrkorridor frei. Die Platzierungsberichte stehen in BuildStatus.json. Schilder ersetzen die sichtbaren Torpaare; eine kleine Streckenübersicht ergänzt die Orientierung auf offenem Wasser. Unsichtbare Kontrollpunkte prüfen die Rennrunde in der richtigen Reihenfolge.

Die Rennstrecke ist auf allen Karten durch schwimmende Bojen markiert: **blau links, orange rechts**. Zwischen den Bojen liegt eine 22 Meter breite Fahrgasse. Es gibt 40 Bojen auf Sonnendeck, 48 in Kupferhafen und 52 auf Frostfjord. Runde Schwimmkörper mit weißem Band und kurzem Markierungsmast ersetzen die spitzen Formen. Die Bojen kennzeichnen jeden Kontrollpunkt; zusätzliche Paare stehen auf breitem Wasser und an wichtigen Kurven. Ein Pfeil oben in der Mitte weist entlang der nächsten Kurve. Die Minikarte markiert den nächsten Kontrollpunkt. Bei falscher Fahrtrichtung oder einem verpassten Kontrollpunkt erscheint ein deutlicher Hinweis; R bringt das Boot zum letzten gültigen Kontrollpunkt zurück.

Kontrollpunkte zählen beim Vorwärtsdurchfahren der gesamten markierten Fahrgasse. Auch eine schnelle Fahrt zwischen zwei Bildern wird erfasst. Die Platzanzeige berücksichtigt die erreichten Kontrollpunkte, Runde und die zurückgelegte Strecke entlang der Kurven. Eine Abkürzung außerhalb der Strecke ersetzt keinen ausgelassenen Kontrollpunkt.

## Steuerung

| Aktion | Tastatur | Gamepad |
|---|---|---|
| Gas / Bremse | W / S oder Pfeile | Trigger |
| Lenken | A / D oder Pfeile | Linker Stick |
| Boost | Umschalt | Linke Schultertaste |
| Item einsetzen | Leertaste | Untere Aktionstaste |
| Nozzle rückwärts abfeuern | S / Pfeil unten + Leertaste | Stick zurück oder linker Trigger + untere Aktionstaste |
| Letzter Kontrollpunkt | R | Obere Aktionstaste |
| Filament wechseln | C | Im Menü |
| Grafik und Ton | G | Im Menü |
| Vollbild / Fenster | F11 | Im Menü |
| Menü / zurück | Escape | Mit Maus im Menü |

Nach einem Unfall setzt R das Boot aufrecht zum letzten erreichten Kontrollpunkt, entfernt Restgeschwindigkeit und richtet es zur Fahrtrichtung aus. Die Runde bleibt erhalten. Bei längerem Feststecken an Land oder einem gekenterten Boot erfolgt auch eine automatische Bergung. Nach dem Rücksetzen besteht kurz Schutz vor Items; erneutes Rücksetzen ist nach zwei Sekunden möglich.

## Items und Makercoins

Filamentrollen geben zufällig eine Standard Nozzle, eine Lenknozzle, einen kurzen Turbo oder ein Schutzschild. Eine Rolle wird nur eingesammelt, wenn der Itemplatz frei ist. Makercoins sind auf zehn begrenzt. Ein ungeschützter Projektiltreffer kostet alle Makercoins und bremst das Boot kurz aus. Ein aktives Schild fängt einen Treffer ab. Sammelobjekte erscheinen nach einer kurzen Zeit wieder.

Die Standard Nozzle fliegt geradeaus. Die Lenknozzle verfolgt den Fahrer direkt vor dir in der Rennwertung und folgt dabei dem Wasserweg durch Kurven. S beziehungsweise eine Rückwärtseingabe beim Abfeuern richtet beide Nozzles nach hinten. Die Lenknozzle trifft dann den Fahrer direkt hinter dir; als Führender also den Zweiten. Als Erster vorwärts eingesetzt bleibt sie im Inventar und erklärt den Rückwärtsschuss. Es gibt keinen automatischen Angriff auf den Letztplatzierten. Das Ziel wird beim Abschuss festgelegt und wechselt nicht auf andere Fahrer. Ein Schild fängt die Nozzle ab; ein verschwundenes oder bereits im Ziel angekommenes Ziel beendet die Verfolgung. Inseln und Felsen blockieren Schüsse.

Unten links zeigt ein großer Itemplatz Symbol, Namen und die Benutzungstaste. Die Lenknozzle zeigt zusätzlich den vorgesehenen Gegner; ihre blaue Spur unterscheidet sie vom geraden Standard-Schuss. Beim Einsammeln pulsiert das Symbol; aktive Items zeigen ihre Restzeit. Das Schild zieht animierte blaue Schutzringe um das Boot und zerbricht bei einem abgefangenen Treffer. Der Turbo erzeugt zwei leuchtende Strahlen am Heck, eine kurze nachlaufende Spur und eine sanfte Kameraweitung. Treffer erzeugen einen sichtbaren Funkenstoß. Diese Effekte sind auch für Mitspieler sichtbar. Die Anzeige skaliert mit der Bildschirmhöhe.

Alle Benchys zeigen FDM-Druckschichten, die am Modell befestigt sind und sich mit dem Boot bewegen. Prusament Galaxy Black trägt vereinzelte kleine, selbstleuchtende Sterne in Weißblau, Violett und warmem Gold. Die Sterne funkeln unabhängig voneinander und reagieren auf den Blickwinkel. Der schwarze Filamentkörper bleibt sichtbar; graue Metallflecken entfallen. Prusament Glow in the Dark ist bei Tageslicht hellgrün und leuchtet im Dunkeln grün. Es stehen insgesamt sechs Filamentfarben zur Auswahl. Die Kamera verwendet keine Bewegungs- oder Tiefenunschärfe und rendert mit voller Auflösung und leichter Schärfung.

Musik, eigene Geräusche für Sammeln, Countdown, Items, Treffer und Zieleinlauf sowie Galaxy-Glitzer sind enthalten. Das Menü speichert Filamentfarbe, Musiklautstärke und Grafikeinstellungen. Es bietet Qualitätsstufen, Auflösung, Fenster/Vollbild, VSync und Bildlimit. Das Menü pausiert ein aktives Einzelrennen. Nach dem Zieleinlauf sowie im gemeinsamen Rennen läuft die Fahrt beim Öffnen des Menüs weiter.

Version 1.5.0 integriert sechs neue Fab-Packs, stellt Frostfjord als Winterwelt wieder her und ergänzt den persönlichen Zieleinlauf mit automatischer Weiterfahrt und Live-Ergebnissen. Die seitliche Menüansicht, Kartenübersicht, Druckanimation und leuchtenden Galaxy-Sterne bleiben enthalten. Der Abstimmungsstand steht in MECHANIKEN-UND-AUSBLICK.md.

## Mit Freunden spielen

1. Alle verwenden dieselbe Version.
2. Der Gastgeber wählt Strecke und „Online-Lobby erstellen“.
3. Freunde geben unter „Mit Freunden verbinden“ die erreichbare IP-Adresse oder den Hostnamen des Gastgebers ein. Die Lobby zeigt verbundene Fahrer.
4. Der Gastgeber startet das Rennen. Freie Plätze bis sechs Fahrer werden mit Gegnern aufgefüllt; bis zu acht Menschen können teilnehmen.
5. Nachdem alle Teilnehmer fertig oder ausgeschieden sind, kann der Gastgeber über das Menü eine andere Strecke für alle laden. Danach startet er das nächste Rennen.

Im selben Heimnetz die lokale IPv4-Adresse des Hosts verwenden. Über das Internet muss UDP-Port 7777 zum Host erreichbar sein, gegebenenfalls durch eine Portfreigabe im Router und die Windows-Firewall. Alternativ können alle im gleichen privaten VPN-Netz spielen. Bei abweichendem Port die Adresse als Host:Port eingeben. Die Windows-Netzwerkeigenschaften zeigen die lokale IPv4-Adresse an. 127.0.0.1 bezeichnet nur den eigenen PC.

Es gibt keine öffentliche Spielersuche, Vermittlungsserver oder automatische Routerfreigabe. Laufende Rennen nehmen keine neuen Fahrer auf. Bei einem Verbindungsfehler kehrt das Spiel mit einer Meldung ins Hauptmenü zurück. Wird der Host beendet, endet die gemeinsame Sitzung.

## Prüfungen und Grenzen

BuildStatus.json und Validation dokumentieren die tatsächlich durchgeführten Prüfungen: befahrbare Korridore aller drei Strecken, komplette Rennen mit Gegnern, Unfall-Rücksetzen, Itemtreffer, Münzverlust und Übertragung zwischen zwei getrennten Spielprozessen. Netzwerkprüfungen laufen lokal auf einem Computer; Verbindungen über fremde Router und reale Internetlatenz sind damit nicht überprüft. Die Spielaufnahmen stammen aus der gebauten Windows-Version. Ein manueller Eingabe- und Spieltest ist damit nicht ersetzt.

Die Gegner fahren physikalisch und nutzen Items. Sie sind keine menschlichen Gegner. Wasserfall, Schaum und die Hafenarchitektur bleiben gestalterisch einfacher als hochwertige Film- oder Konzeptgrafik.

## Projekt vorbereiten

Das Quellarchiv enthält den Spielcode, eigene Materialien, selbst erzeugte Audioquellen und Importwerkzeuge. Die separat lizenzierten Originaldateien aus Electric Dreams, den Fab-Packs und kopierte Scan-Texturen sind nicht darin enthalten. Der fertige Installer enthält die zum Spielen benötigten gekochten Assets.

1. Electric Dreams Environment 5.8 über den Epic Games Launcher herunterladen.
2. Tools/import_electric_dreams_files.py --copy ausführen; bei anderem Cache-Ordner den Quellpfad anpassen.
3. Tools/Prepare-Unreal.ps1 kompiliert den Editor und erzeugt die Grundassets und Inselmaterialien.
4. Tools/import_race_sounds.py im Unreal-Python-Commandlet ausführen, um die mitgelieferten WAV-Dateien zu importieren.
5. Die sechs in FAB-ASSETS.md genannten Packs im Epic-Cache bereitstellen. Rock_Collection_04 aus dem Vault-Cache nach Content/Rock_Collection_04 kopieren; Tools/import_fab_worlds.py im Unreal-Python-Commandlet ausführen. Pfade bei abweichendem Cache anpassen.
6. Tools/build_printed_filament.py im Unreal-Python-Commandlet ausführen, um den aktuellen Galaxy-Look herzustellen.
7. Tools/Package-Windows.ps1 -SkipPreparation erstellt das Windows-Spiel.

Alle drei Welten nutzen den Einstieg Maps/Sonnendeck und unterschiedliche, synchronisierte Streckenlayouts. Tests der Rennregeln: Tools/RaceChecks.cpp. Die opt-in-Prüfungen im Spiel sind ausschließlich über ausdrücklich gesetzte Benchy-Audit-Startparameter aktiv.

## Quellen

Electric Dreams Environment: Epic Games / Quixel, UE-only-Inhalt. Die ausgewählten Modelle und Texturen werden ausschließlich im Unreal-Spiel verwendet und sind keine CC0-Assets. Keine Weitergabe als eigenständige Asset-Sammlung. Auswahl: Validation/ElectricDreamsImport.json. https://www.fab.com/listings/d79688f5-29be-4fb2-a650-2d4a813f5306

Original #3DBenchy: Daniel Norée / Creative Tools, CC0. https://www.3dbenchy.com/license/

Weitere fotografische Texturen und Naturmodelle: Poly Haven, CC0. Quellen siehe AssetSource/Textures/credits.json. https://polyhaven.com/a/boulder_01 und https://polyhaven.com/a/coast_rocks_02

Musik, kurze Spielgeräusche und zusätzliche Geometrie wurden für dieses Spiel erstellt. Filamentnamen sind Marken ihrer Hersteller; kein offizielles Markenprodukt. Das Projektarchiv enthält keinen Unreal Editor. Die Unreal Engine unterliegt den Bedingungen von Epic.

## Geplante Erweiterungen

Freies Bemalen des Benchys ist als spätere Erweiterung vorgemerkt und in dieser Version noch nicht enthalten. Die sechs nutzbaren Fab-Packs sind integriert; weitere Bibliothekseinträge können später ergänzt werden.
