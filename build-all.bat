@echo off
if errorlevel 1 goto end

set PATH=C:\cmake\bin;%PATH%

del /Q CMakeCache.txt >nul 2>nul
cmake -DCMAKE_GENERATOR="Visual Studio 16 2019"
if errorlevel 1 goto end

:end
if errorlevel 1 (
  echo.
  echo.
  echo -------------------------------------------------
  echo ! Build FAILED with errors.                     !
  echo -------------------------------------------------
)

exit /b %errorlevel%
