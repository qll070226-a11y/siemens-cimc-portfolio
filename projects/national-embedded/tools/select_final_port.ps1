$ErrorActionPreference = 'Stop'
$project = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\APP\MDK\CIMC_APP.uvprojx'))
[xml]$xml = Get-Content -Raw -LiteralPath $project
$node = $xml.SelectSingleNode("//File[FilePath='..\ThirdParty\FreeModbus\port\contest_port_runtime.c']")
if ($null -eq $node) { throw 'Runtime port node not found.' }
$node.FileName = 'contest_port_final.c'
$node.FilePath = '..\ThirdParty\FreeModbus\port\contest_port_final.c'
$settings = New-Object System.Xml.XmlWriterSettings
$settings.Indent = $true
$settings.Encoding = New-Object System.Text.UTF8Encoding($false)
$writer = [System.Xml.XmlWriter]::Create($project, $settings)
try { $xml.Save($writer) } finally { $writer.Dispose() }
Write-Host "Selected final port: $project"
