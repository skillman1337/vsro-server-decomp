# vsro-server-decomp

Work-in-progress C++ reconstruction of Silkroad Online's SR_GameServer, with native behavior research, regression tests, and evidence notes.

This repository contains the game-server reconstruction and supporting libraries. It is incomplete and is not a production-ready server distribution. A successful build or focused regression test does not establish full native behavior, original ABI compatibility, or a working multiplayer server.

The existing research notes identify a v1.188 research executable with SHA-256 `bec2375e2c4c1073e3bf7761571470c430de251de74b452dbb86537348ef5290`. They do not establish equivalence to the v1.150 client. Original executables, runtime data, and local analysis databases are not included.

## Build

Use Windows, Python 3, and MinGW-w64 `g++` with C++20 support. Put the compiler's `bin` directory on `PATH`, then run from the repository root:

```powershell
python build_full.py
```

Output: `build/SR_GameServer.exe`. The build links Windows networking, UI, and ODBC libraries. See [build and test instructions](docs/BUILDING.md) for prerequisites and test limitations.

## Repository layout

| Path | Contents |
| --- | --- |
| `SR_GameServer/` | Game objects, skills, world, AI, inventory, and server logic |
| `ServerCommon/` | Shared reference data and database record representations |
| `JMX_ServerFramework/` | Server application, configuration, and window framework |
| `JMX_Library/` | Networking, database, navigation, memory, and utility code |
| `Common/` | Shared framework helpers |
| `tests/` | Focused C++ regressions and Python test runners |
| `tools/` | Research data extraction utilities |
| `docs/` | Build guidance, audits, and retained research evidence |

## Documentation

- [Build and test instructions](docs/BUILDING.md)
- [Visibility audit and known gaps](docs/VISIBILITY-AUDIT.md)
- [Parameter and effect retirement audit](docs/PARAMETER-AUDIT.md)
- [Evidence notes](docs/evidence/README.md)
- [Contribution guidance](CONTRIBUTING.md)
- [Initial GitHub upload](docs/PUBLISHING.md)

Audit documents are dated research records. Later code may supersede individual findings; consult the implementation and reproduce the relevant checks before treating a historical result as current verification.

## Configuration

`server.example.cfg` is a starting point using loopback addresses. Copy it to `server.cfg` for local experiments. Local configuration, build products, logs, crash dumps, and analysis databases are ignored by Git. Startup still depends on incomplete subsystems and external data; the sample is not a complete deployment configuration.

## License

No project license has been selected. This repository does not grant a license to third-party game binaries or assets.
