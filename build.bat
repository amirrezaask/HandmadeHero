@echo off
if not exist "build-out" mkdir build-out
pushd build-out

cl /Fmwin32_handmadehero.map -Gm- -nologo -EHa -Oi -GR- -W4 -wd4100 -wd4189 -wd4701 -wd4244 -wd4018 -wd4211 -DHANDMADE_WIN32=1 -DHANDMADE_SLOW=1 -DHANDMADE_INTERNAL=1 /std:c++17 /Z7 /FC ../src/win32_handmadehero.cpp user32.lib gdi32.lib

popd
