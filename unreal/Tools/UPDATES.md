## Online-Fix und Updates im Spiel

- Online-Bewegung geglaettet: Position, Drehung, Geschwindigkeit und Zeitstempel werden gemeinsam uebertragen. Andere Benchys fahren auf einer kurz gepufferten Zeitleiste; kurze Paketluecken werden begrenzt ueberbrueckt.
- Steuerdaten werden mit 30 statt 20 Meldungen pro Sekunde uebertragen. Ruecksetzen und Rennstart springen direkt zur richtigen Position.
- Windows-Hauptmenue mit Update-Pruefung und „Jetzt aktualisieren (Spiel wird beendet)“. Der Helfer laedt nur das vorbereitete Teilupdate, prueft die Dateien und startet das Spiel wieder.
- Logos, Einstellungen und die bestehende Online-Konfiguration bleiben beim Teilupdate erhalten. Bei einem Installationsfehler wird die vorherige Dateiversion wiederhergestellt.

## Von 1.6.1 aktualisieren: nur etwa 126 MB

**BenchyRacers-Updater.exe** herunterladen und bei geschlossenem Spiel ausfuehren. Die Installation „Benchy Racers Unreal Test“ wird erkannt; bei einer Portable-Ausgabe den Spielordner waehlen. Der Updater laedt das Teilupdate selbst herunter. Die Update-ZIP und UpdateManifest.json muessen nicht manuell entpackt oder kopiert werden.

**Ab 1.6.2** lassen sich spaetere Updates direkt im linken Hauptmenue starten. Ein Klick beendet das Spiel und startet den eingebauten Helfer. Als Gastgeber vorher die Lobby schliessen. Fuer deutlich aeltere oder veraenderte Installationen steht der volle **BenchyRacers-1.6.2-Setup.exe** bereit. Der Installer aktualisiert weiterhin den Ordner der bisherigen Testausgabe; die aeltere separate Installation bleibt erhalten.

Alle Mitspieler und der Gastgeber brauchen **1.6.2**. Windows 64 Bit und DirectX 12; Unreal Engine und Epic Games Launcher sind zum Spielen nicht notwendig. Die bisherigen Welten, Filamente, Logo-Ebenen, Chat und Items bleiben enthalten.

## Geprueft

Bewegungsberechnung mit unregelmaessigen Meldungen, Paketluecken, Drehung und Respawn. Zwei lokale Spielinstanzen mit 100 ms kuenstlicher Paketverzoegerung, Schwankungen bis 40 ms und 3 % Paketverlust: Chat und Logo-Uebertragung sowie spaeter Beitritt. Updater: korrekte Dateipruefung, Ablehnung beschaedigter Downloads und einer falschen Ausgangsversion, Schutz persoenlicher Dateien, Ruecknahme nach einem simulierten Installationsfehler. Installer-Inhalt und Teilupdate sind anhand ihrer Pruefsummen geprueft.

Der Fix muss noch gemeinsam am Internetanschluss des Freundes ausprobiert werden. Die lokale Pruefung ersetzt diesen Test nicht. Mac ist in diesem Release noch kein fertiger Spieldownload.

## Geplant

Mehr Filamente, Hersteller und besondere Farben, weitere Verbesserungen an Online-Verbindungen und der Lobby. Der Updater ist fuer weitere vorbereitete Teilupdates vorgesehen; bei grossen neuen Weltdaten kann der Download entsprechend groesser sein.

Der Quellcode dieses Releases und der Updater liegen im Repository. Private EOS-Konfiguration und separat lizenzierte Fab-Originalassets werden nicht als Quellcode veroeffentlicht. Aeltere Releases bleiben verfuegbar.


Updater bauen: .NET Framework C# Compiler; Referenzen System.Web.Extensions, System.IO.Compression, System.IO.Compression.FileSystem, System.Windows.Forms. Manifest schema 1 mit zielversion und patches je Ausgangsversion. Aufwaertsupdate laedt SHA256-gepruefte Dateipakete; keine binaeren Differenzen innerhalb einer Datei. Nur geaenderte Dateien werden gepackt.
