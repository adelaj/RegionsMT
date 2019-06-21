
# RegionsMT

## Installation for Linux
Build process is described only for the `x86_64` target architecture. The `i386` architecture is also supported. To build `i386` executables, all the occurrences of `x86_64` in the subsequent text should be substituted by `i386`.

### Prerequisites
The following common packages are required for the installation: 
* `git`;
* `cmake` (3.10.2 or newer);
* `make` (4.1 or newer, or may be older, but 3.8.1 will definitely not work).

User should make a decision about the toolchain to use. Prerequisites for following toolchains are available: `gcc`, custom versions of `gcc`, and `clang`. 

1. Packages required by the `gcc` toolchain:
   * `gcc` (7.4.0 or newer, or may be older);
   * `g++` (required only for `cmake` proper operation, not required for compiling);
   * `gcc-multilib` (only for cross compiling, e. g. building `i386` executables on  the `x86_64` machines).

2. Packages required by the custom version of the `gcc` toolchain, `gcc-8` (`gcc` 8.3.0) is used as an example and may be replaced by any suitable version:
   * `gcc-8`;
   * `g++-8`;
   * `gcc-8-multilib` (only for cross compiling).

3. Packages required by the `clang` toolchain:
   * `clang` (6.0 or newer, or may be older);
   * `llvm` (only `llvm-ar` executable is actually required).

### Build sequence
The default shell is assumed to be `bash`. 
#### Directory structure
For definiteness it is assumed that all build files are stored into the `~/build` directory. Files built by `gcc` or `clang` toolchains are stored into `~/build/gcc` or `~/build/clang` directories respectively.
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
git clone https://github.com/adelaj/RegionsMT
```
#### Building dependencies
1. Building with the `gcc` toolchain:
   ```bash
   cd ~/build/gcc
   ARCH="x86_64" \
   CFG="Debug Release" \
   SRC=".." \
   bash ../RegionsMT/.install/install.sh
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
   bash ../RegionsMT/.install/install.sh
   ```
1. Building with the `clang` toolchain (as above, path to `llvm-ar` should be absolute):
   ```bash
   cd ~/build/clang
   ARCH="x86_64" \
   CFG="Debug Release" \
   SRC=".." \
   CLR="False" \
   CC="clang" \
   AR="/usr/bin/llvm-ar" \
   CXX="clang++" \
   bash ../RegionsMT/.install/install.sh
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
   Resulting executables for the `Debug` and `Release` configurations will be `~/build/gcc/RegionsMT-x86_64/Debug/RegionsMT` and `~/build/gcc/RegionsMT-x86_64/Release/RegionsMT` respectively.
    
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
   Resulting executables for the `Debug` and `Release` configurations will be `~/build/gcc-8/RegionsMT-x86_64/Debug/RegionsMT` and `~/build/gcc-8/RegionsMT-x86_64/Release/RegionsMT` respectively.
   
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
   Resulting executables for the `Debug` and `Release` configurations will be `~/build/clang/RegionsMT-x86_64/Debug/RegionsMT` and `~/build/clang/RegionsMT-x86_64/Release/RegionsMT` respectively.

## Installation for Windows
Build process is described only for the `x64` (Windows alias of the `x86_64`) target architecture.  The `Win32` (alias `i386`) architecture is also supported. To build `Win32` executables all the occurrences of `x64` in the subsequent text should be substituted by `Win32`.

TBD

### Installation via WSL
A different way of installation for Windows 10 can be performed on top of the [Windows Subsystem for Linux](https://en.wikipedia.org/wiki/Windows_Subsystem_for_Linux). The build process is identical to that of Linux, however `i386` executables cannot be run, though still can be compiled.

## Installation on Mac OS
TBD
