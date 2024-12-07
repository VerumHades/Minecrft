#! /usr/bin/bash

mkdir -p build
mkdir -p build/release
# rm -rf build
# mkdir build
# cmake ..
cd build/release
cmake ../.. -DCMAKE_BUILD_TYPE=Release
cd ../..

cmake --build build/release -j 8 || { echo "Build failed. Exiting."; exit 1; }
run ./build/release/main
#gdb -ex run ./build/main