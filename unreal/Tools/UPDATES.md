## Neuer Ausgangspunkt: eigene Filamentnamen

Die sieben Filamente heißen jetzt:

- Rabenschwarz Matt
- Mandarinenrausch
- Lagunenblau
- Honiggold
- Sternenstaub Schwarz
- Glühwürmchengrün
- Neonbanane

Farben, Druckschichten, Glitzer und Leuchteffekte bleiben erhalten. Gespeicherte Auswahlen werden übernommen. Die Filamentnamen sind fiktiv und stellen keine Herstellerkooperation dar.

## Spielen und aktualisieren

**Neuinstallation:** `BenchyRacers-1.6.3-Setup.exe` herunterladen und ausführen. Der Installer ist vollständig; ältere Releases sind dafür nicht erforderlich. Gestartet wird weiterhin über die bestehende Verknüpfung „Benchy Racers Unreal Test“. Windows 64 Bit und DirectX 12; Unreal Engine und Epic Games Launcher werden zum Spielen nicht benötigt.

**Schon 1.6.2 installiert:** Im linken Hauptmenü „Jetzt aktualisieren“ wählen. Das Spiel beendet sich, installiert das Teilupdate und startet wieder.

**Noch 1.6.1 installiert:** Einmal den kleinen `BenchyRacers-Updater.exe` herunterladen und bei geschlossenem Spiel ausführen. Die bisherige Installation wird erkannt; Portable-Spieler wählen ihren Spielordner. Für ältere oder veränderte Installationen bitte den vollständigen Installer verwenden.

Die Teilupdates für 1.6.1 und 1.6.2 werden vollständig aus diesem Release geladen und brauchen keine Downloads aus früheren Releases. Logos, Einstellungen und vorhandene Online-Konfiguration bleiben erhalten. Update-ZIP und UpdateManifest.json müssen nicht manuell kopiert werden.

## Enthalten und geprüft

Drei Welten, Online-Lobbys, Chat, Logo-Ebenen, Items und Makercoins. Der Online-Bewegungsfix und der Ingame-Updater sind enthalten. Alle Mitspieler und der Gastgeber brauchen **1.6.3**.

Natives Windows-Spiel und vollständiger Installer gebaut. Druckschichten, Sternenstaub-Glitzer und beide Leuchtfilamente im Spiel geprüft. Beide Teilupdate-Wege mit den echten Ausgangsdateien geprüft: sämtliche Zieldateien stimmen mit den Prüfsummen überein, persönliche Testdateien bleiben erhalten. Der Updater aus der Vorversion wurde bereits mit echtem GitHub-Download geprüft.

Der Online-Bewegungsfix ist mit künstlicher Verzögerung und Paketverlust lokal geprüft; der gemeinsame Test am Internetanschluss des Freundes steht noch aus. Ein fertiger Mac-Spieldownload ist noch nicht enthalten.

## Projekt und Ausblick

Das Projektarchiv enthält den passenden Quellcode und eigene beziehungsweise CC0-Assets. Separat lizenzierte Fab-/Electric-Dreams-Originalassets sowie private EOS-Konfiguration sind im Projektarchiv ausgeschlossen. Die benötigten Laufzeitassets und der eingeschränkte EOS-Spielclient-Zugang sind im freigegebenen Installer enthalten.

Geplant: weitere Fantasie-Filamente und Effekte sowie Verbesserungen an Lobby und Online-Rennen.

Original #3DBenchy: Daniel Norée / Creative Tools, CC0 1.0 – https://www.3dbenchy.com/license/ . Eigenständiges Community-Spiel ohne behauptete offizielle Unterstützung durch die Urheber des Modells.

Updater: .NET Framework C# mit System.Web.Extensions, System.IO.Compression, System.IO.Compression.FileSystem und System.Windows.Forms. Manifest schema 1. Jede Ausgangsversion hat ihre eigene fromVersion aus BuildStatus.json und alte Dateiprüfsummen; mehrere Ausgangsversionen können dasselbe ZIP mit unterschiedlichen alten Prüfsummen verwenden. Paket-URLs müssen auf die aktuelle Release-Version zeigen.
