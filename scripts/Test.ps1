$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$exe = Join-Path $projectRoot 'Builds\Windows\RobotRescue\Binaries\Win64\RobotRescue.exe'
if (!(Test-Path -LiteralPath $exe)) { throw 'Package the Windows game first using scripts/Build.ps1.' }
$testRoot = Join-Path $projectRoot ('Saved\Automation\' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $testRoot -Force | Out-Null
$logFile = Join-Path $testRoot 'runtime.log'
$arguments = @('/Game/Workshop', '-RRTest', '-windowed', '-RenderOffscreen', '-ResX=1280', '-ResY=720', '-unattended', '-NoMessaging', ('-UserDir="' + $testRoot + '"'), ('-abslog="' + $logFile + '"'))
$testProcess = Start-Process -FilePath $exe -ArgumentList $arguments -WindowStyle Hidden -PassThru
if (!$testProcess.WaitForExit(600000)) { $testProcess.Kill(); throw "Runtime checks timed out. Logs: $testRoot" }
if ($testProcess.ExitCode -ne 0) { throw "Runtime exited with $($testProcess.ExitCode). Logs: $testRoot" }
$receiptFile = Get-ChildItem -LiteralPath $testRoot -Recurse -File -Filter 'RobotRescueTestResults.json' | Select-Object -First 1
if (!$receiptFile) { throw "No test receipt found. Logs: $testRoot" }
$receipt = Get-Content -LiteralPath $receiptFile.FullName -Raw | ConvertFrom-Json
if (!$receipt.passed) { $receipt.checks | Where-Object { !$_.passed } | Format-Table; throw 'Gameplay checks failed.' }
Write-Host "$($receipt.checks.Count) gameplay checks passed. Receipt: $($receiptFile.FullName)"
