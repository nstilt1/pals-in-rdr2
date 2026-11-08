# Development

The playable prototype source is in `src/`. The earlier `OriginalRuntimeBridge` project is a separate host-neutral skeleton.

## Build

Use Windows, Visual Studio 2022 C++ tools, Windows SDK, CMake, Python 3, and a separately obtained Script Hook RDR2 SDK. Keep the SDK and build output outside this repository.

```powershell
cmake -S . -B ../prototype-build -G "Visual Studio 17 2022" -A x64 -DSCRIPTHOOK_SDK_ROOT="C:/path/to/external-sdk"
cmake --build ../prototype-build --config Release
ctest --test-dir ../prototype-build -C Release --output-on-failure
python tools/check_repository.py
```

The plugin is `../prototype-build/Release/RDR2PalPrototype.asi`.

## Local installation inputs

`Install.bat` launches `Install.ps1`, which asks for the game directory, compiled ASI, locally generated YDD model dictionary, and shared YTD texture dictionary. Install Script Hook RDR2, an ASI loader, and Lenny's Mod Loader separately from their authors. The installer checks prerequisites and resource headers, requests Windows elevation, backs up overwritten files, verifies copied hashes, and enables its entry while retaining other LML entries. It does not download, extract, or generate game assets. Header checks do not prove arbitrary user-selected resources are valid in-game.

The current working local assets are asset build **0.1.2**. A fresh clone does not include those assets or a turnkey asset-conversion pipeline. `experiments/` preserves original conversion scripts for development; external tools and intermediate local inputs are still required. Set `RDR2PAL_LOCAL_WORKSPACE` to a private directory outside this repository before using those experiments. Their directory conventions refer to the development workspace, not a fresh game installation. Shared-texture conversion and writer orchestration are not yet a complete portable build command.

The prototype uses a goat appearance replacement, so existing goats also use the replacement. F8 targets compatible nearby animals; protected mission actors, humans and horses are excluded. Rendering was confirmed by the user; further animation, skinning, mission and stability validation remains ongoing.

## Repository boundary

Publish only this source directory. Do not publish its parent workspace, game directories, generated assets, SDKs, downloaded dependencies, logs, dumps or backups. `.gitignore` is a convenience; `tools/check_repository.py` also rejects non-allowlisted files, including ignored files. Review text for accidental proprietary content and credentials before committing.

```powershell
git init
git config core.hooksPath .githooks
python tools/check_repository.py
git add .
python tools/check_repository.py --staged
```

No GitHub upload is performed by the installer. The project distributes original source/configuration only; ownership of games is not a guarantee of permission to redistribute derived content.

## Diagnostics

Plugin **0.2.3** logs UTC times, version, settings, native-call BEGIN/END markers, post-swap frames and heartbeat messages in `%LOCALAPPDATA%\RDR2PalPrototype\RDR2PalPrototype.log` (temporary-directory fallback). `freeze_probe` can capture local thread stacks from a frozen process with appropriate Windows permissions. Do not commit those captures.
