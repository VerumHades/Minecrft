clear

mkdir -p build

build_type="Release"

if [[ "$1" == "d" ]]; then
    build_type="Debug"
fi

path="build/$build_type"

mkdir -p "build/$build_type"

if [[ "$2" == "r" ]]; then
    echo "Remaking build"

    cd "build/"
    cmake ../.. -DCMAKE_BUILD_TYPE=Debug
    cd ../..
else
    echo "The first argument is not 'something'. Skipping commands."
fi


cmake --build build/debug -j 8 || { echo "Build failed. Exiting."; exit 1; }
gdb -ex run ./build/debug/main
#gdb -ex run ./build/main