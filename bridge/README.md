# Capture Kit Bridge

Original Windows x64 C ABI adapter for the capture rules in
[Capture Kit (Bevy)](https://github.com/ufodriverr/capture-kit-bevy), reviewed at
`04c8554fbb44c077afb6a559fdc32294e519f52c` (tag 1.0.0).

The upstream project is an original creature-capture survival starter kit,
not a Palworld engine rewrite. Its code is MIT OR Apache-2.0; this bridge builds
the selected capture module under MIT. Its copyright and permission notice
must accompany the resulting DLL. The full evaluation checkout, binaries,
licenses copied for redistribution and generated content remain outside this
source repository. No Palworld or RDR2 content is included.

## Scope

`capture_kit_bridge.dll` exports a version query, capture-chance evaluation
and weighted shake evaluation. It compiles the externally supplied upstream
`crates/ck_core/src/capture.rs` directly, through a minimal original configuration
adapter; it does not vendor that module or build Bevy/the full game/dependency
graph. Only this module was selected for the first DLL. It has no engine/world
access, file I/O, rendering, inventory mutation or RNG; the host supplies rolls.

The same DLL can be called by compatible Windows x64 games/mods through the
header's C ABI. Other platforms require building an appropriate library. The
game-specific adapter still owns eligible-target detection, throwing, capture
world transactions, persistence, crafting and weapon-wheel integration.

`CaptureKitBridge.h` defines plain fixed-layout structs, caller-owned buffers,
versioned exports and error codes. Callers must provide valid aligned pointers
and correct lengths; null and invalid numerical inputs are rejected, but an FFI
cannot validate arbitrary memory addresses. Version 1 rejects dead targets and
requires strictly positive shake weights summing to one. Failed calls leave
output buffers untouched. Do not pass C++ or Rust objects across the boundary.

## Build and verification

Supply a reviewed external checkout and an external output directory:

```powershell
.\bridge\build_capture_bridge.ps1 -Upstream C:\private\capture-kit-bevy -Output C:\private\capture-kit-bridge
```

Requires Rust and the Windows linker/toolchain. The build pins the reviewed
revision and rejects modified upstream capture/license files. It produces the
DLL, header, both relevant MIT license files, a NOTICE and a revision/hash
manifest. Redistribute the notices with the DLL; it is not all original project
code just because the adapter is original. The upstream author grants its
icons/audio under the stated license only to the extent they hold rights;
those media files are not used by this bridge.

Configure the normal CMake build with
`-DCAPTURE_KIT_BRIDGE_DLL=C:/private/capture-kit-bridge/capture_kit_bridge.dll`
to enable a test executable that loads the real DLL, checks exports and struct
sizes, evaluates the upstream formula, and exercises shake and input-error
behavior. These tests passed for the local build. No installation into RDR2 or
integration with the installed ASI has been performed yet.

Upstream `crates/ck_game/src/anim.rs` animates primitive placeholder models
procedurally. It is useful reference for throw/shake/pose timing, not a set of
native skeletal animation clips ready to import into RDR2. Its Bevy-specific
gameplay, crafting and animation code needs a separate adaptation; no claim of
drop-in compatibility is made.
