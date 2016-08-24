@echo off
python ..\compiler\fc.py
copy /Y ..\compiler\*.h* .
mingw32-make

