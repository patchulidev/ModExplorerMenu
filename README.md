![](https://capsule-render.vercel.app/api?type=waving&height=300&color=gradient&text=Modex&desc=A%20Mod%20Explorer%20Menu&descSize=20&section=header)

![GitHub last commit](https://img.shields.io/github/last-commit/patchulidev/modexplorermenu?style=for-the-badge) ![GitHub License](https://img.shields.io/github/license/patchulidev/modexplorermenu?style=for-the-badge) ![GitHub Issues or Pull Requests](https://img.shields.io/github/issues/patchulidev/modexplorermenu?style=for-the-badge) ![GitHub Release](https://img.shields.io/github/v/release/patchulidev/modexplorermenu?include_prereleases&display_name=release&style=for-the-badge) ![Static Badge](https://img.shields.io/badge/nexus-page-gray?style=for-the-badge&labelColor=orange&link=https%3A%2F%2Fwww.nexusmods.com%2Fskyrimspecialedition%2Fmods%2F137877)

This is a CommonlibSSE-NG Plugin for Skyrim SE/AE Game versions 1.5.97 - 1.6.1170. This project utilizes xmake, following the outline of the template [commonlibsse-ng-template](https://github.com/libxse/commonlibsse-ng-template/tree/main). Build instructions can be found below.

![Static Badge](https://img.shields.io/badge/Skyrim-1.5.97+-gray?style=for-the-badge&labelColor=blue) ![Static Badge](https://img.shields.io/badge/Skyrim-1.6.1170-gray?style=for-the-badge&labelColor=blue)


### Requirements
* [XMake](https://xmake.io) [2.8.2+]
* C++23 Compiler (MSVC, Clang-CL)

### Dependencies (Managed)
* [imgui](https://github.com/ocornut/imgui) [v1.91.5]
* [freetype](https://github.com/freetype/freetype) [Latest]
* [fmt](https://github.com/fmtlib/fmt) [Latest]
* [nlohmann-json](https://github.com/nlohmann/json) [v.3.12.0]
* [simpleini](https://github.com/brofield/simpleini) [Latest]
* [commonlibsse-ng](https://github.com/alandtse/CommonLibVR/) [Latest]

### Information

This project is natively maintained and built on Windows 11 using Visual Studio Code 2026. Mileage may vary.

This project is setup for a local download of Commonlib in the project folder. Will require reconfiguration if you have a global instance of it.

P.S. You may have include path issues with my xmake configuration - sorry.

## Getting Started
```bat
git clone --recurse-submodules https://github.com/patchulidev/ModExplorerMenu
cd commonlibsse-ng-template
```

### Build
To build the project, run the following command:
```bat
xmake build
```

> ***Note:*** *This will generate a `build/windows/` directory in the **project's root directory** with the build output.*
> ***Note:*** *Project packages are installed locally in the .xmake directory in your workspace folder. This can be turned off*

### Build Output (Optional) (Untested)
If you want to redirect the build output, set one of or both of the following environment variables:

- Path to a Skyrim install folder: `XSE_TES5_GAME_PATH`

- Path to a Mod Manager mods folder: `XSE_TES5_MODS_PATH`

### Project Generation (Optional)
If you want to generate a Visual Studio project, run the following command:
```bat
xmake project -k vsxmake
```

> ***Note:*** *This will generate a `vsxmakeXXXX/` directory in the **project's root directory** using the latest version of Visual Studio installed on the system.*

### Upgrading Packages (Optional)
If you want to upgrade/modify the project's dependencies, run the following commands:
```bat
xmake repo --update
xmake require --upgrade
```

### Clean and Reconfigure (Optional)
Similarly to CMake, you may need to clean and reconfigure your installation.
```bat
xmake f -c
xmake build
```

## Documentation
Please refer to the [Wiki](https://github.com/libxse/commonlibsse-ng-template/wiki) for more advanced topics and template guidance.
