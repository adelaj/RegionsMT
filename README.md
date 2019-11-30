
# RegionsMT

[![Build Status](https://api.travis-ci.org/adelaj/RegionsMT.svg?branch=master)](https://travis-ci.org/adelaj/RegionsMT) [![Build status](https://ci.appveyor.com/api/projects/status/7rmx0ccfv1lq7ng6?svg=true)](https://ci.appveyor.com/project/adelaj/regionsmt)

## Installation for Linux
Build process is described only for the `x86_64` target architecture. The `i386` architecture is supported as well. To build `i386` executables, all the occurrences of `x86_64` in the subsequent text should be substituted by `i386`.

### Prerequisites
The following common packages are required for the installation: 
* `bash` (4.3 or newer, older versions will definitely not work);
* `cmake` (3.14 or newer, older versions will definitely not work);
* `git`;
* `make` (4.1 or newer, or may be older, but 3.8.1 will definitely not work).

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
   * `clang` (6.0 or newer, or may be older);
   * `llvm` (only `llvm-ar` executable is actually required);
   * `llvm-dev` (`LLVMgold.so` and `libLTO.so` shared libraries are required);
   * `binutils` (2.30, only `ld.gold` executable with plugin support is required).

### Build sequence
The command-line shell dialect which is used further is assumed to be `bash`. 
#### Directory structure
For definiteness it is assumed that all build files are stored into the `~/build` directory. Files built by `gcc`, `gcc-8` or `clang` toolchains are stored into `~/build/gcc`, `~/build/gcc-8`, or `~/build/clang` directories respectively.
```bash
cd ~
mkdir build
cd build
mkdir gcc # Mandatory only if gcc toolchain is prefered, optional otherwise
mkdir gcc-8 # Mandatory only if gcc-8 toolchain is prefered, optional otherwise
mkdir clang # Mandatory only if clang toolchain is prefered, optional otherwise
```
#### Downloading sources
```bash
cd ~/build
git clone --depth 1 https://github.com/adelaj/RegionsMT
```
#### Building dependencies
1. Building with the `gcc` toolchain:
   ```bash
   cd ~/build/gcc
   ARCH="x86_64" \
   CFG="Debug Release" \
   SRC=".." \
   bash ../RegionsMT/.install/install_libraries.sh
   ```
1. Building with the `gcc-8` toolchain (note that path to `gcc-ar-8` should be absolute, relative path to the librarian tool will be treated incorrectly by `cmake`):
   ```bash
   cd ~/build/gcc-8
   # Note, that path to gcc-ar-8 should be absolute. 
   # Relative path to the librarian tool will be treated incorrectly by cmake
   ARCH="x86_64" \
   CFG="Debug Release" \
   SRC=".." \
   CC="gcc-8" \
   AR="/usr/bin/gcc-ar-8" \
   CXX="g++-8" \
   bash ../RegionsMT/.install/install_libraries.sh
   ```
1. Building with the `clang` toolchain (as above, path to `llvm-ar` should be absolute):
   ```bash
   cd ~/build/clang
   ARCH="x86_64" \
   CFG="Debug Release" \
   SRC=".." \
   CC="clang" \
   AR="/usr/bin/llvm-ar" \
   CXX="clang++" \
   bash ../RegionsMT/.install/install_libraries.sh
   ```
#### Building `RegionsMT` 
1. Building with the `gcc` toolchain:
   ```bash
   cd ~/build/RegionsMT
   CFG="Debug Release" \
   ARCH="x86_64" \
   LIBRARY_PATH="../gcc" \
   INSTALL_PATH="../gcc" \
   make -j
   ```
   Resulting executables for the `Debug` and `Release` configurations are `~/build/gcc/RegionsMT-x86_64/Debug/RegionsMT` and `~/build/gcc/RegionsMT-x86_64/Release/RegionsMT` respectively.
    
2. Building with the `gcc-8` toolchain:
   ```bash
   cd ~/build/RegionsMT
   CFG="Debug Release" \
   ARCH="x86_64" \
   LIBRARY_PATH="../gcc-8" \
   INSTALL_PATH="../gcc-8" \
   CC="gcc-8" \
   make -e -j
   ```
   Resulting executables for the `Debug` and `Release` configurations are `~/build/gcc-8/RegionsMT-x86_64/Debug/RegionsMT` and `~/build/gcc-8/RegionsMT-x86_64/Release/RegionsMT` respectively.
   
3. Building with the `clang` toolchain:
   ```bash
   cd ~/build/RegionsMT
   CFG="Debug Release" \
   ARCH="x86_64" \
   LIBRARY_PATH="../clang" \
   INSTALL_PATH="../clang" \
   CC="clang" \
   make -e -j
   ```
   Resulting executables for the `Debug` and `Release` configurations are `~/build/clang/RegionsMT-x86_64/Debug/RegionsMT` and `~/build/clang/RegionsMT-x86_64/Release/RegionsMT` respectively.

## Installation for Windows
Build process is described only for the `x64` (Windows alias of the `x86_64`) target architecture.  The `Win32` (alias `i386`) architecture is supported as well. To build `Win32` executables all the occurrences of `x64` in the subsequent text should be substituted by `Win32`.

### Prerequisites
The following programs are required for the Windows installation:
* [Visual Studio 2019 Community](https://visualstudio.microsoft.com/vs/community/) with C++ support enabled;
* [git for Windows](https://git-scm.com/);
* [CMake](https://cmake.org/) (3.14 or newer version).

### Build sequence
The command-line interpreted which is used further is assumed to be `PowerShell`. 

#### Directory structure
The installation directory is assumed to be `~/build`, which in Windows terms should be treated as something like `C:\Users\User\build`.
```PowerShell
cd ~
New-Item -ItemType Directory -Path build
cd build
New-Item -ItemType Directory -Path msvc
```
#### Downloading sources
```PowerShell
cd ~/build
git clone --depth 1 https://github.com/adelaj/RegionsMT
```
#### Building dependencies
Note, that current installation process builds `pthread-win32` library regardless of its usage by the final project. 
```PowerShell
cd ~/build/msvc
../RegionsMT/.install/install_libraries.ps1 `
-ARCH "x64" `
-CFG "Debug Release" `
-SRC ".."
```
#### Building `RegionsMT` 
```PowerShell
cd ~/build/msvc
New-Item -ItemType Directory -Path RegionsMT-x64
cd RegionsMT-x64
cmake `
-D CMAKE_GENERATOR_PLATFORM="x64" `
-D CMAKE_LIBRARY_PATH="../msvc" `
-D FORCE_POSIX_THREADS="ON" `
../../RegionsMT
cmake --build . --config "Debug"
cmake --build . --config "Release"
```
Resulting executables for the `Debug` and `Release` configurations are `~/build/msvc/RegionsMT-x64/Debug/RegionsMT.exe` and `~/build/msvc/RegionsMT-x64/Release/RegionsMT.exe` respectively.

### Installation via WSL
A different way of installation for Windows 10 can be performed on top of the [Windows Subsystem for Linux (WSL)](https://en.wikipedia.org/wiki/Windows_Subsystem_for_Linux). The build process is essentially identical to that of Linux, however `i386` executables are not supported, though still can be built.


### Installation via Cygwin or MinGW ---  TBD

## Installation for Mac OS (preliminary)

### Prerequisites
The following programs should be installed:
* A Mac OS package manager such as [Homebrew](https://brew.sh/) or [MacPorts](https://www.macports.org/);
* [Xcode](https://developer.apple.com/xcode/) with [Command Line Tools](https://stackoverflow.com/questions/9329243/xcode-install-command-line-tools).

The following packages are required for the installation: 
* `git`;
* `cmake` (3.14 or newer).
