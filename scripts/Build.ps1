param(
 [string]$EngineRoot = 'C:\Program Files\Epic Games\UE_5.8\Engine',
 [switch]$Editor,
 [switch]$ValidateOnly
)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$projectFile = Join-Path $projectRoot 'RobotRescue.uproject'
$buildTool = Join-Path $EngineRoot 'Build\BatchFiles\Build.bat'
$packageTool = Join-Path $EngineRoot 'Build\BatchFiles\RunUAT.bat'
foreach ($required in @($projectFile, $buildTool, $packageTool, (Join-Path $projectRoot 'Content\Workshop.umap'))) {
 if (!(Test-Path -LiteralPath $required)) { throw "Required file not found: $required. Check the checkout and -EngineRoot." }
}
if ($ValidateOnly) { Write-Host 'Project, workshop map and Unreal build tools found.'; exit 0 }
if ($Editor) {
 & $buildTool RobotRescueEditor Win64 Development "-Project=$projectFile" -WaitMutex -NoHotReloadFromIDE
} else {
 $buildOutput = Join-Path $projectRoot 'Builds'
 & $packageTool BuildCookRun "-project=$projectFile" -noP4 -platform=Win64 -clientconfig=Development -build -cook -stage -pak -archive "-archivedirectory=$buildOutput" -map=/Game/Workshop -unattended -utf8output -NoXGE
}
if ($LASTEXITCODE -ne 0) { throw "Unreal build failed (exit $LASTEXITCODE). See the Unreal build log above." }
Write-Host 'Build completed successfully.'
