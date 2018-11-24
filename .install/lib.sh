declare -a lib=(gsl)
declare -a arch=(i386 x86_64)
declare -a conf=(Debug Release)
build_lib() {
    dir=$1-$2-$3
    rm -rf $dir
    mkdir $dir
    cd $dir
    declare -A flags=([i386]=" -m32" [x86_64]="")
    cmake -D CMAKE_BUILD_TYPE="$3" -D CMAKE_C_FLAGS_INIT="-flto -fuse-linker-plugin -mavx${flags[$2]}" -D CMAKE_AR="/usr/bin/gcc-ar" -D CMAKE_C_ARCHIVE_CREATE="<CMAKE_AR> qcs <TARGET> <OBJECTS>" -D CMAKE_C_ARCHIVE_FINISH=true -D CMAKE_REQUIRED_LIBRARIES="m" ../$lib
    make VERBOSE=1 $1
    cd ..
}
for i in ${lib[*]}; do
    for j in ${arch[*]}; do
        for k in ${conf[*]}; do
            build_lib $i $j $k
        done
    done
done

