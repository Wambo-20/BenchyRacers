param([string]$EngineRoot)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'Find-Engine.ps1')
$taskEngine = Find-BenchyEngine $EngineRoot
$taskProjectRoot = Split-Path $PSScriptRoot -Parent
$taskProject = Join-Path $taskProjectRoot 'BenchyRacers.uproject'
$taskBuild = Join-Path $taskEngine 'Engine\Build\BatchFiles\Build.bat'
Write-Host 'Unreal-Projekt kompilieren …'
& $taskBuild BenchyRacersEditor Win64 Development $taskProject -WaitMutex
if ($LASTEXITCODE -ne 0) {throw 'Unreal-Kompilierung fehlgeschlagen. Siehe Meldungen oben.'}
$taskEditor = Join-Path $taskEngine 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$taskScript = Join-Path $PSScriptRoot 'bootstrap_unreal.py'
Write-Host 'Modelle importieren und Sonnendeck vorbereiten …'
& $taskEditor $taskProject '-run=pythonscript' "-script=$taskScript" -unattended -nosplash -NoAudio -stdout -FullStdOutLogOutput
if ($LASTEXITCODE -ne 0) {throw 'Asset-Vorbereitung fehlgeschlagen. Siehe Saved\Logs.'}
if (-not (Test-Path -LiteralPath (Join-Path $taskProjectRoot 'Content\Maps\Sonnendeck.umap'))) {
    throw 'Sonnendeck wurde nicht erstellt. Siehe Saved\Logs.'
}
$taskIslandScript=Join-Path $PSScriptRoot 'build_island_look.py'
$taskScan=Join-Path $taskProjectRoot 'Content\Megascans\3D_Plants\AlexandraPalm\SM_AlexandraPalm_01.uasset'
if (Test-Path -LiteralPath $taskScan) {
    Write-Host 'Flussinsel-Materialien und Beleuchtung vorbereiten ...'
    & $taskEditor $taskProject '-run=pythonscript' "-script=$taskIslandScript" -unattended -nosplash -NoAudio -stdout -FullStdOutLogOutput
    if ($LASTEXITCODE -ne 0) {throw 'Die Flussinsel-Materialien konnten nicht vorbereitet werden.'}
} else {
    throw 'Die Electric-Dreams-Auswahl fehlt. Zuerst Tools/import_electric_dreams_files.py --copy ausführen.'
}
& $taskEditor $taskProject '-run=pythonscript' "-script=$(Join-Path $PSScriptRoot 'build_item_material.py')" -unattended -nosplash -NoAudio -stdout -FullStdOutLogOutput
if ($LASTEXITCODE -ne 0) {throw 'Item-Material konnte nicht erstellt werden.'}
& $taskEditor $taskProject '-run=pythonscript' "-script=$(Join-Path $PSScriptRoot 'build_navigation_material.py')" -unattended -nosplash -NoAudio -stdout -FullStdOutLogOutput
if ($LASTEXITCODE -ne 0) {throw 'Bojen-Material konnte nicht erstellt werden.'}
& $taskEditor $taskProject '-run=pythonscript' "-script=$(Join-Path $PSScriptRoot 'build_printed_filament.py')" -unattended -nosplash -NoAudio -stdout -FullStdOutLogOutput
if ($LASTEXITCODE -ne 0) {throw 'Druckschichten- und Spezialfilament-Material konnte nicht erstellt werden.'}
Write-Host 'Vorbereitung abgeschlossen. Als Nächstes im Editor spielen und prüfen.'
