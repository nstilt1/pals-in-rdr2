param([Parameter(Mandatory=$true)][string]$Upstream,[Parameter(Mandatory=$true)][string]$Output)
$ErrorActionPreference='Stop'
$repository=[IO.Path]::GetFullPath((Split-Path $PSScriptRoot))
$Upstream=[IO.Path]::GetFullPath($Upstream)
$Output=[IO.Path]::GetFullPath($Output)
foreach($path in @($Upstream,$Output)){
 if($path -eq $repository -or $path.StartsWith($repository+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)){throw 'Upstream checkout and generated outputs must remain outside the public repository.'}
}
$revision='04c8554fbb44c077afb6a559fdc32294e519f52c'
$actual=git -C $Upstream rev-parse HEAD
if($LASTEXITCODE -ne 0 -or $actual -ne $revision){throw 'Review and pin a supported Capture Kit revision before building.'}
git -C $Upstream diff --quiet HEAD -- crates/ck_core/src/capture.rs LICENSE-MIT
if($LASTEXITCODE -ne 0){throw 'Reviewed upstream source/license changed.'}
$capture=Join-Path $Upstream 'crates\ck_core\src\capture.rs'
if(!(Test-Path -LiteralPath $capture)){throw 'Upstream capture module missing.'}
New-Item -ItemType Directory -Path $Output -Force | Out-Null
$binding=Join-Path $Output 'upstream-binding.rs'
Set-Content -LiteralPath $binding -Value ('#[path = r#"'+$capture+'"#] mod upstream_capture;') -Encoding UTF8
$previous=$env:CAPTURE_KIT_BINDING
try {
 $env:CAPTURE_KIT_BINDING=$binding
 rustc --edition 2021 --crate-type cdylib --crate-name capture_kit_bridge -C opt-level=2 -C panic=abort --out-dir $Output (Join-Path $PSScriptRoot 'CaptureKitBridge.rs')
 if($LASTEXITCODE -ne 0){throw 'Rust DLL build failed.'}
} finally { $env:CAPTURE_KIT_BINDING=$previous }
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'CaptureKitBridge.h') -Destination $Output -Force
Copy-Item -LiteralPath (Join-Path $Upstream 'LICENSE-MIT') -Destination (Join-Path $Output 'CAPTURE-KIT-LICENSE-MIT.txt') -Force
Set-Content -LiteralPath (Join-Path $Output 'NOTICE.txt') -Value @"
Capture Kit Bridge: original adapter code from Pals in RDR2, MIT licensed.
Includes Capture Kit capture rules, copyright (c) 2026 ufodriverr and contributors.
Upstream https://github.com/ufodriverr/capture-kit-bevy revision $revision.
Capture Kit components used under MIT; see CAPTURE-KIT-LICENSE-MIT.txt.
This DLL contains capture math/shake evaluation only. No Bevy runtime, game assets,
animations, recipes, game access or native inventory/weapon-wheel integration.
"@
Copy-Item -LiteralPath (Join-Path $repository 'LICENSE') -Destination (Join-Path $Output 'BRIDGE-LICENSE-MIT.txt') -Force
$dll=Join-Path $Output 'capture_kit_bridge.dll'
[pscustomobject]@{UpstreamRevision=$revision;UpstreamCaptureSHA256=(Get-FileHash -LiteralPath $capture).Hash;DLLSHA256=(Get-FileHash -LiteralPath $dll).Hash;ApiVersion=1;Scope='capture rules and shake evaluation'} | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $Output 'build-manifest.json')
Write-Output "Built $dll; include both MIT license files and NOTICE.txt when redistributing."
