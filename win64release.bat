@echo off

mkdir build64r
cd build64r

cmake -DCMAKE_BUILD_TYPE=Release -G "Visual Studio 16 2019" -A x64 -Wdev ..

start /b PuffinEngine.sln
cd ..