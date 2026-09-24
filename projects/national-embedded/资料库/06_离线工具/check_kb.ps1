$ErrorActionPreference = 'Stop'
$kb = Split-Path -Parent $PSScriptRoot
$missing = @()
$files = Get-ChildItem -LiteralPath $kb -Recurse -File
$leafNames = @($files | ForEach-Object { $_.Name })

foreach ($name in @('index.html', 'README.md', 'APP_build.log', 'BootLoader_build.log')) {
    if ($leafNames -notcontains $name) { $missing += $name }
}

if (($files | Where-Object Extension -eq '.pdf').Count -lt 6) { $missing += 'PDF count is less than 6' }
if (($files | Where-Object Extension -eq '.zip').Count -lt 1) { $missing += 'ZIP archive is missing' }
if (($files | Where-Object Extension -eq '.md').Count -lt 20) { $missing += 'Markdown count is less than 20' }

$appLogPath = ($files | Where-Object Name -eq 'APP_build.log' | Select-Object -First 1).FullName
$blLogPath = ($files | Where-Object Name -eq 'BootLoader_build.log' | Select-Object -First 1).FullName
if ($appLogPath) {
    $appLog = Get-Content -LiteralPath $appLogPath -Raw
    if ($appLog -notmatch '0 Error\(s\), 0 Warning\(s\)') { $missing += 'APP build log did not pass' }
}
if ($blLogPath) {
    $blLog = Get-Content -LiteralPath $blLogPath -Raw
    if ($blLog -notmatch '0 Error\(s\), 0 Warning\(s\)') { $missing += 'Bootloader build log did not pass' }
}

if ($missing.Count -gt 0) {
    Write-Host 'Knowledge base check failed:' -ForegroundColor Red
    $missing | ForEach-Object { Write-Host " - $_" }
    exit 1
}

Write-Host "Knowledge base check passed: $($files.Count) files." -ForegroundColor Green
