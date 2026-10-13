# Online-Lobbys ohne Portfreigabe

Im Hauptmenü kannst du dein Filament und PNG-Logo wählen und speichern.
Mit **Online-Lobby erstellen** wirst du Gastgeber. Gib deinen Freunden den
achtstelligen **Lobby-Code** links im Menü. Sie geben ihn unter **Mit Freunden
verbinden** ein. Alle brauchen dieselbe Spielversion. Der Gastgeber startet
das Rennen; seine Spielinstanz muss während der gemeinsamen Sitzung laufen.

Die Verbindung verwendet Epic Online Services mit Geräteanmeldung. Ein
Epic-Konto ist für den Spieler nicht erforderlich. Verbindungen versuchen
zuerst einen direkten Weg und verwenden bei Bedarf Epics Relay. Es ist keine
eingehende Portfreigabe im Router nötig. Internetzugang und ausgehende
Verbindungen zum Online-Dienst müssen möglich sein.

Der Lobby-Code ist eine Einladungshilfe, kein Passwort für vertrauliche
Daten. Ein Rennspiel besitzt hier keine vertraulichen Lobby-Daten.

## Entwickler-Konfiguration

`Config/BenchyEOS.example.ini` enthält die benötigten Feldnamen. Die echte
Konfiguration wird separat als `Config/BenchyEOS.ini` bereitgestellt oder
beim Test über `-BenchyEOSConfig="absoluter Pfad"` geladen. Sie gehört nicht
in das Quellcode-Repository oder den Quellcode-Download.

Verwende ausschließlich einen eingeschränkten Spiel-Client mit
nutzergebundenen Lobby-Rechten. Keine Server- oder Administrator-Zugangsdaten
im Spiel verteilen. Die Client-Konfiguration für das laufende Spiel ist in
einem ausgelieferten Spiel grundsätzlich zugänglich; ihre Berechtigungen
müssen deshalb eingeschränkt bleiben.

Die aktuelle Richtlinie aktiviert nur `lobbies:connect`, `lobbies:readLobby`
und `lobbies:findLobbies`. Spieleridentitäten werden über EOS Connect mit
Device ID erstellt. Voice-Chat, Freunde-Zugriff, Cloud-Speicher und
Administratorfunktionen sind nicht aktiviert.

Beim Beitritt während eines Rennens wartet der Fahrer in der Lobby auf das
nächste Rennen. Beim Verlassen des Gastgebers endet die gemeinsame Sitzung;
automatischer Gastgeberwechsel ist noch nicht enthalten.

Verbindungsfehler bleiben im Menü sichtbar, lassen sich kopieren und werden
lokal unter `Saved/Diagnostics/Connection.txt` gespeichert.
