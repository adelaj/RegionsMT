#!/bin/bash

if [ -z "$ARCH" ]; then ARCH=$(arch); fi
if [ -z "$CFG" ]; then CFG=Release; fi
if [ -z "$BUILD_PATH" ]; then BUILD_PATH="."; fi
if [ -z "$LOG_PATH" ]; then LOG_PATH=$BUILD_PATH/.`basename $0 .${0##*.}`_`date +%s`; fi
if [ -z "$TOOLCHAIN" ]; then TOOLCHAIN="gcc"; fi
if [ -z "$LIBRARY" ]; then LIBRARY="gsl"; fi

declare -A \
CC=([gcc]="gcc" [clang]="clang") \
CXX=([gcc]="g++" [clang]="clang++") \
AR=([gcc]="gcc-ar" [clang]="llvm-ar") \
C_FLAGS=( \
[,,]="-mavx" \
[,i386,]="-m32" \
[,x86_64,]="-m64" \
[,,Release]="-flto" \
[gcc,,Release]="-fuse-linker-plugin" \
[gcc,,Debug]="-Og" \
[clang,,Debug]="-O0")

crash() {
    cat $2
    return $1
}

wait_pid() {
    declare -i res=0 &&
    declare -n pidref=$1
    for p in ${!pidref[@]}; do
        wait $p || {
            declare out=$? &&
            let "res=-1" && 
            echo "${pidref[$p]}: "`tput setaf 9`"*** Error $out"`tput sgr0` 
        }
    done &&
    return $res
}

build_vars_2() {
    for v in $1; do
        declare -g -A BUILD_$v[$2,$3,]="$(echo $(eval echo \
        \${$v'[,,]'} \${$v'[$2,,]'} \${$v'[,$3,]'} \${$v'[$2,$3,]'}) | xargs)"
        # echo BUILD_$v[$2,$3,] = $(eval echo \${BUILD_$v'[$t,$i,]'})
    done
}

build_vars_3() {
    for v in $1; do
        declare -g -A BUILD_$v[$2,$3,$4]="$(echo $(eval echo \
        \${BUILD_$v'[$2,$3,]'} \${$v'[,,$4]'} \${$v'[$2,,$4]'} \${$v'[,$3,$4]'} \${$v'[$2,$3,$4]'}) | xargs)"
        # echo BUILD_$v[$2,$3,$4] = $(eval echo \${BUILD_$v'[$2,$3,$4]'})
    done
}

toolset_vars() {
    for v in $1; do
        if [ -z "$(eval echo \${$v[$2]})" ]; then 
            declare -g -A BUILD_$v[$2]="$(eval which \${$v'[$3]'}$4)"
        else
            declare -g -A BUILD_$v[$2]="$(eval which \${$v'[$2]'})"
        fi
    done
}

build_library_cmake_cfg() {
    declare log="$LOG_PATH/$2/$1-$3/$4.log" &&
    mkdir -p "${log%/*}" &&
    cmake \
    -G "Unix Makefiles" \
    -B "$2/$1-$3/$4" \
    -D CMAKE_C_COMPILER="${BUILD_CC[$2]}" \
    -D CMAKE_CXX_COMPILER="${BUILD_CXX[$2]}" \
    -D CMAKE_AR="${BUILD_AR[$2]}" \
    -D CMAKE_BUILD_TYPE="$4" \
    -D CMAKE_C_FLAGS_INIT="${BUILD_C_FLAGS[$2,$3,$4]}" \
    -D CMAKE_C_ARCHIVE_CREATE="<CMAKE_AR> qcs <TARGET> <OBJECTS>" \
    -D CMAKE_C_ARCHIVE_FINISH="" \
    "$1" \
    &>$log &&
    cmake --build "$2/$1-$3/$4" --target $1 -- VERBOSE=1 -j \
    &>>$log ||
    crash $? $log
}

build_library_cmake() {
    declare -a pid &&
    for k in $CFG; do
        build_library_cmake_cfg $1 $2 $3 $k &
        pid[$!]="build_library_cmake_cfg $1 $2 $3 $k"
    done &&
    wait_pid pid 
}

build_library_xcode() {
    # mkdir -p "Xcode/$1-$2"
    cmake \
    -B "Xcode/$1-$2"
    -G "Xcode" \
    -D CMAKE_OSX_ARCHITECTURES="$2" \
    -D CMAKE_C_FLAGS_INIT="$(eval echo \${BUILD_C_FLAGS[Xcode,$2,$3]})" \
    "$1" &&
    declare -a pid &&
    for i in $CFG; do
        {
            mkdir -p "$LOG_PATH/$2/$1-$3" && 
            cmake --build "Xcode/$1-$2" --target $1 --config $i -- -j &>"$LOG_PATH/Xcode/$1-$3/$k.log" ||
            crash $? "$LOG_PATH/Xcode/$1-$3/$k.log"
        } &
        pid[$!]="cmake --build "Xcode/$1-$2" --target $1 --config $i -- -j"
    done &&
    wait_pid pid
}

download_library_git() {
    declare log="$LOG_PATH/$1.log"
    mkdir -p "$LOG_PATH" &&
    if [ -d "$1" ]; then
        git -C "$1" reset --hard HEAD &>$log &&
        git -C "$1" pull &>>$log
    else
        git clone --depth 1 "$2" "$1" &>$log
    fi ||
    crash $? $log
}

download_gsl() {
    download_library_git gsl "https://github.com/ampl/gsl"
}

build_gsl() {
    if [ "$1" == "Xcode" ]; then
        build_library_xcode gsl $2
    else
        build_library_cmake gsl $1 $2
    fi
}

install_library() {
    download_$1 &&
    declare -a pid &&
    for t in $TOOLCHAIN; do
        for i in $ARCH; do
            build_$1 $t $i &
            pid[$!]="build_$1 $t $i"
        done
    done &&
    wait_pid pid
}

install_libraries() {
    for t in $TOOLCHAIN; do
        if [ "$t" != "Xcode" ]; then
            toolset_vars "CC CXX AR" $t ${t%%-*} ${t#${t%%-*}*}
        fi
        for i in $ARCH; do
            build_vars_2 "C_FLAGS" $t $i
            for j in $CFG; do 
                build_vars_3 "C_FLAGS" $t $i $j
            done
        done
    done
    declare -a pid &&
    for i in $LIBRARY; do
        install_library $i &
        pid[$!]="install_library $i"
    done &&
    wait_pid pid
}

uninstall_libraries() {
    for t in $TOOLCHAIN; do
        for i in $ARCH; do
        done     
    done    
}

if [ "$1" == "clean" ]; then
    clean_libraries
else
    install_libraries
fi