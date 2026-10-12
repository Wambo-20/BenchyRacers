param([string]$EngineRoot,[switch]$Editor)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'Find-Engine.ps1')
$taskEngine = Find-BenchyEngine $EngineRoot
$taskProjectRoot = Split-Path $PSScriptRoot -Parent
if (-not (Test-Path -LiteralPath (Join-Path $taskProjectRoot 'Content\Maps\Sonnendeck.umap'))) {
    & (Join-Path $PSScriptRoot 'Prepare-Unreal.ps1') -EngineRoot $taskEngine
}
$taskArguments = @(('"'+(Join-Path $taskProjectRoot 'BenchyRacers.uproject')+'"'))
if (-not $Editor) {$taskArguments += @('/Game/Maps/Sonnendeck','-game','-log')}
Start-Process -FilePath (Join-Path $taskEngine 'Engine\Binaries\Win64\UnrealEditor.exe') -ArgumentList $taskArguments -WindowStyle $(if ($Editor) {'Normal'} else {'Hidden'})
