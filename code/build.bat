@echo off

REM TODO - can we just build both with one exe?

IF NOT EXIST ..\build mkdir ..\build
pushd ..\build
cl -MT -nologo -Gm -GR- -EHa- -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -wd4996 -DHANDMADE_INTERNAL=1 -DHANDMADE_SLOW=1 -DHANDMADE_WIN32=1 -FC -Zi -Fmwin32_handmade.map ..\code\win32_handmade.cpp /link -opt:ref -subsystem:windows,5.1 user32.lib gdi32.lib
popd