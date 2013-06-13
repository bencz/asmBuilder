IF EXIST "%PROGRAMFILES(X86)%" (GOTO 64BIT) ELSE (GOTO 32BIT)

:64BIT
set b = "C:\Program Files (x86)\Microsoft Visual Studio 11.0\Common7\Tools"
GOTO BUILD

:32BIT
set b = "C:\Program Files\Microsoft Visual Studio 11.0\Common7\Tools"
GOTO BUILD

:BUILD

%b%\vsvars32.bat && ( cl compiler.c

cd Debug
del *.obj
del *.ilk
del *.tlog
del *.pdb
del *.log
del *.lastbuildstate)