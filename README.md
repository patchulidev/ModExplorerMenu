# Modex - A Mod Explorer Menu

This is a CommonlibSSE-NG Plugin for Skyrim SE/AE Game versions 1.5.97 - 1.6.1170. This project utilizes xmake, following the outline of the template [commonlibsse-ng-template](https://github.com/libxse/commonlibsse-ng-template/tree/main). Build instructions can be found below.

### Requirements
* [XMake](https://xmake.io) [2.8.2+]
* C++23 Compiler (MSVC, Clang-CL)

### Dependencies (Managed)
* [imgui](https://github.com/ocornut/imgui) [v1.91.5]
* [freetype](https://github.com/freetype/freetype) [Latest]
* [fmt}](https://github.com/fmtlib/fmt) [Latest]
* [nlohmann-json](https://github.com/nlohmann/json) [v.3.12.0]
* [simpleini](https://github.com/brofield/simpleini) [Latest]
* [commonlibsse-ng](https://github.com/alandtse/CommonLibVR/) [Latest]

### Information

This project is natively maintained and built on Windows 11 using Visual Studio Code. Mileage may vary.

This project is setup for a local download of Commonlib in the project folder. Will require reconfiguration if you have a global instance of it.

P.S. You may have include path issues with my xmake configuration - sorry.

## Getting Started
```bat
git clone --recurse-submodules https://github.com/Patchu1i/ModExplorerMenu
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
