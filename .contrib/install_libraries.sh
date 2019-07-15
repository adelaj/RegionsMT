#!/bin/bash

declare -A \
VALID_LIBRARY=([gsl]="https://github.com/ampl/gsl") \
VALID_ARCH=([x86_64]="x86_64" [i386]="i386") \
VALID_CONFIG=([Release]="Release" [Debug]="Debug")

if [ -z "$ARCH" ]; then ARCH=${VALID_ARCH[`arch`]}; fi
if [ -z "$CFG" ]; then CFG=Release; fi
if [ -z "$BUILD_PATH" ]; then BUILD_PATH="."; fi
if [ -z "$LOG_PATH" ]; then LOG_PATH=$BUILD_PATH/.`basename $0 .${0##*.}`_`date +%s`; fi
if [ -z "$TOOLCHAIN" ]; then TOOLCHAIN="gcc"; fi
if [ -z "$LIBRARY" ]; then LIBRARY=${!VALID_LIBRARY[@]}; fi

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

unique() {
    echo "$1" | xargs -n1 | awk '!x[$0]++' | xargs
}

validate_var() {
    for i in $1; do
        declare tmp &&
        declare -n ref=VALID_$i &&
        for j in ${!1}; do
            if [ ! -z ${ref[$j]} ]; then
                tmp="$tmp $j"
            fi
        done &&
        declare -g $i=`unique "$tmp"`
    done
}

crash() {
    cat $2
    return $1
}

wait_pid() {
    declare -i res=0 &&
    declare -n pidref=$1 &&
    for p in ${!pidref[@]}; do
        wait $p || {
            declare out=$? &&
            let "res=-1" && 
            echo "${pidref[$p]}: "`tput setaf 9`"*** Error $out"`tput sgr0` 
        }
    done &&
    return $res
}

build_var_2() {
    for v in $1; do
        declare -g -A BUILD_$v[$2,$3,]="$(echo $(eval echo \
        \${$v'[,,]'} \${$v'[$2,,]'} \${$v'[,$3,]'} \${$v'[$2,$3,]'}) | xargs)"
        # echo BUILD_$v[$2,$3,] = $(eval echo \${BUILD_$v'[$t,$i,]'})
    done
}

build_var_3() {
    for v in $1; do
        declare -g -A BUILD_$v[$2,$3,$4]="$(echo $(eval echo \
        \${BUILD_$v'[$2,$3,]'} \${$v'[,,$4]'} \${$v'[$2,,$4]'} \${$v'[,$3,$4]'} \${$v'[$2,$3,$4]'}) | xargs)"
        # echo BUILD_$v[$2,$3,$4] = $(eval echo \${BUILD_$v'[$2,$3,$4]'})
    done
}

toolset_var() {
    for v in $1; do
        if [ -z "$(eval echo \${$v'[$2]'})" ]; then 
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
    -B "$BUILD_PATH/$2/$1-$3/$4" \
    -D CMAKE_C_COMPILER="${BUILD_CC[$2]}" \
    -D CMAKE_CXX_COMPILER="${BUILD_CXX[$2]}" \
    -D CMAKE_AR="${BUILD_AR[$2]}" \
    -D CMAKE_BUILD_TYPE="$4" \
    -D CMAKE_C_FLAGS_INIT="${BUILD_C_FLAGS[$2,$3,$4]}" \
    -D CMAKE_C_ARCHIVE_CREATE="<CMAKE_AR> qcs <TARGET> <OBJECTS>" \
    -D CMAKE_C_ARCHIVE_FINISH="" \
    "$1" \
    &>$log &&
    cmake --build "$BUILD_PATH/$2/$1-$3/$4" --target $1 -- VERBOSE=1 -j \
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
    {
        declare log="$LOG_PATH/$2/$1-$3.log" &&
        mkdir -p "${log%/*}" &&
        cmake \
        -B "Xcode/$1-$2" \
        -G "Xcode" \
        -D CMAKE_OSX_ARCHITECTURES="$2" \
        -D CMAKE_C_FLAGS_INIT="$(eval echo \${BUILD_C_FLAGS[Xcode,$2,$3]})" \
        "$1" \
        &>$log ||
        crash $? $log
    } &&
    declare -a pid &&
    for i in $CFG; do
        {
            declare log="$LOG_PATH/Xcode/$1-$3/$i.log" &&
            mkdir -p "${log%/*}" && 
            cmake --build "$BUILD_PATH/Xcode/$1-$2" --target $1 --config $i -- -j &>$log ||
            crash $? $log
        } &
        pid[$!]="cmake --build "$BUILD_PATH/Xcode/$1-$2" --target $1 --config $i -- -j"
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

build_gsl() {
    if [ "$1" == "Xcode" ]; then
        build_library_xcode gsl $2
    else
        build_library_cmake gsl $1 $2
    fi
}

install_library() {
    download_library_git $1 ${VALID_LIBRARY[$1]} &&
    declare -a pid &&
    for t in $TOOLCHAIN; do
        for i in $ARCH; do
            build_$1 $t $i &
            pid[$!]="build_$1 $t $i"
        done
    done &&
    wait_pid pid
}

install() {
    for t in $TOOLCHAIN; do
        if [ "$t" != "Xcode" ]; then
            toolset_var "CC CXX AR" $t ${t%%-*} ${t#${t%%-*}*}
        fi
        for i in $ARCH; do
            build_var_2 "C_FLAGS" $t $i
            for j in $CFG; do 
                build_var_3 "C_FLAGS" $t $i $j
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

clean() {
    for t in $TOOLCHAIN; do
        if [ "$t" == "Xcode" ]; then
            for i in $LIBRARY; do
                if [ ! -z ${VALID_LIBRARY[$i]} ]; then
                    for j in $ARCH; do
                        rm -rf "$BUILD_DIR/$i-$j"
                    done
                fi
            done
        else
            for i in $LIBRARY; do
            
            done
            if [ -z "`ls -A "$t"`" ]; then rm -rf "$t"; fi
        fi
    done
}

validate_var LIBRARY ARCH CFG 
validate_toolchain_var TOOLCHAIN

case "$1" in
    clean-log)
        rm -rf `find . -type d -name '.install_libraries_*'`
        ;;
    clean-source)
        for i in $LIBRARY; do rm -rf "$i"; done 
        ;;
    clean-build)
        clean
        ;;
    *)
        install
esac