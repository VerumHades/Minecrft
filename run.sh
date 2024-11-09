#! /usr/bin/bash

clear
# rm -rf build
# mkdir build
# cmake ..
cmake --build build -j 8 || { echo "Build failed. Exiting."; exit 1; }
./build/main || gdb -ex run ./build/main
#gdb -ex run ./build/main