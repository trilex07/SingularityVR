@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x86 >nul
if errorlevel 1 exit /b 1
cd /d C:\Users\Trilex\vr-mods\SingularityVR-production\spikes\view_matrix
set SDK=C:\Users\Trilex\vr-mods\dependencies\openxr-local
set MH=C:\Users\Trilex\vr-mods\SingularityVR-production\third_party\minhook
set GAMEBIN=C:\Program Files (x86)\Steam\steamapps\common\Singularity\Binaries

REM Compile
cl /nologo /LD /EHsc /W3 /Zi /MD /std:c++17 /O2 /DPERFORMANCE_BUILD_1 ^
    /I"%SDK%\include" /I"%MH%\include" ^
    /Fe:d3d9.dll d3d9.cpp ^
    "%MH%\src\buffer.c" "%MH%\src\hook.c" "%MH%\src\trampoline.c" "%MH%\src\hde\hde32.c"

REM Link with response file to avoid quoting issues
echo /MAP:d3d9.map > link.rsp
echo /DEF:d3d9.def >> link.rsp
echo /LIBPATH:"%GAMEBIN%" >> link.rsp
echo /LIBPATH:. >> link.rsp
echo "C:\Users\Trilex\vr-mods\SingularityVR-production\spikes\view_matrix\openxr_loader.lib" >> link.rsp
echo dxgi.lib >> link.rsp
echo user32.lib >> link.rsp
echo shell32.lib >> link.rsp

link @link.rsp /OUT:d3d9.dll /DLL /MAP:d3d9.map /DEF:d3d9.def ^
    d3d9.obj buffer.obj hook.obj trampoline.obj hde32.obj

if errorlevel 1 exit /b 1
echo Build OK
dir d3d9.dll