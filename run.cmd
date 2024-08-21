del build
mkdir build
cd build
cmake ..
cmake --build . --config Release
cd ..
gdb build\Release\main.exe
