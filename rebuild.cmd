@echo off
 cls
::rmdir build /s /q
::mkdir build
cd build
cmake -DCMAKE_COLOR_MAKEFILE=ON -G "Unix Makefiles" ..
cd ..