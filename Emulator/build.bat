@echo off
cd ..\compiler
python fc.py
cd ..\emulator
copy /Y ..\compiler\*.h* .
mingw32-make

