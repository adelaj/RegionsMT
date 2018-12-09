# Run Sample: 
#   ARCH="i386 x86_64" CFG="Debug Release" bash lib.sh

if [[ -z $ARCH ]]; then ARCH=$(arch); fi
if [[ -z $CFG ]]; then CFG=Release; fi
LIB=gsl
C_FLAGS="-flto -fuse-linker-plugin -mavx"
C_FLAGS_i386="-m32"
C_FLAGS_x86_64="-m64"
build_lib() {
    DIR=$1-$2-$3
    rm -rf $DIR
    mkdir $DIR
    cd $DIR
    C_FLAGS_INIT=$(echo $C_FLAGS $(eval echo "\$C_FLAGS_$2") | xargs)
    cmake -D CMAKE_BUILD_TYPE="$3" -D CMAKE_C_FLAGS_INIT="$C_FLAGS_INIT" -D CMAKE_AR="/usr/bin/gcc-ar" -D CMAKE_C_ARCHIVE_CREATE="<CMAKE_AR> qcs <TARGET> <OBJECTS>" -D CMAKE_C_ARCHIVE_FINISH=true -D CMAKE_REQUIRED_LIBRARIES="m" ../$1
    make VERBOSE=1 $1
    cd ..
}
for i in $LIB; do
    for j in $ARCH; do
        for k in $CFG; do
            build_lib $i $j $k
        done
    done
done
