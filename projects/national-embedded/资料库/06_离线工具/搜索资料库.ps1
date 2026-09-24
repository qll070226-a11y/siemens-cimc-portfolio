param(
    [Parameter(Mandatory = $true, Position = 0)]
    [string]$Keyword
)

$kb = Split-Path -Parent $PSScriptRoot
$files = Get-ChildItem -LiteralPath $kb -Recurse -File |
    Where-Object { $_.Extension -in '.md', '.txt', '.csv', '.log', '.html' }

Select-String -LiteralPath $files.FullName -SimpleMatch -Pattern $Keyword |
    Select-Object Path, LineNumber, Line |
    Format-Table -AutoSize -Wrap
