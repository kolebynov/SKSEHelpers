# AGENTS.md

## Project Scope
- `SKSEHelpers` is a shared C++23 header-only (CMake `INTERFACE`) library of helpers for SKSE plugins built on **CommonLibSSE-NG** (`alandtse/CommonLibSSE-NG`). Target: `SKSEHelpers` (alias `SKSEHelpers::SKSEHelpers`) in `CMakeLists.txt`.
- Code was copied from `STB-Widgets` (Observables, TaskScheduler, ResourceLock, Utilities, TESLookup, PointerLinkedList) and `PrismaUI` (Hooks, TraitUtils, VariantUtils, CallPipeline). Those projects keep their own copies and do **not** consume this library; changes here do not propagate there and vice versa.
- CommonLibSSE-NG is **not** vendored (no submodule). Do not add one.

## Layout
Everything lives in `src/` (headers and sources together); `src/` is the PUBLIC include directory, so consumers write `#include <TaskScheduler.h>`, `#include <Hooks/HookInstaller.h>`.

|Path|What|
|---|---|
|`src/Observables/`|Reactive primitives: `Subject`, `ReactiveObject`, `TrackedValue`, `GenericSubscription` (RAII token), adaptors `Transform`, `Window`, `CombineLatest`; `Observable` concept and `operator\|`.|
|`src/TaskScheduler.h`|`TaskScheduler<TTimestamp>` (default `steady_clock::time_point`) delayed/interval task queue. No singleton: consumer owns instances (non-movable), constructs with an initial timestamp and calls `Tick(now)` (e.g. from a frame hook). Delays are added to the last ticked timestamp, so same-delay tasks scheduled within one frame fire on the same tick.|
|`src/SchedulerUtilities.h`|`Debounce`, `WaitFor` (take a `TaskScheduler<T>&`). No CommonLib.|
|`src/GenericUtilities.h`|`NearlyEqualComparer`, `SafeRange`, `TruePredicate`. No CommonLib.|
|`src/Utilities.h`|Game helpers: `IsActive`, actor active-effect / equipped-item views, `DispelByAdjustElapsedTime`, `ChangeGlobalAtomic`.|
|`src/TESLookup.h`|`LookupByEditorId` / `LookupByFormId` / `LookupByAbsoluteFormId` with mandatory `throwOnError`; debug builds warn on duplicate lookups.|
|`src/ResourceLock.h`|Mutex-guarded value with `Acquire()` lock handles.|
|`src/PointerLinkedList.h`|Range/iterator view over intrusive `next`-pointer lists.|
|`src/Hooks/`|`Hooks.h` (`HookDefinition` concept: address hooks `Id`+`Offset` installed via SKSE trampoline `write_call<5>` (`WriteCall`) or MinHook (`Detour`); vfunc hooks `VTable` (`REL::VariantID` from `RE::VTABLE_*`)+`Index` installed via `write_vfunc` (`VFunc`)), `HookInstaller.h` (wraps a handler around the original via `CallPipeline`; the handler's result is returned to the game; one install per hook type per DLL), `HooksLib.h` (concrete hook definitions from PrismaUI and SkyrimDungeons; the SkyrimDungeons ones are SE-only: AE id / AE+VR offsets are 0, vfunc slots verified on SE only).|
|`src/CallPipeline.h`, `src/TraitUtils.h`, `src/VariantUtils.h`|Generic metaprogramming: call pipelines, `FunctionTraits`/`TupleAppendT`, `Overload`/`Match`.|
|`tests/runtime/`|Catch2 unit tests (`SKSEHelpersTests` executable, run via CTest). Only headers without CommonLib: the target gets `src/` as include path but does not link `SKSEHelpers`/CommonLib, so including a CommonLib-dependent header fails to compile.|
|`tests/compile/`|`static_assert`-only tests for CommonLib-dependent headers (`SKSEHelpersCompileTests` OBJECT target, links `SKSEHelpers`); building it is the test.|

Most helpers are in the global namespace (`ResourceLock`, `TaskScheduler`, `Overload`, `Match`, `IsActive`, …); `Observables`, `Hooks`, `TESLookup` are namespaced. Keep new names unambiguous.

## CommonLibSSE-NG Resolution
`CMakeLists.txt` resolves CommonLib in this order:
1. Existing `CommonLibSSE::CommonLibSSE` target (consumer already added CommonLib) — reused as is.
2. `COMMONLIB_PATH` (`-DCOMMONLIB_PATH=...` or environment variable) → `add_subdirectory(... EXCLUDE_FROM_ALL)` with `BUILD_TESTS OFF`. Missing/invalid path is a configure-time `FATAL_ERROR`.

Also `find_package(minhook CONFIG REQUIRED)`. Consumers' vcpkg manifests must provide CommonLib's deps plus `minhook` (see `vcpkg.json`).

Consumer usage:
```cmake
add_subdirectory("C:/work/SKSEHelpers" "${CMAKE_BINARY_DIR}/SKSEHelpers")
target_link_libraries(MyPlugin PRIVATE SKSEHelpers::SKSEHelpers)
```

## Commands (PowerShell, repo root; needs `VCPKG_ROOT`)
```powershell
cmake --preset default -DCOMMONLIB_PATH=C:/work/CommonLibSSE-NG   # VS 2026 (v145), x64-windows-static, /MT
cmake --build --preset release                                     # header checks + tests; or --preset debug
ctest --preset release                                             # run unit tests; or --preset debug
.\Format-Code.ps1            # clang-format src/ and tests/ in place
.\Format-Code.ps1 -Check     # report unformatted files, exit 1 on violations
```
The `default` preset enables the vcpkg manifest feature `tests` (Catch2). `SKSEHELPERS_TESTS` (default ON when top-level) adds the test targets; a top-level build without that feature fails at `find_package(Catch2)`.
Sources are globbed with `CONFIGURE_DEPENDS`; re-run configure if a new file is not picked up.

## Build Rules
- Warnings: `/Wall /WX`, `/external:anglebrackets /external:W0` on the `SKSEHelpersHeaderChecks` target so only this repo's files are diagnosed. Code MUST build with zero warnings. Never silence with pragmas or by dropping flags; fix the code.
- Because of `/external:anglebrackets`, include this repo's headers with **quotes** and third-party/STL/CommonLib with **angle brackets**. No `..` in include paths (C4464); resolve from `src/`, e.g. `"Hooks/Hooks.h"`.
- `SKSEHELPERS_HEADER_CHECKS` (default ON when top-level) adds the `SKSEHelpersHeaderChecks` OBJECT target that compiles every header in its own TU under these flags. Each header MUST be self-contained: include every STL/CommonLib header it uses; never rely on a consumer PCH or aliases such as `logger::` (use `SKSE::log::`).
- Templates are only diagnosed when instantiated; header checks don't cover template bodies. The test targets instantiate them under the same flags plus `/wd` for informational /Wall warnings that fire on any instantiation or come from the optimizer/Catch2 (padding, implicitly deleted special members, inlining, Spectre, C4868); the list and reasons are in `tests/CMakeLists.txt`. Do not extend it to hide real diagnostics.
- INTERFACE compile definitions `NOMINMAX` and `WIN32_LEAN_AND_MEAN` exist because `Hooks/Hooks.h` includes `MinHook.h` (→ `<Windows.h>`). CommonLib headers (`<RE/Skyrim.h>`, `<SKSE/SKSE.h>`) MUST be included before `MinHook.h`/`<Windows.h>`. Do not include `<REL/REL.h>` directly; it is not self-contained (get it via `<SKSE/SKSE.h>`).
- Header-only: no `.cpp` files under `src/`; keep it that way (use `inline`/function-local statics for out-of-line state). `.cpp` files exist only in `tests/`.

## Code Conventions
- C++23, MSVC only, Windows x64. SE/AE/VR multi-targeting comes from CommonLib (`REL::RelocationID`, `REL::VariantOffset`).
- Formatting: `.clang-format` (tabs, col 130, left pointers), `.editorconfig`. Run `Format-Code.ps1` on touched files.
- Files: UTF-8 **with BOM**, CRLF. Russian comments are normal; preserve encoding. When editing line 1 keep exactly one BOM.
- `#pragma once` in every header.
- `TESLookup`: pass the most specific `T`; `throwOnError=true` for static forms resolved once at `kDataLoaded`, `false` only for dynamic IDs and then null-check.
- Observables: `Subject`/`ReactiveObject` are non-movable (subscriptions store an owner pointer). Subscriptions are RAII; `std::move(sub).Detach()` keeps the observer alive intentionally. Observers may subscribe/unsubscribe (and re-enter `Notify`) from inside a notification: unsubscribed observers are skipped immediately and erased when the outermost `Notify` ends; new observers are queued and receive values from the next `Notify` on (`SubscribeAndNotify` still calls them at once).

## Verification
Before finishing a change:
1. `cmake --build --preset release` with zero warnings (header checks, compile tests, unit tests).
2. `ctest --preset release` passes. Behavior changes to CommonLib-free headers need a Catch2 test in `tests/runtime/` (one file per header area, deterministic: drive `TaskScheduler` with `std::uint64_t` timestamps and explicit `Tick`). Changes to `HookDefinition` concepts go in `tests/compile/`.
3. CommonLib-dependent templates not covered by `tests/compile/` (`Utilities.h`, `TESLookup.h`, `HookInstaller.h` instantiation): throwaway consumer project (outside the repo) that `add_subdirectory`s this repo with `-DCOMMONLIB_PATH=...`, builds a `SHARED` target with `/utf-8 /Wall /WX /permissive- /external:anglebrackets /external:W0`, includes the touched headers and instantiates the changed templates. Delete it afterwards.
4. `.\Format-Code.ps1 -Check`.
Runtime behavior of game-facing code (hooks, lookups, actor helpers) can only be verified in-game by a consumer plugin; list what to check.
