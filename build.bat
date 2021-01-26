@echo off
call ..\bin\buildsuper_x64-win.bat .\4fda0.cpp release
move .\custom_4coder.dll ..\..\custom_4coder.dll
move .\custom_4coder.pdb ..\..\custom_4coder.pdb
move .\vc140.pdb ..\..\vc140.pdb