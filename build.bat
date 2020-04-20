@echo off

set RemoveWarnings=/W3
set LibsPath=%~dp0libs
set IncludedLibs=glew/x64/glew32s.lib sqlite/sqlite3.lib user32.lib winmm.lib opengl32.lib shlwapi.lib gdi32.lib Advapi32.lib
set CompilerFlags=/MT /nologo /Gm- /GR- /EHsc /O2 /Oi -WX -W4 %RemoveWarnings% /I%LibsPath% -D_CRT_SECURE_NO_WARNINGS /Zi
set LinkerFlags= -incremental:no /ignore:4099 -LIBPATH:%LibsPath% -opt:ref %IncludedLibs%

pushd %~dp0
IF NOT EXIST .\build mkdir .\build
pushd .\build


del *.pdb > NUL 2> NUL
rc  /fo Yaffe.res ..\Yaffe\Yaffe.rc
cl %CompilerFlags% ..\Yaffe\Yaffe.cpp /link %LinkerFlags% Yaffe.res /SUBSYSTEM:WINDOWS /MACHINE:x64 /ASSEMBLYRESOURCE:Yaffe.res
popd
popd
pause
