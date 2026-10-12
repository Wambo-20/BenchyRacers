param([string]$EngineRoot,[switch]$SkipPreparation,[string]$OutputDirectory)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'Find-Engine.ps1')
$taskEngine = Find-BenchyEngine $EngineRoot
$taskProjectRoot = Split-Path $PSScriptRoot -Parent
if (-not $SkipPreparation) {
    & (Join-Path $PSScriptRoot 'Prepare-Unreal.ps1') -EngineRoot $taskEngine
}
if (-not (Test-Path -LiteralPath (Join-Path $taskProjectRoot 'Content\Maps\Sonnendeck.umap'))) {
    throw 'Sonnendeck fehlt. Führe zuerst Prepare-Unreal.ps1 aus.'
}
$taskProject = Join-Path $taskProjectRoot 'BenchyRacers.uproject'
$taskOutput = Join-Path $taskProjectRoot 'BuildOutput'
if ($OutputDirectory) {$taskOutput=$OutputDirectory}
& (Join-Path $taskEngine 'Engine\Build\BatchFiles\RunUAT.bat') BuildCookRun "-project=$taskProject" -noP4 -platform=Win64 -clientconfig=Development -build -cook -stage -pak -package -prereqs -nodebuginfo -archive "-archivedirectory=$taskOutput" -utf8output
if ($LASTEXITCODE -ne 0) {throw 'Windows-Paket konnte nicht erstellt werden. Siehe Saved\Logs.'}
Write-Host "Windows-Paket erstellt unter $taskOutput"
