# Aktueller Abstimmungsstand

## Nozzles – Version 1.3

- Standard Nozzle: gerader Schuss nach vorne; S / Pfeil nach unten / linker Stick nach hinten / linker Trigger beim Abfeuern kehrt die Schussrichtung um.
- Lenknozzle: verfolgt den Fahrer direkt vor dem Schützen in der Rennwertung. Die Verfolgung bleibt auf dem Wasserweg und wechselt nicht auf andere Gegner.
- Rückwärts abgefeuert: verfolgt den Fahrer direkt hinter dem Schützen. Ein Führender trifft damit den Zweiten.
- Als Führender vorwärts: kein automatischer Angriff auf den Letzten. Die Lenknozzle bleibt im Inventar und die Anzeige erklärt den Rückwärtsschuss.
- Ohne gültiges Ziel wird keine Lenknozzle verbraucht. Ein Schild fängt sie ab. Verschwindet das bereits anvisierte Ziel oder fährt ins Ziel, endet der Angriff.
- Standard Nozzles werden von Inseln und Felsen blockiert. Die Lenknozzle folgt Kurven und greift nur das beim Abschuss gewählte Boot an.
- Turbo, Schild und Makercoins bleiben Teil des Itemsystems. Alle vier Items können aus Filamentrollen entstehen.

## Filamentdarstellung – Version 1.4.1

- Prusament Galaxy Black: wenige kleine, selbstleuchtende Sterne in Weißblau, Violett und warmem Gold. Unabhängiges Funkeln und wechselnde Lichtreflexe bei Drehung des Bootes; keine dichte graue Metallfleckenschicht.
- Prusament Glow in the Dark: helles Grün im Tageslicht, grüne Eigenleuchtfarbe im Dunkeln.
- Objektfeste FDM-Druckschichten mit feineren Rillen auf den Deckflächen. Muster bewegen sich mit dem Boot.
- Volle Renderauflösung, abgeschaltete Tiefenunschärfe und kein Kamerabewegungsblur; leichte Schärfung.

## Menü und Drucksequenz – Version 1.4

- Linkes Seitenmenü mit drei direkt anklickbaren Karten und sechs Filament-Farbfeldern.
- Große 3D-Vorschau rechts auf einem drehenden Druckbett; gedruckte Oberflächen, Galaxy und Glow bleiben sichtbar.
- Erhöhte Kartenübersicht als tatsächlicher Welt-Hintergrund, keine Perspektive von der Startposition.
- Einzelspiel-Kartenauswahl lädt die gewählte Welt mit gleicher Farbe und gleichem Namen. In Online-Lobbys entscheidet der Host über den gemeinsamen Kartenwechsel.
- Beim Filamentwechsel wird das Boot 2,8 Sekunden von unten aufgebaut. Die Nozzle folgt Konturen aus dem echten FBX-Modell.
- Drucksequenz vor jedem Rennen innerhalb des bestehenden fünfsekündigen Countdowns; Kamerablende zurück in die normale Fahransicht. Kein Eingriff in Rennwertung und Fahrphysik.
- Escape und Weiterfahren stellen die Fahrkamera wieder her; Grafikmenü bleibt im seitlichen Menü erreichbar.

## Welten und Zieleinlauf – Version 1.5

- Drei unterschiedliche Welten: Sonnendeck, Kupferhafen und Frostfjord. Frostfjord ersetzt Smaragdfluss in der Kartenauswahl.
- Sechs Fab-Packs eingebaut: Rock Collection 04, Nordic Coastal Cliff, Ice Cliff, Fresh Windswept Snow, Wooden Planks und Rusty Metal Sheet.
- Deckende Felsen, geprüfte Bodenkontakte und freie Fahrkorridore. Winterkarte mit Schnee, Eiswänden und kaltem Wasser; keine Palmen.
- Der persönliche Zieleinlauf friert Zeit und Platz ein. Das Benchy folgt danach physikalisch und servergesteuert der Strecke weiter.
- Ergebnisfenster mit eigener hervorgehobener Zeile und Live-Zeiten der anderen Fahrer. Unfertige Fahrer zeigen „läuft noch...“, getrennte Teilnehmer „ausgeschieden“.
- Fertige Boote beeinflussen noch fahrende Boote nicht durch Kollisionen oder Items. Das Rennen endet erst, wenn alle Teilnehmer fertig oder ausgeschieden sind; kein pauschales Nachlaufzeitlimit.
- Ein neues Rennen und Kartenwechsel sind erst nach Ende des gemeinsamen Rennens möglich. Alle Ergebnisdaten werden beim Neustart zurückgesetzt.

## Spätere Erweiterungen

- Eigene Benchy-Bemalung: auswählbare Bereiche/Freihandmalen, Lackierungen speichern und im Online-Rennen übertragen. Vom Nutzer ausdrücklich für einen späteren Zeitpunkt vorgesehen; noch nicht implementiert.
- Zusätzliche Bibliothekseinträge können später ergänzt werden. Die sechs tatsächlich verwendeten Packs und die übrigen noch nicht heruntergeladenen Einträge stehen in FAB-ASSETS.md und Validation/FabAssetInventory.json.
