$ErrorActionPreference = 'Stop'
$root = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$project = Join-Path $root 'APP\MDK\CIMC_APP.uvprojx'
$errors = 0
function Check([bool]$ok, [string]$message) {
    if ($ok) { Write-Host "[OK]   $message" -ForegroundColor Green }
    else { Write-Host "[FAIL] $message" -ForegroundColor Red; $script:errors++ }
}
try {
    [xml]$xml = Get-Content -Raw -LiteralPath $project
    Check $true 'Keil project XML parsed'
} catch {
    Check $false "Keil project XML parse failed: $_"
    exit 1
}
$filePaths = @($xml.SelectNodes('//FilePath') | ForEach-Object { $_.InnerText })
Check (($filePaths | Select-Object -Unique).Count -eq $filePaths.Count) 'No duplicate project files'
$missing = @($filePaths | Where-Object {
    $candidate = [System.IO.Path]::GetFullPath((Join-Path (Split-Path $project) $_))
    -not (Test-Path -LiteralPath $candidate)
})
Check ($missing.Count -eq 0) 'All project files exist'
if ($missing) { $missing | ForEach-Object { Write-Host "       missing: $_" } }
Check (($filePaths | Where-Object { $_ -eq '..\User\contest_main.c' }).Count -eq 1) 'Single contest main entry'
Check (($filePaths | Where-Object { $_ -eq '..\User\contest_irq_all.c' }).Count -eq 1) 'Single contest IRQ entry'
Check (($filePaths | Where-Object { $_ -eq '..\ThirdParty\FreeModbus\mb.c' }).Count -eq 1) 'FreeModbus core included'
$config = Get-Content -Raw -LiteralPath (Join-Path $root 'APP\HeaderFiles\contest_config.h')
Check ($config -match 'CONTEST_MODBUS_SLAVE_ADDRESS\s+[1-9]') 'Modbus address is not broadcast'
Check ($config -match 'CONTEST_SAMPLE_PERIOD_MS\s+[1-9]') 'Sample period is valid'
Check ($config -match 'CONTEST_LOG_PERIOD_MS\s+[1-9]') 'Log period is valid'
Check (Test-Path -LiteralPath (Join-Path $root 'BootLoader\MDK\CIMC_BL.uvprojx')) 'Bootloader project exists'
if ($errors -ne 0) {
    Write-Host "`n$errors check(s) failed." -ForegroundColor Red
    exit 1
}
Write-Host "`nAll static checks passed." -ForegroundColor Cyan
