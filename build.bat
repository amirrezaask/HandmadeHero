@echo off

mkdir build-out
pushd build-out

cl /std:c++17 /Zi /FC ../src/main.cpp user32.lib gdi32.lib

popd
