# Benchy Racers

Installierbares 3D-Wasser-Rennspiel für Windows mit dem originalen #3DBenchy. Aktuelles Update: **1.6.1 – Logo-Ebenen und Lobby (Testausgabe)**.

## Download

[Windows-Installer 1.6.1](https://github.com/Wambo-20/BenchyRacers/releases/download/v1.6.1/BenchyRacers-1.6.1-Test-Setup.exe) oder [Portable-ZIP](https://github.com/Wambo-20/BenchyRacers/releases/download/v1.6.1/BenchyRacers-1.6.1-Test-Portable.zip). Nach der Installation **Benchy Racers Unreal Test** starten. Die Testausgabe besitzt einen eigenen Installationsordner; Version 1.5.1 bleibt separat erhalten. Windows 64 Bit und DirectX 12 sind erforderlich, Unreal Engine und Epic Games Launcher nicht.

[Release-Details, Projektarchiv und Prüfsummen](https://github.com/Wambo-20/BenchyRacers/releases/tag/v1.6.1).

## Im Spiel

- Drei Welten: Sonnendeck, Kupferhafen und Frostfjord, mit Flüssen, Kanälen, Felsen und Vegetation.
- Einzelrennen und private Rennen mit bis zu acht Menschen.
- Sieben Filamente mit Druckschichten, funkelndem Galaxy und leuchtendem Glow in the Dark sowie Neon Gelb.
- Standard Nozzle, Lenknozzle, Turbo, Schild, Makercoins und Boost.
- Große Bootsvorschau im Logo-Editor und bis zu acht unabhängig bearbeitbare PNG-Ebenen: Seiten, Deck und Heck, eigene Konturen und gespeicherte Designs.
- Lobby-Chat und Live-Benchys mit Fahrernamen und übertragenen Logos. Späte Teilnehmer warten auf das nächste Rennen.
- Zieleinlauf mit automatischer Weiterfahrt und Live-Ergebnissen, Druckanimation, Grafikoptionen und Musik.

## Mit Freunden spielen

Alle brauchen Version 1.6.1. Der Gastgeber erstellt eine Online-Lobby und schickt den achtstelligen Code links im Menü. Freunde geben ihn unter „Mit Freunden verbinden“ ein. Diese Testausgabe integriert Epic Online Services für P2P-/Relay-Verbindungen ohne manuelle Routerfreigabe. Geräteanmeldung, Lobby-Erstellung und Host-Transport sind geprüft; eine vollständige Verbindung zwischen zwei verschiedenen PCs ist **noch nicht bestätigt**. Chat und Logo-Übertragung sind mit zwei lokalen Spielinstanzen geprüft. Beim Beenden des Gastgebers endet die Sitzung.

## Quellcode

Der aktuelle Spielcode, die Konfiguration ohne private EOS-Zugangsdaten und die Werkzeuge liegen unter [unreal](unreal). Das Projektarchiv im Release enthält zusätzliche eigene Assets. Separat lizenzierte Fab-/Electric-Dreams-Originalassets sind ausgeschlossen und müssen zum Bearbeiten aus der eigenen Bibliothek bereitgestellt werden. Entwicklung mit Unreal Engine 5.8.3. Die fertigen Windows-Downloads enthalten die benötigten Laufzeitassets.

Die ältere Godot-Ausgabe bleibt unter `work/game` und in den bisherigen Releases erhalten.

## Geplant

Mehr Filamente und Hersteller, direkteres Platzieren der Logos mit der Maus sowie weitere Lobby-/Internetverbesserungen. Freies Bemalen ist derzeit nicht vorgesehen. Drag-and-drop aufs Boot ist noch nicht enthalten.

## Credits

- Original #3DBenchy: Daniel Norée / Creative Tools, [CC0](https://www.3dbenchy.com/license/).
- Naturmodelle und Texturen: [Poly Haven](https://polyhaven.com), CC0; Einzelquellen stehen im Projektarchiv.
- Electric Dreams: Epic Games / Quixel, Unreal-only. Fab-Inhalte sind gemäß ihren jeweiligen Lizenzen im fertigen Spiel eingebunden; keine Weitergabe als eigenständige Asset-Sammlung.
- Filamentnamen sind Marken ihrer Hersteller; kein offizielles Markenprodukt.
- Die ältere Godot-Ausgabe enthält ihre Lizenzhinweise unter `docs`.

Für den eigenen Spielcode wurde bisher keine separate Open-Source-Lizenz festgelegt. Die Drittanbieterlizenzen gelten unabhängig davon.

