@echo off
cls
rmdir build /s /q
mkdir build
cd build
cmake ..
cd ..