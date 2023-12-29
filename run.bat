@echo off

mkdir build-out
pushd build-out

cl ../src/main.cpp user32.lib

popd

start .\build-out\main.exe
