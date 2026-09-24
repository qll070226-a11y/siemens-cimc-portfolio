$ErrorActionPreference = 'Stop'
$root = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$project = Join-Path $root 'APP\MDK\CIMC_APP.uvprojx'
$errors = 0
function Check([bool]$ok, [string]$message) {
    if ($ok) { Write-Host "[OK]   $message" -ForegroundColor Green }
    else { Write-Host "[FAIL] $message" -ForegroundColor Red; $script:errors++ }
}

try { [xml]$xml = Get-Content -Raw -LiteralPath $project; Check $true 'Keil 工程 XML 可解析' }
catch { Check $false "Keil 工程 XML 无法解析: $_"; exit 1 }
$filePaths = @($xml.SelectNodes('//FilePath') | ForEach-Object { $_.InnerText })
Check (($filePaths | Select-Object -Unique).Count -eq $filePaths.Count) 'Keil 工程无重复源文件'
$missing = @($filePaths | Where-Object { -not (Test-Path -LiteralPath ([System.IO.Path]::GetFullPath((Join-Path (Split-Path $project) $_)))) })
Check ($missing.Count -eq 0) 'Keil 工程引用的文件全部存在'
if ($missing) { $missing | ForEach-Object { Write-Host "       missing: $_" } }
Check (($filePaths | Where-Object { $_ -eq '..\User\contest_main.c' }).Count -eq 1) '国赛 main 入口唯一'
Check (($filePaths | Where-Object { $_ -eq '..\User\contest_irq_all.c' }).Count -eq 1) '国赛中断入口唯一'
Check (($filePaths | Where-Object { $_ -eq '..\ThirdParty\FreeModbus\mb.c' }).Count -eq 1) 'FreeModbus 核心已加入工程'
$config = Get-Content -Raw -LiteralPath (Join-Path $root 'APP\HeaderFiles\contest_config.h')
Check ($config -match 'CONTEST_MODBUS_SLAVE_ADDRESS\s+[1-9]') 'Modbus 从站地址非广播地址'
Check ($config -match 'CONTEST_SAMPLE_PERIOD_MS\s+[1-9]') '采样周期有效'
Check ($config -match 'CONTEST_LOG_PERIOD_MS\s+[1-9]') '日志周期有效'
Check (Test-Path -LiteralPath (Join-Path $root 'BootLoader\MDK\CIMC_BL.uvprojx')) 'Bootloader 工程存在'
if ($errors -ne 0) { Write-Host "`n发现 $errors 项错误。" -ForegroundColor Red; exit 1 }
Write-Host "`n模板静态检查全部通过。" -ForegroundColor Cyan
