@echo off

mkdir build64d
cd build64d

cmake -DCMAKE_BUILD_TYPE=Debug -G "Visual Studio 16 2019" -A x64 -Wdev ..

start /b PuffinEngine.sln
cd ..