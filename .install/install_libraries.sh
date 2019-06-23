#!/bin/bash

if [ -z "$ARCH" ]; then ARCH=$(arch); fi
if [ -z "$CFG" ]; then CFG=Release; fi
if [ -z "$CC" ]; then CC="gcc"; fi
if [ -z "$CXX" ]; then CXX="g++"; fi
if [ -z "$AR" ]; then AR="/usr/bin/gcc-ar"; fi
if [ "${XCODE,,}" == "true" ]; then XCODE="true"; else XCODE="false"; fi
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
    cmake --build . --target $1 --parallel -- VERBOSE=1
}

build_lib_xcode() {
    C_FLAGS_INIT=$(echo $C_FLAGS $(eval echo "\${C_FLAGS_$2} \${C_FLAGS__$3} \${C_FLAGS_$2_$3}") | xargs)
    cmake -G "Xcode" -D CMAKE_OSX_ARCHITECTURES="$2" -D CMAKE_C_FLAGS_INIT="$C_FLAGS_INIT" -D CMAKE_REQUIRED_LIBRARIES="m" "../../$SRC/$1"
    for i in $CFG; do
        cmake --build . --target $1 --config $i --parallel
    done
}

for i in $LIB; do
    SDIR=$(eval dirname $0)/$i
    if [ -f "$SDIR/prologue.sh" ]; then SRC=$SRC CLR=$CLR bash $SDIR/prologue.sh; fi 
    for j in $ARCH; do
        DIR=$i-$j
        mkdir -p $DIR
        cd $DIR
        if [ $XCODE == "true" ]; then
            build_lib_xcode $i $j
        else
            for k in $CFG; do
                mkdir -p $k
                cd $k
                build_lib $i $j $k
                cd ..
            done
        fi
        cd ..
    done
    if [ -f "$SDIR/epilogue.sh" ]; then SRC=$SRC CLR=$CLR bash $SDIR/epilogue.sh; fi
done
fi