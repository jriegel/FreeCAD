@echo off
rem   Build script, uses vcbuild to completetly build FreeCAD

rem start again nice (LOW)
if "%1"=="" (
    start /WAIT /LOW /B cmd.exe /V /C %~s0 go_ahead
    goto:eof
)
rem  set the aprobiated Variables here or outside in the system
if NOT DEFINED VCDIR set VCDIR=C:\Program Files (x86)\Microsoft Visual Studio 12.0

rem Register VS Build programms
call "%VCDIR%\VC\vcvarsall.bat"

rem "C:\Program Files\TortoiseSVN\bin\TortoiseProc.exe" /command:update /path:"C:\SW_Projects\CAD\FreeCAD_10" /closeonend:3


rem Start the Visuall Studio build process
msbuild.exe build\FreeCAD_trunk.sln /t:ZERO_CHECK
msbuild.exe build\FreeCAD_trunk.sln /m /p:Configuration="Debug" 
msbuild.exe build\FreeCAD_trunk.sln /m /p:Configuration="Release" 

PAUSE

