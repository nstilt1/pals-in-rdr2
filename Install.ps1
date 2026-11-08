param([string]$GameDir,[string]$Plugin,[string]$Model,[string]$Textures,[switch]$ValidateOnly)
$ErrorActionPreference='Stop'
try {
 function InputPath($value,$prompt) {
  if(!$value){$value=Read-Host $prompt}
  return [IO.Path]::GetFullPath($value.Trim().Trim('"'))
 }
 $GameDir=InputPath $GameDir 'RDR2 installation folder'
 $Plugin=InputPath $Plugin 'Compiled RDR2PalPrototype.asi file'
 $Model=InputPath $Model 'Locally prepared a_c_goat_01_merge_4.ydd file'
 $Textures=InputPath $Textures 'Locally prepared a_c_goat_01_merge.ytd file'
 foreach($name in @('RDR2.exe','ScriptHookRDR2.dll','vfs.asi')){
  if(!(Test-Path -LiteralPath (Join-Path $GameDir $name) -PathType Leaf)){throw "Missing prerequisite: $name"}
 }
 foreach($file in @($Plugin,$Model,$Textures)){
  if(!(Test-Path -LiteralPath $file -PathType Leaf)){throw "File missing: $file"}
 }
 if([IO.Path]::GetExtension($Plugin) -ne '.asi'){throw 'Select a compiled .asi plugin.'}
 foreach($file in @($Model,$Textures)){
  $stream=[IO.File]::OpenRead($file)
  try{$header=New-Object byte[] 4;[void]$stream.Read($header,0,4)}finally{$stream.Dispose()}
  if([Text.Encoding]::ASCII.GetString($header) -ne 'RSC8'){throw "Invalid resource header: $file"}
 }
 if($ValidateOnly){Write-Output 'Input validation passed; no files changed.';exit 0}
 if(Get-Process RDR2 -ErrorAction SilentlyContinue){throw 'Close RDR2 before installing.'}
 $identity=[Security.Principal.WindowsIdentity]::GetCurrent()
 $principal=[Security.Principal.WindowsPrincipal]::new($identity)
 if(!$principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)){
  # Elevate only this installation process; no permanent policy changes.
  $arguments=@('-NoProfile','-ExecutionPolicy','Bypass','-File',('"'+$PSCommandPath+'"'),'-GameDir',('"'+$GameDir+'"'),'-Plugin',('"'+$Plugin+'"'),'-Model',('"'+$Model+'"'),'-Textures',('"'+$Textures+'"'))
  $child=Start-Process powershell.exe -ArgumentList $arguments -Verb RunAs -Wait -PassThru -WindowStyle Hidden
  exit $child.ExitCode
 }
 $backup=Join-Path $env:LOCALAPPDATA ('RDR2PalPrototype\backups\'+[guid]::NewGuid().ToString('N'))
 New-Item -ItemType Directory -Path $backup -Force | Out-Null
 $mod=Join-Path $GameDir 'lml\local_creature_prototype'
 New-Item -ItemType Directory -Path $mod -Force | Out-Null
 $items=@(
  @{Source=$Plugin;Relative='RDR2PalPrototype.asi'},
  @{Source=$Model;Relative='lml\local_creature_prototype\a_c_goat_01_merge_4.ydd'},
  @{Source=$Textures;Relative='lml\local_creature_prototype\a_c_goat_01_merge.ytd'}
 )
 $records=@()
 foreach($item in $items){
  $dest=Join-Path $GameDir $item.Relative
  if(Test-Path -LiteralPath $dest){$saved=Join-Path $backup $item.Relative;New-Item -ItemType Directory -Path (Split-Path $saved) -Force | Out-Null;Copy-Item -LiteralPath $dest -Destination $saved}
  $hash=(Get-FileHash -LiteralPath $item.Source -Algorithm SHA256).Hash
  if([IO.Path]::GetFullPath($item.Source) -ne [IO.Path]::GetFullPath($dest)){Copy-Item -LiteralPath $item.Source -Destination $dest -Force}
  if((Get-FileHash -LiteralPath $dest -Algorithm SHA256).Hash -ne $hash){throw "Copy verification failed: $dest"}
  $records+=@{Path=$dest;SHA256=$hash}
 }
 $configuration=@{
  'lml\local_creature_prototype\install.xml'='<EasyInstall><Name>Local Creature Prototype</Name><Author>Local project</Author><Version>0.1.2</Version><Resources><Resource><StreamingFile>a_c_goat_01_merge_4.ydd</StreamingFile><StreamingFile>a_c_goat_01_merge.ytd</StreamingFile></Resource></Resources></EasyInstall>'
  'RDR2PalPrototype.ini'="[Prototype]`r`nDebugHotkeyEnabled=1`r`nDryRun=0`r`nRadiusMeters=25`r`nReplacementModel=a_c_goat_01`r`nSourceModel=`r`nAssetBuild=local-user-selected`r`n"
 }
 foreach($relative in $configuration.Keys){
  $dest=Join-Path $GameDir $relative
  if(Test-Path -LiteralPath $dest){$saved=Join-Path $backup $relative;New-Item -ItemType Directory -Path (Split-Path $saved) -Force | Out-Null;Copy-Item -LiteralPath $dest -Destination $saved}
  Set-Content -LiteralPath $dest -Value $configuration[$relative] -Encoding UTF8
 }
 $modsPath=Join-Path $GameDir 'lml\mods.xml'
 if(Test-Path -LiteralPath $modsPath){Copy-Item -LiteralPath $modsPath -Destination (Join-Path $backup 'mods.xml');[xml]$mods=Get-Content -LiteralPath $modsPath -Raw}
 else{[xml]$mods='<ModsManager><Mods/><LoadOrder/></ModsManager>'}
 $entry=$mods.SelectSingleNode('/ModsManager/Mods/Mod[@folder="local_creature_prototype"]')
 if(!$entry){$entry=$mods.CreateElement('Mod');$entry.SetAttribute('folder','local_creature_prototype');[void]$mods.SelectSingleNode('/ModsManager/Mods').AppendChild($entry)}
 foreach($pair in @(@('Name','Local Creature Prototype'),@('Enabled','true'),@('Overwrite','false'))){$node=$entry.SelectSingleNode($pair[0]);if(!$node){$node=$mods.CreateElement($pair[0]);[void]$entry.AppendChild($node)};$node.InnerText=$pair[1]}
 if(!$mods.SelectSingleNode('/ModsManager/LoadOrder/Mod[text()="local_creature_prototype"]')){$node=$mods.CreateElement('Mod');$node.InnerText='local_creature_prototype';[void]$mods.SelectSingleNode('/ModsManager/LoadOrder').AppendChild($node)}
 $mods.Save($modsPath)
 $records | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $backup 'installed-files.json')
 Write-Output "Installed. Backups: $backup"
} catch {Write-Error $_;exit 1}
