# Building and testing

## Requirements

- Windows and a MinGW-w64 GCC toolchain supporting C++20.
- Python 3; the build and test runners use the standard library.
- Git for source control.

The current scripts use GCC flags and Windows libraries (`ws2_32`, `iphlpapi`, `gdi32`, `comctl32`, and `odbc32`). MSVC and Linux builds are not configured by these scripts.

## Full build

From the repository root, with `g++` on `PATH`:

```powershell
python --version
g++ --version
python build_full.py
```

The script compiles the `.cpp` sources with up to 16 workers, writes objects under `build/obj/`, and links `build/SR_GameServer.exe`. It does not run the server or tests.

## Visibility regression

```powershell
python tests/run-visibility.py
```

This runner currently expects the compiler at `D:/msys64/mingw64/bin/g++.exe`. If installed elsewhere, adjust its `cxx` assignment first. The same hardcoded path appears in the two other Python test runners.

The visibility runner rebuilds the server sources with four workers, links `build/visibility-audit/server.exe`, then links and runs the visibility assertions. Its objects are prerequisites for the lifecycle and linked-teardown runners.

## Tests requiring external evidence

The complete fixture collection is not in this repository. Some tests depend on the sibling research tree formerly located at `../../rebuild/apps/server/`.

- `tests/run-linked-teardown.py <fixture>` consumes the native linked-teardown fixture after the visibility build.
- `tests/run-skill-lifecycle.py <timed-job-fixture> <periodic-jobs-fixture> [parameter-retirement-fixture]` consumes the visibility objects. It also uses hardcoded sibling-tree paths for resource offsets, deferred instructions, berserk points, and prepared costs.
- `tests/retirement-selector.cc` requires the native retirement-selector fixture described in the parameter audit.

These runners are not a self-contained fresh-clone test suite. Preserve fixture provenance and expected binary hashes when supplying external evidence. See [PARAMETER-AUDIT.md](PARAMETER-AUDIT.md) for the historical fixture locations and results.

For a standalone parameter regression:

```powershell
New-Item -ItemType Directory -Force build | Out-Null
g++ -std=c++20 -I. tests/parameter-modifiers.cc SR_GameServer/GParamKeeper.cpp -o build/parameter-modifiers-test.exe
if ($LASTEXITCODE -eq 0) { .\build\parameter-modifiers-test.exe }
```

## Generated definitions

`SR_GameServer/ParameterDefinitions.inc` is intentionally versioned. Regeneration requires the exact hash-checked research executable:

```powershell
python tools/extract-parameter-definitions.py PATH_TO_RESEARCH_EXE SR_GameServer/ParameterDefinitions.inc
```

The binary is not needed simply to compile the checked-in definitions.
