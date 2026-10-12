# Fab-Assets in Version 1.5

Sechs heruntergeladene Packs sind in den Rennwelten integriert. Tools/import_fab_worlds.py importiert ihre Originaldateien aus dem lokalen Epic-Cache und erzeugt eigene deckende Materialien. Validation/FabWorldImport.json dokumentiert den Import; BuildStatus.json enthält die Platzierungsprüfungen für jede Karte. Originaldateien werden nicht als eigenständige Assets im Quellarchiv weitergegeben. Das fertig gekochte Spiel enthält die benötigten Spieldaten.

- Rock Collection 04: sieben unterschiedliche Felsformen auf allen Karten.
- Nordic Coastal Cliff: verwitterte Uferfelsen auf allen Karten, mit Schneeüberlagerung in Frostfjord.
- Ice Cliff: gescannte Eiswände in Frostfjord.
- Fresh Windswept Snow: schneebedecktes Gelände in Frostfjord, mit felsigen steilen Flächen.
- Wooden Planks: Stege, Wände, Pfosten und Kisten in Kupferhafen.
- Rusty Metal Sheet: Metalldächer der Hafengebäude.

Texturen sind auf 4K für Schnee und Klippen sowie 2K für die übrigen Oberflächen begrenzt. Alle neuen Felsen haben deckende Materialien und Nanite. Ein Raster über die Modellgrundfläche prüft Bodenkontakt und Abstand zur Strecke; Modelle werden ins echte triangulierte Gelände eingebettet. Wintervarianten der vorhandenen Pflanzen ergänzen die kalte Karte.

Die übrigen Bibliothekseinträge (zwei Huge Nordic Coastal Cliff, Snow Pile, Nordic Beach Rock Formation, Quarry Cliff, Real Stones Pack 01) sind noch nicht heruntergeladen oder integriert. Beach Sand With Pebbles zeigt im Launcher eine UEFN-Referenzlizenz mit gesperrten Quelldatei-Downloads; diese Variante wird nicht im nativen Spiel verwendet. Der Inventarbericht unterscheidet erworbene, heruntergeladene und tatsächlich platzierte Inhalte.
