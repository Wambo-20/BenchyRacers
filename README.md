# Benchy Racers

Installierbares 3D-Wasser-Rennspiel für Windows mit dem originalen #3DBenchy. Aktuelle Version: **1.5.1 – Unreal Engine**.

## Download und Spielen

Den [Windows-Installer](https://github.com/Wambo-20/BenchyRacers/releases/download/v1.5.1/BenchyRacers-Unreal-Setup.exe) herunterladen und ausführen. Alternativ die [Portable-ZIP](https://github.com/Wambo-20/BenchyRacers/releases/download/v1.5.1/BenchyRacers-Unreal-Portable.zip) vollständig entpacken und `BenchyRacers.exe` starten. Windows 64 Bit und eine DirectX-12-Grafikkarte sind erforderlich. Unreal Engine, Epic Games Launcher und Fab-Packs werden zum Spielen nicht benötigt. Der Installer aktualisiert die separate Unreal-Ausgabe.

Die [Release-Seite](https://github.com/Wambo-20/BenchyRacers/releases/tag/v1.5.1) enthält außerdem das Unreal-Projektarchiv und SHA256-Prüfsummen.

- Drei unterschiedliche Welten: **Sonnendeck**, **Kupferhafen** und der verschneite **Frostfjord**, mit Flüssen, Kanälen, gescannten Felsen und Vegetation.
- Einzelrennen gegen fünf Gegner; private Rennen mit bis zu acht Menschen.
- Sieben Filamentfarben mit sichtbaren Druckschichten, funkelndem Prusament Galaxy Black und leuchtendem Prusament Glow in the Dark sowie Neon Gelb. Das normale Gelb ist dunkler und wärmer.
- Filamentrollen mit Standard Nozzle, Lenknozzle, Turbo oder Schild; Makercoins und Boost.
- Seitliches Menü mit großer Benchy-Vorschau, Kartenpanorama, Druckanimation, Grafikoptionen und Musik.
- Persönlicher Zieleinlauf mit automatischer Weiterfahrt und Live-Ergebnissen. Das Rennen läuft weiter, bis alle fertig oder ausgeschieden sind.

**Steuerung:** W/A/S/D oder Pfeile, Shift für Boost, Leertaste für Items, S + Leertaste für den Rückwärtsschuss, R zum letzten Kontrollpunkt, G für Grafik/Ton, F11 für Vollbild und Esc für das Menü. Gamepad wird unterstützt.

## Mit Freunden spielen

Alle verwenden Version 1.5.1. Ein Spieler erstellt eine Online-Lobby; oben im linken Auswahlmenü stehen seine Internet- und Heimnetz-/VPN-Adresse mit Kopierknöpfen. Er schickt die passende Adresse an seine Freunde; sie geben diese im Beitrittsfeld ein. **UDP-Port 7777** muss erreichbar sein. Im Heimnetz reicht die lokale Host-IP; über das Internet gegebenenfalls Router-/Firewallfreigabe oder ein gemeinsames privates VPN verwenden. Es gibt keine öffentliche Spielersuche, Relay-Server oder automatische Routerfreigabe.

Details stehen in der [aktuellen Spielanleitung](unreal/LIESMICH.md). Zwei getrennte lokale Spielprozesse wurden geprüft; Verbindungen über fremde Router sind noch nicht durchgängig getestet.

## Quellcode und ältere Ausgabe

Der aktuelle C++-Spielcode, Konfiguration und Werkzeuge stehen unter [unreal](unreal). Das [Unreal-Projektarchiv](https://github.com/Wambo-20/BenchyRacers/releases/download/v1.5.1/BenchyRacers-Unreal-Projekt.zip) enthält zusätzlich die eigenen Projektassets. Separat lizenzierte Originalassets aus Electric Dreams und Fab sind ausgeschlossen und müssen zum Bearbeiten aus der eigenen Bibliothek bereitgestellt werden. Die Anleitung und `unreal/FAB-ASSETS.md` beschreiben die Vorbereitung für Unreal Engine 5.8.3. Der fertige Windows-Download enthält alle zum Spielen benötigten Assets.

Das ursprüngliche Godot-Projekt bleibt unter `work/game` erhalten. Sein [erstes Release v0.4.1](https://github.com/Wambo-20/BenchyRacers/releases/tag/v0.4.1) ist weiterhin verfügbar. Die Unreal- und Godot-Ausgaben sind im Mehrspielermodus nicht miteinander kompatibel.

## Geplant für kommende Updates

- Mehr Filamente zur Auswahl: zusätzliche Hersteller, Farben und besondere Filament-Looks.
- Eigenes Benchy frei bemalen und individuelle Designs gestalten.

Weitere Filamente und freies Bemalen sind für kommende Updates geplant.

## Prüfungen und Credits

Version 1.5 wurde als Windows-Spiel gebaut und geprüft: komplette Rennen auf allen drei Karten, Navigation, Auftrieb, Rücksetzen, Items und Münzverlust, Filament-/Menüanimationen sowie Zieleinlauf und Ergebnisübertragung zwischen zwei Spielprozessen. Das Update 1.5.1 wurde zusätzlich mit sieben Filamenten, gelbem und grünem Eigenleuchten, Menüansichten, kopierbaren Adressen und zwei getrennten Spielprozessen geprüft. Prüfberichte stehen im Projektarchiv. Freies Bemalen des Benchys ist für später vorgemerkt.

- Original #3DBenchy: Daniel Norée / Creative Tools, [CC0](https://www.3dbenchy.com/license/).
- Naturmodelle und Texturen: [Poly Haven](https://polyhaven.com), CC0; Einzelquellen stehen im Projektarchiv.
- Electric Dreams: Epic Games / Quixel, Unreal-only. Fab-Inhalte sind gemäß ihren jeweiligen Lizenzen im fertigen Spiel eingebunden; keine Weitergabe als eigenständige Asset-Sammlung.
- Filamentnamen sind Marken ihrer Hersteller; kein offizielles Markenprodukt.
- Die ältere Godot-Ausgabe enthält ihre Lizenzhinweise unter `docs`.

Für den eigenen Spielcode wurde bisher keine separate Open-Source-Lizenz festgelegt. Die Drittanbieterlizenzen gelten unabhängig davon.

