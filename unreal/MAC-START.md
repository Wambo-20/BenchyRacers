# Benchy Racers auf Mac – Übergabe

Stand: 1.6.2 mit dem neuen Online-Bewegungspuffer. Noch keine fertig gebaute
oder getestete Mac-Version. Windows-Build und Netzwerkprüfungen sind bestanden.

## Auf dem Mac vorbereiten

1. Mindestens macOS 14.5; Modell, RAM und GPU prüfen. Epic nennt 16 GB als
   Mindest-RAM für die Entwicklung, 32 GB empfohlen. Das sind Editor-Anforderungen,
   keine bereits gemessenen Mindestanforderungen unseres fertigen Spiels.
2. Unreal Engine 5.8, passend zu Windows 5.8.3, über den Epic Games Launcher
   installieren. Xcode 26.1.1 ist empfohlen; 26.4 ist mit UE 5.8 inkompatibel.
   Xcode einmal öffnen und die Einrichtung selbst abschließen.
3. Dieses gesamte Projektpaket entpacken. Es enthält auch die lokalen Assets.
   Fab-Originalassets dürfen nicht als öffentliches Quellcodepaket hochgeladen werden.
4. In Codex den lokalen Mac als Arbeitsort wählen und den entpackten Projektordner
   öffnen. Eine Fernverbindung zum Windows-PC führt Builds weiterhin auf Windows aus.
5. Zuerst Editor starten und Karten sowie Materialien prüfen. Danach
   `bash Tools/Build-Mac.command` auf dem Mac ausführen.

Standardmäßig entsteht eine Universal-Version mit arm64 und x86_64. Für einen
schnelleren ersten M1-Test: `BENCHY_MAC_ARCH=arm64 bash Tools/Build-Mac.command`.
Bei abweichendem Engine-Pfad `BENCHY_UE_ROOT` setzen. Die `.command`-Datei enthält
Unix-Zeilenenden; der Aufruf mit `bash` benötigt keine Ausführungsrechte.

## Vorbereitete Anpassungen

Mac-Grafikprofil: mittlere Qualität, 1600×900, 75 % interne Auflösung, gewöhnliche
Schatten, Bildschirmreflexionen und keine Nanite-/Hardware-Raytracing-Abhängigkeit.
M1 unterstützt keine Nanite-Geometrie und keine virtuellen Shadow Maps.
Nanite-Assets brauchen verwendbare klassische Fallback-Meshes. Das muss nach
dem Mac-Cook geprüft werden, insbesondere importierte Felsen und Vegetation.
Die realen Bildraten auf M1 und älteren Intel-GPUs sind noch nicht gemessen.

PNG-Import nutzt auf dem Mac einen nativen Cocoa-Dateidialog. Windows behält
seinen bisherigen Dialog. EOS-Gerätename ist plattformabhängig; Lobby-Bucket
bleibt auf beiden Plattformen `BenchyRacers-1.6.2`.

## Online-Konfiguration und Prüfungen

Dieses Übertragungspaket enthält keine privaten EOS-Konfigurationswerte. Für
Online-Rennen dieselbe bestehende `BenchyEOS.ini` aus der eigenen Windows-
Installation privat nach `Config/BenchyEOS.ini` kopieren. Nicht in Git einchecken
oder in Logs ausgeben. Der Build kopiert sie zusätzlich in den Laufzeit-Config-
Ordner und legt diesen bei Bedarf an. Bei einer abweichenden Paketstruktur hält
das Skript an, statt die Online-Konfiguration still zu überspringen.
Einzelrennen funktionieren ohne EOS.

Zu prüfen: Cocoa-PNG-Dialog samt Abbrechen und Unicode-Dateinamen; Logo-Ebenen;
alle drei Karten und Asset-Fallbacks; Audio; Wasser und Glow; 60-FPS-Ziel bzw.
geeignete niedrigere Qualität; Mac/Windows-Lobby, Chat, Logos, Rennen, Items und
Rennbeitritt. Bei Universal-Builds auch EOS-Dylibs auf beide Architekturen prüfen.
Ein alter Intel-Mac ist nicht automatisch geeignet: erst Modell, macOS und GPU
prüfen. Keine Unterstützung für ältere macOS-Versionen zugesagt.

Öffentliche Mac-Releases erst nach Prüfung von Codesign/Notarisierung und
Verpackung als Mac-App. Eine Windows-EXE oder ein Windows-Installer ist kein Mac-Spiel.

## Quellen

- https://dev.epicgames.com/documentation/unreal-engine/macos-development-requirements-for-unreal-engine
- https://dev.epicgames.com/documentation/unreal-engine/supporting-universal-binaries-for-macos-in-unreal-engine
