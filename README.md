# RDR2 × Palworld Prototype

**Work in progress.** Press **F8** to turn the nearest compatible animal into a **Lamball**.

## Requirements

- Windows 10/11, 64-bit, and RDR2 for PC in **Story Mode**.
- A local Palworld installation for preparing your own assets.
- [Script Hook RDR2](https://www.dev-c.com/scripthookrdr2/) compatible with your game build, with its ASI loader.
- [Lenny's Mod Loader](https://www.rdr2mods.com/downloads/rdr2/tools/76-lennys-mod-loader-rdr/). The working prototype uses beta 11.
- Windows Package Manager (`winget`) for automated prerequisite setup, Microsoft Visual C++ x64 runtime, a compiled prototype `.asi`, and locally prepared model/texture files.

## Install

1. Close RDR2. Download Script Hook RDR2 and Lenny’s Mod Loader ZIPs from the links above. Run **Setup.bat** to install the C++ runtime through winget, then supply the ZIPs to install the mod dependencies into your game folder. Existing dependency files are kept.
2. Double-click **Install.bat** and select your RDR2 folder, compiled prototype `.asi`, and locally prepared model and texture files when prompted.
3. Start RDR2 in **Story Mode** and press **F8** near a compatible animal.

The installer uses your local files. Game assets and third-party tools are not included in this repository. See [BUILD.md](BUILD.md) for building and development progress.

### Optional PowerShell setup

Install the Microsoft C++ runtime through Windows Package Manager:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Install-Dependencies.ps1 -InstallRuntime
```

To build from source, install the development tools (Visual Studio C++ Build Tools with Windows SDK, CMake, Python and Git), then follow [BUILD.md](BUILD.md):

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Install-Dependencies.ps1 -BuildTools
```

Windows Package Manager presents any required agreements/install prompts. Reopen your terminal after installation. The external Script Hook SDK must also be obtained separately. These commands do not generate the local game assets.
