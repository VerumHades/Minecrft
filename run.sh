#! /usr/bin/bash

clear
# rm -rf build
# mkdir build
cd build
# cmake ..
cmake --build .
cd ..
#./build/main
gdb -ex run ./build/main