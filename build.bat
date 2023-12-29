@echo off

mkdir build-out
pushd build-out

cl ../src/main.cpp user32.lib gdi32.lib

popd
