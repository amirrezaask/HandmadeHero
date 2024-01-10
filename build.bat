@echo off

mkdir build-out
pushd build-out

cl -DHANDMADE_WIN32=1 -DHANDMADE_SLOW=1 -DHANDMADE_INTERNAL=1 /std:c++17 /Zi /FC ../src/win32_handmadehero.cpp user32.lib gdi32.lib

popd
