@echo off
if errorlevel 1 goto end

set PATH=D:\Tools\cmake\bin;C:\cmake\bin;%PATH%
rem mkdir x64
rem cd x64
del /Q CMakeCache.txt >nul 2>nul
rem cmake -DCMAKE_GENERATOR="Visual Studio 14 2015 Win64" %* ..
rem cmake -G "Visual Studio 14 2015 Win64"
cmake -DCMAKE_GENERATOR="Visual Studio 14 2015 Win64" %*
if errorlevel 1 goto end
rem cd ..

:end
if errorlevel 1 (
  echo.
  echo.
  echo -------------------------------------------------
  echo ! Build FAILED with errors.                     !
  echo -------------------------------------------------
)

exit /b %errorlevel%
