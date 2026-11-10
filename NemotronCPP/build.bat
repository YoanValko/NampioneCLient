@echo off
REM NemotronCPP Build Script
REM Requires: Visual Studio 2019/2022, CMake 3.15+, Java JDK

echo ==========================================
echo NemotronCPP Build Script
echo ==========================================

REM Check for CMake
cmake --version >nul 2>&1
if errorlevel 1 (
    echo ERROR: CMake not found in PATH
    echo Download from https://cmake.org/download/
    pause
    exit /b 1
)

REM Check for Visual Studio
if not defined VisualStudioVersion (
    echo ERROR: Run this from Developer Command Prompt for Visual Studio
    echo Or set up environment with vcvarsall.bat
    pause
    exit /b 1
)

REM Check for Java
java -version >nul 2>&1
if errorlevel 1 (
    echo ERROR: Java not found in PATH
    pause
    exit /b 1
)

REM Create build directory
if not exist build mkdir build
cd build

echo Configuring with CMake...
cmake .. -G "Visual Studio 17 2022" -A x64
if errorlevel 1 (
    echo CMake configure failed
    pause
    exit /b 1
)

echo Building...
cmake --build . --config Release
if errorlevel 1 (
    echo Build failed
    pause
    exit /b 1
)

cd ..

echo.
echo ==========================================
echo Build successful!
echo ==========================================
echo Executables in: build\bin\Release\
echo.
echo Files created:
echo   - NemotronInjector.exe (injector)
echo   - NemotronDLL.dll (injected DLL)
echo.
echo To use:
echo   1. Start Minecraft 1.8.9
echo   2. Run NemotronInjector.exe as Administrator
echo   3. Use external Python GUI to control modules
echo.
pause