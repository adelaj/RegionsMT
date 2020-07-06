
# RegionsMT

[![Build Status](https://api.travis-ci.org/adelaj/RegionsMT.svg?branch=master)](https://travis-ci.org/adelaj/RegionsMT) [![Build status](https://ci.appveyor.com/api/projects/status/7rmx0ccfv1lq7ng6?svg=true)](https://ci.appveyor.com/project/adelaj/regionsmt)

## Installation for Linux
Build process is described only for the `x86_64` target architecture. The `i386` architecture is supported as well. To build `i386` executables, all the occurrences of `x86_64` in the subsequent text should be substituted by `i386`.

### Prerequisites
The following common packages are required for the installation: 
* `bash` (4.3 or newer, older versions will definitely not work);
* `cmake` (3.15 or newer, older versions will definitely not work);
* `git`;
* `make` (4.1 or newer, older versions will definitely not work).

User should make a decision about the toolchain to use. The following toolchains are available: `gcc`, custom versions of `gcc`, and `clang`. 

1. Packages required by the `gcc` toolchain:
   * `gcc` (7.4.0 or newer, or may be older);
   * `g++` (required only for `cmake` proper operation, not required for compiling);
   * `gcc-multilib` (only for cross compiling, e. g. building `i386` executables on  the `x86_64` machines).

2. Packages required by the custom version of the `gcc` toolchain, for instance the  `gcc-8` (`gcc` 8.3.0) is used and may be replaced by any suitable version:
   * `gcc-8`;
   * `g++-8`;
   * `gcc-8-multilib` (only for cross compiling).

3. Packages required by the `clang` toolchain:
   * `clang` (6.0 or newer);
   * `llvm` (only `llvm-ar` and `llvm-ranlib` executables are actually required);
   * `lld`.

### Build sequence
The command-line shell dialect which is used further is assumed to be `bash`. 
#### Directory structure
For definiteness it is assumed that all build files are stored into the `~/build` directory. Files built by `gcc`, `gcc-8` or `clang` toolchains will be stored into `~/build/gcc`, `~/build/gcc-8`, or `~/build/clang` directories respectively.
```bash
cd ~
mkdir build
```
#### Downloading sources
```bash
cd ~/build
git clone --depth 1 https://github.com/adelaj/RegionsMT
```
#### Building `RegionsMT` and all of the required dependencies
1. Building with the `gcc` toolchain:
   ```bash
   cd ~/build/RegionsMT
   make MATRIX="`echo gcc:x86_64:{Debug,Release}`" -j
   ```
   Resulting executables for the `Debug` and `Release` configurations are `~/build/gcc/RegionsMT/x86_64/Debug/RegionsMT` and `~/build/gcc/RegionsMT/x86_64/Release/RegionsMT` respectively.
    
2. Building with the `gcc-8` toolchain:
   ```bash
   cd ~/build/RegionsMT
   make MATRIX="`echo gcc-8:x86_64:{Debug,Release}`" -j
   ```
   Resulting executables for the `Debug` and `Release` configurations are `~/build/gcc-8/RegionsMT/x86_64/Debug/RegionsMT` and `~/build/gcc-8/RegionsMT/x86_64/Release/RegionsMT` respectively.
   
3. Building with the `clang` toolchain:
   ```bash
   cd ~/build/RegionsMT
   make MATRIX="`echo clang:x86_64:{Debug,Release}`" -j
   ```
   Resulting executables for the `Debug` and `Release` configurations are `~/build/clang/RegionsMT/x86_64/Debug/RegionsMT` and `~/build/clang/RegionsMT/x86_64/Release/RegionsMT` respectively.

## Installation for Windows (Visual Studio)
Build process is described only for the `x64` (Windows alias of the `x86_64`) target architecture.  The `Win32` (alias `i386`) architecture is supported as well. To build `Win32` executables all the occurrences of `x64` in the subsequent text should be substituted by `Win32`.

### Prerequisites
The following programs are required for the Windows installation:
* [Visual Studio 2019 Community](https://visualstudio.microsoft.com/vs/community/) with C++ support enabled;
* [git for Windows](https://git-scm.com/). Directory containing executables should be added to `PATH` during installation;
* [CMake](https://cmake.org/) (3.15 or newer version). Directory containing executables should be added to `PATH` during installation;
* [MSYS2](https://www.msys2.org/) (either `x86_64` or `i686` version) with `make` package (installation is listed below). For instance, it is assumed that `x86_64` of `MSYS2` is installed (by default) into `C:\msys64` folder. Do not add any subfolders of `C:\msys64` to `PATH` manually! 

### Build sequence
The command-line interpreter which is used further is assumed to be `powershell`.
#### Directory structure
The installation directory is assumed to be `~/build`, which in Windows terms should be treated as something like `C:\Users\User\build`.
```PowerShell
cd ~
New-Item -ItemType Directory -Path build
```
#### Downloading sources
```PowerShell
cd ~/build
git clone --depth 1 https://github.com/adelaj/RegionsMT
```
#### Upgrading `MSYS2` and installing `make`
```PowerShell
$env:MSYSTEM="MSYS2"
C:\msys64\usr\bin\bash.exe -lc 'pacman -Syu --noconfirm'
C:\msys64\usr\bin\bash.exe -lc 'pacman -Syu --noconfirm' # Should be run for the second time
C:\msys64\usr\bin\bash.exe -lc 'pacman -S --needed make'
```
#### Building `RegionsMT` and all of the required dependencies
```PowerShell
cd ~/build/RegionsMT
$env:MSYSTEM="MSYS2"
$env:CHERE_INVOKING="1"
$env:MSYS2_ARG_CONV_EXCL="*"
$env:GIT=C:\msys64\usr\bin\bash.exe -c '/usr/bin/which git'
$env:CMAKE=C:\msys64\usr\bin\bash.exe -c '/usr/bin/which cmake'
C:\msys64\usr\bin\bash.exe -lc 'make MATRIX=\"`echo msvc:x64:{Debug,Release}`\" -j'
```
Resulting executables for the `Debug` and `Release` configurations are `~/build/msvc/RegionsMT/x64/Debug/RegionsMT.exe` and `~/build/msvc/RegionsMT/x64/Release/RegionsMT.exe` respectively.

## Installation for Windows (WSL)
A different way of installation for Windows 10 can be performed on top of the [Windows Subsystem for Linux (WSL)](https://en.wikipedia.org/wiki/Windows_Subsystem_for_Linux). The build process is essentially identical to that of Linux, however `i386` executables cannot run, though still can be built.

### Installation via Cygwin or MinGW ---  TBD

## Installation for Mac OS (preliminary)

### Prerequisites
The following programs should be installed:
* A Mac OS package manager such as [Homebrew](https://brew.sh/) or [MacPorts](https://www.macports.org/);
* [Xcode](https://developer.apple.com/xcode/) with [Command Line Tools](https://stackoverflow.com/questions/9329243/xcode-install-command-line-tools).

The following packages are required for the installation: 
* `git`;
* `cmake` (3.15 or newer).
