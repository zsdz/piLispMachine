@echo off
rem MAKEIT.BAT - a script to make GNU Go

rem  If you get funny error messages when compiling, try
rem  to uncomment one of the lines below, and edit the path to 
rem  match your system. The second version is for version 6,
rem  which has its files in a different place from version 5.
 
rem call "c:\Program Files\DevStudio\VC\Bin\Vcvars32.bat"
rem call "c:\Program Files\Microsoft Visual Studio\VC98\Bin\Vcvars32.bat"

copy ..\config.vc config.h
nmake -f makefile.win

