#!/bin/bash
set -euo pipefail
PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
UE_ROOT="${BENCHY_UE_ROOT:-/Users/Shared/Epic Games/UE_5.8}"
ARCH="${BENCHY_MAC_ARCH:-arm64+x86_64}"
if [[ "$(uname -s)" != Darwin ]]; then echo 'Dieses Werkzeug muss auf dem Mac laufen.'; exit 1; fi
OS_VERSION="$(sw_vers -productVersion)"
OS_MAJOR="${OS_VERSION%%.*}"; REST="${OS_VERSION#*.}"; OS_MINOR="${REST%%.*}"
if (( OS_MAJOR < 14 || (OS_MAJOR == 14 && OS_MINOR < 5) )); then echo 'Unreal 5.8 benötigt mindestens macOS 14.5.'; exit 1; fi
if ! XCODE_VERSION="$(xcodebuild -version 2>/dev/null | head -n 1 | awk '{print $2}')"; then
 echo 'Bitte die vollständige Xcode-App installieren, einmal öffnen und die Einrichtung abschließen.'; exit 1
fi
XCODE_MAJOR="${XCODE_VERSION%%.*}"
if (( XCODE_MAJOR < 26 )) || [[ "$XCODE_VERSION" == 26.4* ]]; then
 echo "Xcode $XCODE_VERSION passt nicht zu Unreal 5.8. Empfohlen: Xcode 26.1.1."; exit 1
fi
case "$ARCH" in arm64|x86_64|arm64+x86_64) ;; *) echo 'Ungültige Mac-Architektur.'; exit 1;; esac
if [[ ! -f "$UE_ROOT/Engine/Build/BatchFiles/RunUAT.sh" ]]; then
 echo 'Unreal Engine 5.8 fehlt. BENCHY_UE_ROOT kann auf den tatsächlichen Installationsordner zeigen.'; exit 1
fi
mkdir -p "$PROJECT_ROOT/BuildOutput/Mac"
bash "$UE_ROOT/Engine/Build/BatchFiles/RunUAT.sh" BuildCookRun \
 "-project=$PROJECT_ROOT/BenchyRacers.uproject" -noP4 -platform=Mac \
 "-specifiedarchitecture=$ARCH" "-architecture=$ARCH" -clientconfig=Development \
 -build -cook -stage -pak -package -nodebuginfo -archive \
 "-archivedirectory=$PROJECT_ROOT/BuildOutput/Mac" -utf8output \
 2>&1 | tee "$PROJECT_ROOT/BuildOutput/Mac/Build.log"
if [[ -f "$PROJECT_ROOT/Config/BenchyEOS.ini" ]]; then
 COPIED=0
 while IFS= read -r -d '' DEST; do
  mkdir -p "$DEST/Config"
  cp "$PROJECT_ROOT/Config/BenchyEOS.ini" "$DEST/Config/BenchyEOS.ini"
  COPIED=1
 done < <(find "$PROJECT_ROOT/BuildOutput/Mac" -type d -path '*/Contents/UE/BenchyRacers' -print0)
 if [[ "$COPIED" == 0 ]]; then echo 'EOS-Konfiguration wurde nicht übertragen: Paketstruktur im BuildOutput prüfen.'; exit 1; fi
fi
echo 'Mac-Paket erstellt. Vor dem Weitergeben beide Architekturen und Online-Rennen prüfen.'
echo 'Ein öffentlicher Mac-Download braucht außerdem Signierung und Notarisierung.'
