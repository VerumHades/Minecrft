@echo off
cls
::cmake --build ./build --config Debug  -j 8 &&  gdb -ex run build\Debug\main.exe
cmake --build ./build --config Release -j 8 && gdb -ex run build\main.exe
::gdb -ex run build\main.exe

