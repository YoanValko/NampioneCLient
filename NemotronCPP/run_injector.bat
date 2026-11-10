@echo off
REM Nemotron Client - Run Injector
REM Runs the compiled injector (NemotronInjector.exe)

echo ==========================================
echo Nemotron Client Injector
echo ==========================================
echo.

REM Check admin
net session >nul 2>&1
if %errorlevel% neq 0 (
    echo ERROR: Please run as Administrator!
    echo Right-click this file -> "Run as administrator"
    pause
    exit /b 1
)

REM Check for injector
if not exist "NemotronInjector.exe" (
    echo NemotronInjector.exe not found.
    echo Run compile_and_inject.bat first, or build the C++ project.
    pause
    exit /b 1
)

REM Check for DLL
if not exist "NemotronDLL.dll" (
    echo ERROR: NemotronDLL.dll not found!
    echo Build the C++ project first (build.bat).
    pause
    exit /b 1
)

REM Run injector
NemotronInjector.exe

pause