param([string]$GameDir,[string]$ScriptHookArchive,[string]$LoaderArchive,[switch]$InstallRuntime,[switch]$BuildTools,[switch]$ValidateOnly)
$ErrorActionPreference='Stop'
try {
 if($InstallRuntime -or $BuildTools){
  if(!(Get-Command winget -ErrorAction SilentlyContinue)){throw 'Install Microsoft App Installer (winget) first: https://learn.microsoft.com/windows/package-manager/winget/'}
  $packages=@('Microsoft.VCRedist.2015+.x64')
  if($BuildTools){$packages+=@('Kitware.CMake','Python.Python.3.12','Git.Git','Microsoft.VisualStudio.2022.BuildTools')}
  foreach($package in $packages){
   if($ValidateOnly){Write-Output "Would install through winget: $package";continue}
   $arguments=@('install','--exact','--id',$package,'--source','winget')
   if($package -eq 'Microsoft.VisualStudio.2022.BuildTools'){$arguments+=@('--override','--wait --passive --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended')}
   & winget @arguments
   # APPINSTALLER_CLI_ERROR_UPDATE_NOT_APPLICABLE means already installed.
   if($LASTEXITCODE -notin @(0,-1978335189)){throw "winget did not complete $package (exit $LASTEXITCODE)."}
  }
  Write-Output 'Development/runtime setup finished. Reopen your terminal.'
  exit 0
 }
 function ReadPath($value,$prompt){if(!$value){$value=Read-Host $prompt};return [IO.Path]::GetFullPath($value.Trim().Trim('"'))}
 $GameDir=ReadPath $GameDir 'RDR2 installation folder'
 $ScriptHookArchive=ReadPath $ScriptHookArchive 'Downloaded official Script Hook RDR2 ZIP'
 $LoaderArchive=ReadPath $LoaderArchive 'Downloaded official Lenny Mod Loader ZIP'
 if(!(Test-Path -LiteralPath (Join-Path $GameDir 'RDR2.exe'))){throw 'RDR2.exe not found in selected game folder.'}
 Add-Type -AssemblyName System.IO.Compression.FileSystem
 $inputs=@(
  @{Archive=$ScriptHookArchive;Names=@('ScriptHookRDR2.dll','dinput8.dll')},
  @{Archive=$LoaderArchive;Names=@('vfs.asi','ModManager.Core.dll','ModManager.NativeInterop.dll','NLog.dll','lml.ini')}
 )
 $payloads=@()
 foreach($dependency in $inputs){
  $zip=[IO.Compression.ZipFile]::OpenRead($dependency.Archive)
  try{
   foreach($name in $dependency.Names){
    $entries=@($zip.Entries | Where-Object { $_.Name -ieq $name -and ($dependency.Archive -ne $LoaderArchive -or $_.FullName -match '(^|/)ModLoader/') })
    if($entries.Count -ne 1){throw "Expected one $name in the dependency archive."}
    if($entries[0].Length -gt 50MB){throw 'Unexpected dependency file size.'}
    $stream=$entries[0].Open();$memory=[IO.MemoryStream]::new()
    try{$stream.CopyTo($memory);$payloads+=@{Name=$name;Bytes=$memory.ToArray()}}finally{$stream.Dispose();$memory.Dispose()}
   }
  }finally{$zip.Dispose()}
 }
 if($ValidateOnly){Write-Output 'Dependency archives validated; no files changed.';exit 0}
 if(Get-Process RDR2 -ErrorAction SilentlyContinue){throw 'Close RDR2 before installing dependencies.'}
 $principal=[Security.Principal.WindowsPrincipal]::new([Security.Principal.WindowsIdentity]::GetCurrent())
 if(!$principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)){
  $arguments=@('-NoProfile','-ExecutionPolicy','Bypass','-File',('"'+$PSCommandPath+'"'),'-GameDir',('"'+$GameDir+'"'),'-ScriptHookArchive',('"'+$ScriptHookArchive+'"'),'-LoaderArchive',('"'+$LoaderArchive+'"'))
  $child=Start-Process powershell.exe -ArgumentList $arguments -Verb RunAs -Wait -PassThru -WindowStyle Hidden
  exit $child.ExitCode
 }
 foreach($payload in $payloads){
  $destination=Join-Path $GameDir $payload.Name
  if(Test-Path -LiteralPath $destination){Write-Output "Keeping existing $($payload.Name)";continue}
  # Extract only exact runtime names, never archive paths, examples or game assets.
  [IO.File]::WriteAllBytes($destination,$payload.Bytes)
  if([Convert]::ToBase64String([IO.File]::ReadAllBytes($destination)) -ne [Convert]::ToBase64String($payload.Bytes)){throw 'Dependency copy verification failed.'}
 }
 Write-Output 'Dependency setup finished. Run Install.bat next. Use -InstallRuntime if the C++ runtime is missing.'
}catch{Write-Error $_;exit 1}
