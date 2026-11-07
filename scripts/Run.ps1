param(
 [string]$EngineRoot = 'C:\Program Files\Epic Games\UE_5.8\Engine',
 [switch]$Editor
)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
if ($Editor) {
 $exe = Join-Path $EngineRoot 'Binaries\Win64\UnrealEditor.exe'
 $arguments = @(('"' + (Join-Path $projectRoot 'RobotRescue.uproject') + '"'))
} else {
 $exe = Join-Path $projectRoot 'Builds\Windows\RobotRescue.exe'
 $arguments = @('-windowed', '-ResX=1280', '-ResY=720', '-NoMessaging')
}
if (!(Test-Path -LiteralPath $exe)) { throw "Not found: $exe. Build the project first using scripts/Build.ps1." }
# Run.ps1 is an explicit user request to open the game/editor window.
Start-Process -FilePath $exe -ArgumentList $arguments -WorkingDirectory $projectRoot
