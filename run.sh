#! /usr/bin/bash

clear
# rm -rf build
# mkdir build
cd build
# cmake ..
cmake --build . -j 8 || { echo "Build failed. Exiting."; exit 1; }
cd ..
#./build/main
gdb -ex run ./build/main