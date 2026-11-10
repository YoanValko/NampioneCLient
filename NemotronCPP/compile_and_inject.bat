@echo off
REM Nemotron Client - Compile and Inject
REM Compiles SimpleInjector.cs with csc (built into Windows) and runs it

echo ==========================================
echo Nemotron Client - Compile & Inject
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

REM Check for NemotronDLL.dll
if not exist "NemotronDLL.dll" (
    echo ERROR: NemotronDLL.dll not found in current directory!
    echo Make sure you built the C++ project first.
    echo Expected: %~dp0NemotronDLL.dll
    pause
    exit /b 1
)

REM Find csc.exe (C# compiler)
set CSC_PATH=
for /r "%windir%\Microsoft.NET\Framework" %%f in (csc.exe) do set CSC_PATH=%%f
for /r "%windir%\Microsoft.NET\Framework64" %%f in (csc.exe) do set CSC_PATH=%%f

if not defined CSC_PATH (
    echo ERROR: csc.exe (C# compiler) not found!
    echo Install .NET Framework 4.x or Visual Studio.
    pause
    exit /b 1
)

echo Found C# compiler: %CSC_PATH%
echo.

REM Compile the injector
echo Compiling SimpleInjector.cs...
%CSC_PATH% /nologo /optimize+ /target:exe /out:NemotronInjector.exe SimpleInjector.cs /platform:x64 2>&1

if errorlevel 1 (
    echo.
    echo COMPILATION FAILED!
    pause
    exit /b 1
)

echo Compilation successful!
echo.

REM Run the injector
echo Running injector...
NemotronInjector.exe

echo.
pause