clear

mkdir -p build
mkdir -p build/debug
#rm -rf build
#mkdir build
#cmake ..
#cd build/debug
#cmake ../.. -DCMAKE_BUILD_TYPE=Debug
#cd ../..

cmake --build build/debug -j 8 || { echo "Build failed. Exiting."; exit 1; }
gdb -ex run ./build/debug/main
#gdb -ex run ./build/main