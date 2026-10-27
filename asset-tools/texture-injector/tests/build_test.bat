@echo off
"C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x86 >nul 2>nul
cd /d "C:\Users\Trilex\vr-mods\SingularityVR-assets\asset-tools\texture-injector\tests"
cl /nologo /EHsc /W4 /O2 /std:c++17 /Fe:"C:\Users\Trilex\vr-mods\SingularityVR-assets-local\build\texgroups-test\texture_groups_test.exe" /Fo:"C:\Users\Trilex\vr-mods\SingularityVR-assets-local\build\texgroups-test\" texture_groups_test.cpp
if %errorlevel% neq 0 exit /b 1
"C:\Users\Trilex\vr-mods\SingularityVR-assets-local\build\texgroups-test\texture_groups_test.exe"

