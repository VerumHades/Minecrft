@echo off
cmake --build ./build --config Debug  -j 8 &&  gdb -ex run build\main.exe