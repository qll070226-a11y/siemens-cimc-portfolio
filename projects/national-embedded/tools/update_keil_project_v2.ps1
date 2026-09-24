$ErrorActionPreference = 'Stop'
$Project = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\APP\MDK\CIMC_APP.uvprojx'))
[xml]$xml = Get-Content -Raw -LiteralPath $Project
$target = $xml.SelectSingleNode('/Project/Targets/Target')
$includeNode = $xml.SelectSingleNode('/Project/Targets/Target/TargetOption/TargetArmAds/Cads/VariousControls/IncludePath')
$paths = @($includeNode.InnerText -split ';' | Where-Object { $_ })
foreach ($path in @('..\ThirdParty\FreeModbus\include','..\ThirdParty\FreeModbus\port','..\ThirdParty\FreeModbus\rtu')) {
    if ($paths -notcontains $path) { $paths += $path }
}
$includeNode.InnerText = $paths -join ';'
$groups = $target.SelectSingleNode('Groups')
foreach ($groupName in @('Contest','FreeModbus')) {
    foreach ($node in @($groups.SelectNodes("Group[GroupName='$groupName']"))) { [void]$groups.RemoveChild($node) }
}
function Add-FileGroup([string]$name, [string[]]$files) {
    $group = $xml.CreateElement('Group'); $groupName = $xml.CreateElement('GroupName')
    $groupName.InnerText = $name; [void]$group.AppendChild($groupName)
    $filesNode = $xml.CreateElement('Files')
    foreach ($path in $files) {
        $file = $xml.CreateElement('File'); $fileName = $xml.CreateElement('FileName')
        $fileName.InnerText = [System.IO.Path]::GetFileName($path)
        $fileType = $xml.CreateElement('FileType'); $fileType.InnerText = '1'
        $filePath = $xml.CreateElement('FilePath'); $filePath.InnerText = $path
        [void]$file.AppendChild($fileName); [void]$file.AppendChild($fileType); [void]$file.AppendChild($filePath)
        [void]$filesNode.AppendChild($file)
    }
    [void]$group.AppendChild($filesNode); [void]$groups.AppendChild($group)
}
$mainFile = $groups.SelectSingleNode("Group/Files/File[FilePath='..\User\main.c']")
$mainFile.FileName = 'contest_main.c'; $mainFile.FilePath = '..\User\contest_main.c'
$irqFile = $groups.SelectSingleNode("Group/Files/File[FilePath='..\User\gd32f4xx_it.c']")
$irqFile.FileName = 'contest_irq_all.c'; $irqFile.FilePath = '..\User\contest_irq_all.c'
Add-FileGroup 'Contest' @('..\Function\contest_adc.c','..\Function\contest_app.c','..\Function\contest_modbus.c','..\Function\contest_storage_runtime.c')
Add-FileGroup 'FreeModbus' @('..\ThirdParty\FreeModbus\mb.c','..\ThirdParty\FreeModbus\rtu\mbrtu.c','..\ThirdParty\FreeModbus\rtu\mbcrc.c','..\ThirdParty\FreeModbus\functions\mbfunccoils.c','..\ThirdParty\FreeModbus\functions\mbfuncdisc.c','..\ThirdParty\FreeModbus\functions\mbfuncholding.c','..\ThirdParty\FreeModbus\functions\mbfuncinput.c','..\ThirdParty\FreeModbus\functions\mbfuncother.c','..\ThirdParty\FreeModbus\functions\mbutils.c','..\ThirdParty\FreeModbus\port\contest_port.c')
$settings = New-Object System.Xml.XmlWriterSettings; $settings.Indent = $true; $settings.Encoding = New-Object System.Text.UTF8Encoding($false)
$writer = [System.Xml.XmlWriter]::Create($Project, $settings)
try { $xml.Save($writer) } finally { $writer.Dispose() }
Write-Host "Updated: $Project"
