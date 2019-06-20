#!/bin/bash

# Run samples: 
#   ARCH="i386 x86_64" CFG="Debug Release" SRC="." CLR="False" CC="gcc" CXX="g++" AR="gcc-ar" bash RegionsMT/.install/install.sh
#   ARCH="i386" CFG="Debug" SRC="../lib-src" CLR="False" CC="gcc-8" AR="/usr/bin/gcc-ar-8" CXX="g++-8" bash ../RegionsMT/.install/install.sh
#   ARCH="i386 x86_64" CFG="Debug Release" SRC="../lib-src" CLR="False" CC="gcc-8" AR="/usr/bin/gcc-ar-8" CXX="g++-8" bash ../RegionsMT/.install/install.sh
#   ARCH="x86_64" CFG="Debug" SRC="../lib-src" CLR="False" CC="clang" AR="/usr/bin/llvm-ar" CXX="clang++" bash ../RegionsMT/.install/install.sh

if [ -z "$ARCH" ]; then ARCH=$(arch); fi
if [ -z "$CFG" ]; then CFG=Release; fi
if [ -z "$CC" ]; then CC="gcc"; fi
if [ -z "$CXX" ]; then CXX="g++"; fi
if [ -z "$AR" ]; then AR="/usr/bin/gcc-ar"; fi
if [ "${CLR,,}" == "true" ]; then CLR="true"; else CLR="false"; fi

LIB=gsl

C_FLAGS="-mavx"
C_FLAGS_i386="-m32"
C_FLAGS_x86_64="-m64"
C_FLAGS__Release="-flto"

if [[ "$CC" == gcc* ]]; then 
    C_FLAGS__Release+=" -fuse-linker-plugin"
    C_FLAGS__Debug+=" -Og"
elif [[ "$CC" == clang* ]]; then
    C_FLAGS__Debug+=" -O0"
fi

build_lib() {
    C_FLAGS_INIT=$(echo $C_FLAGS $(eval echo "\${C_FLAGS_$2} \${C_FLAGS__$3} \${C_FLAGS_$2_$3}") | xargs)
    cmake -D CMAKE_C_COMPILER="$CC" -D CMAKE_CXX_COMPILER="$CXX" -D CMAKE_AR="$AR" -D CMAKE_BUILD_TYPE="$3" -D CMAKE_C_FLAGS_INIT="$C_FLAGS_INIT" -D CMAKE_C_ARCHIVE_CREATE="<CMAKE_AR> qcs <TARGET> <OBJECTS>" -D CMAKE_C_ARCHIVE_FINISH=true -D CMAKE_REQUIRED_LIBRARIES="m" "../../$SRC/$1"
    cmake --build . --target $1 -- VERBOSE=1 -j
}

for i in $LIB; do
    SDIR=$(eval dirname $0)/$i
    if [ -f "$SDIR/prologue.sh" ]; then SRC=$SRC CLR=$CLR bash $SDIR/prologue.sh; fi 
    for j in $ARCH; do
        DIR=$i-$j
        mkdir -p $DIR
        cd $DIR
        for k in $CFG; do
            mkdir -p $k
            cd $k
            build_lib $i $j $k
            cd ..
        done
        cd ..
    done
    if [ -f "$SDIR/epilogue.sh" ]; then SRC=$SRC CLR=$CLR bash $SDIR/epilogue.sh; fi
done
