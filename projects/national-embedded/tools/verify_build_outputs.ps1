$ErrorActionPreference = 'Stop'
$root = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$items = @(
    (Join-Path $root 'APP\MDK\output\Project.axf'),
    (Join-Path $root 'APP\MDK\output\Project.hex'),
    (Join-Path $root 'APP\MDK\output\App.bin'),
    (Join-Path $root 'BootLoader\MDK\output\Project.axf'),
    (Join-Path $root 'BootLoader\MDK\output\Project.hex')
)
foreach ($path in $items) {
    if (-not (Test-Path -LiteralPath $path)) { throw "Missing build output: $path" }
    $item = Get-Item -LiteralPath $path
    if ($item.Length -le 0) { throw "Empty build output: $path" }
    Write-Host ("[OK] {0} ({1} bytes)" -f $item.FullName, $item.Length)
}
