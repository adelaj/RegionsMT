#!/bin/bash

# Run Sample: 
#   ARCH="i386 x86_64" CFG="Debug Release" SRC="." CLR="False" bash install.sh

if [ -z "$ARCH" ]; then ARCH=$(arch); fi
if [ -z "$CFG" ]; then CFG=Release; fi
if [ "${CLR,,}" == "true" ]; then CLR="true"; else CLR="false"; fi
LIB=gsl
C_FLAGS="-flto -fuse-linker-plugin -mavx"
C_FLAGS_i386="-m32"
C_FLAGS_x86_64="-m64"
build_lib() {
    C_FLAGS_INIT=$(echo $C_FLAGS $(eval echo "\$C_FLAGS_$2") | xargs)
    cmake -D CMAKE_BUILD_TYPE="$3" -D CMAKE_C_FLAGS_INIT="$C_FLAGS_INIT" -D CMAKE_AR="/usr/bin/gcc-ar" -D CMAKE_C_ARCHIVE_CREATE="<CMAKE_AR> qcs <TARGET> <OBJECTS>" -D CMAKE_C_ARCHIVE_FINISH=true -D CMAKE_REQUIRED_LIBRARIES="m" "../../$SRC/$1"
    cmake --build . --target $1 -- VERBOSE=1
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
