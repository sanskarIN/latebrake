# Third-party notices

The repository's authored source files and documents are distributed under [BSD-3-Clause](LICENSE), except where specifically noted.

## Unreal Engine 5.8 (required external tool/runtime)

Unreal Engine and built-in engine resources are products of Epic Games and are governed by **Epic's license terms**, separate from Latebrake's BSD license. The project refers to existing `/Engine/BasicShapes/Cube` and `/Engine/BasicShapes/Cylinder` content at runtime, and uses licensed Unreal module/plugin APIs such as Chaos. These engine assets, binaries, SDKs and source files are **not redistributed** in this Git repository.

Obtain and use Unreal Engine through authorized channels and comply with Epic's licensing and distribution terms when shipping games.

## Standard development tools

Portable tests use CMake and a C++20 compiler installed by contributors or CI; these tools are not vendored. GitHub Actions workflows refer to the official `actions/checkout` action; refer to its upstream license and terms.

## New dependencies

Before adding third-party code, assets, plugins or runtime services, update this file and [ASSET_LICENSES.md](ASSET_LICENSES.md) with provider, version, origin, license and required notices. Do not assume a dependency is acceptable simply because it is free to download.
