@echo off
REM Nemotron External GUI Launcher
REM Requires: Python 3.7+, pywin32

echo Starting Nemotron External GUI...

REM Check for Python
python --version >nul 2>&1
if errorlevel 1 (
    echo Python not found. Trying 'py'...
    py --version >nul 2>&1
    if errorlevel 1 (
        echo Neither 'python' nor 'py' found.
        echo Install Python from https://python.org
        pause
        exit /b 1
    ) else (
        set PYTHON_CMD=py
    )
) else (
    set PYTHON_CMD=python
)

REM Check for pywin32
%PYTHON_CMD% -c "import win32file" 2>nul
if errorlevel 1 (
    echo Installing pywin32...
    %PYTHON_CMD% -m pip install pywin32 --quiet
    if errorlevel 1 (
        echo Failed to install pywin32
        pause
        exit /b 1
    )
)

REM Run GUI
echo Launching GUI...
%PYTHON_CMD% "%~dp0\nemotron_gui.py"

if errorlevel 1 (
    echo.
    echo GUI exited with error.
    pause
)