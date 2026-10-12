function Find-BenchyEngine {
    param([string]$EngineRoot)
    if ($EngineRoot -and (Test-Path -LiteralPath (Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor.exe'))) {
        return (Resolve-Path -LiteralPath $EngineRoot).Path
    }
    $taskCandidates = @()
    $taskManifest = 'C:\ProgramData\Epic\UnrealEngineLauncher\LauncherInstalled.dat'
    if (Test-Path -LiteralPath $taskManifest) {
        $taskInstalls = (Get-Content -LiteralPath $taskManifest -Raw | ConvertFrom-Json).InstallationList
        $taskCandidates += $taskInstalls | Where-Object {$_.AppName -like 'UE_5*'} | ForEach-Object {$_.InstallLocation}
    }
    foreach ($taskRegistry in @('HKLM:\SOFTWARE\EpicGames\Unreal Engine','HKCU:\SOFTWARE\Epic Games\Unreal Engine\Builds')) {
        try {
          if (Test-Path $taskRegistry -ErrorAction Stop) {
            $taskValues = Get-ItemProperty $taskRegistry -ErrorAction Stop
            $taskCandidates += $taskValues.PSObject.Properties | Where-Object {$_.Name -notlike 'PS*'} | ForEach-Object {$_.Value}
            $taskCandidates += Get-ChildItem $taskRegistry -ErrorAction Stop | ForEach-Object {(Get-ItemProperty $_.PSPath -ErrorAction Stop).InstalledDirectory}
          }
        } catch {
            # Restricted registry access must not prevent discovery through installation folders.
        }
    }
    $taskCandidates += Get-ChildItem 'C:\Program Files\Epic Games' -Directory -ErrorAction SilentlyContinue | Where-Object {$_.Name -like 'UE_5*'} | ForEach-Object {$_.FullName}
    foreach ($taskCandidate in ($taskCandidates | Where-Object {$_ -is [string]} | Sort-Object -Descending -Unique)) {
        if (Test-Path -LiteralPath (Join-Path $taskCandidate 'Engine\Binaries\Win64\UnrealEditor.exe')) {return $taskCandidate}
    }
    throw 'Unreal Engine 5 wurde nicht gefunden. Installiere sie im Epic Games Launcher oder gib -EngineRoot mit dem Installationsordner an.'
}
