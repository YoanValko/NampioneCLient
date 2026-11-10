@echo off
REM Nemotron Injector - Injects NemotronDLL.dll into Minecraft 1.8.9
REM Run as Administrator!

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

REM Find DLL
set "DLL_PATH=%~dp0NemotronDLL.dll"
if not exist "%DLL_PATH%" (
    echo ERROR: NemotronDLL.dll not found!
    echo Expected at: %DLL_PATH%
    echo.
    echo Make sure you built the project first.
    pause
    exit /b 1
)

echo Looking for Minecraft 1.8.9 process...

REM Find javaw.exe process with "minecraft" in command line
for /f "tokens=2 delims= " %%P in ('wmic process where "name='javaw.exe' and commandline like '%%minecraft%%'" get ProcessId /value ^| find "ProcessId"') do (
    set "PID=%%P"
)

if not defined PID (
    echo.
    echo ERROR: Minecraft 1.8.9 not found!
    echo Make sure Minecraft 1.8.9 is running.
    echo.
    echo Trying to list all javaw.exe processes:
    wmic process where "name='javaw.exe'" get ProcessId,CommandLine
    pause
    exit /b 1
)

echo Found Minecraft PID: %PID%
echo Injecting NemotronDLL.dll...

REM Use PowerShell to inject via CreateRemoteThread
powershell -NoProfile -Command ^
    "$dll = '%DLL_PATH%'; ^
     $pid = %PID%; ^
     $kernel32 = [System.Runtime.InteropServices.Marshal]::GetModuleHandle('kernel32.dll'); ^
     $loadLibrary = [System.Runtime.InteropServices.Marshal]::GetProcAddress($kernel32, 'LoadLibraryW'); ^
     $proc = [System.Diagnostics.Process]::GetProcessById($pid); ^
     $hProc = [Microsoft.Win32.SafeHandles.SafeProcessHandle]::new([System.Runtime.InteropServices.Marshal]::GetProcAddress([System.Runtime.InteropServices.Marshal]::GetModuleHandle('kernel32.dll'), 'OpenProcess'), 0x1F0FFF, 0, $pid); ^
     if (-not $hProc.IsInvalid) { ^
         $size = ($dll.Length + 1) * 2; ^
         $mem = [System.Runtime.InteropServices.Marshal]::AllocHGlobal($size); ^
         [System.Runtime.InteropServices.Marshal]::Copy([char[]]$dll.ToCharArray(), 0, $mem, $dll.Length); ^
         $thread = [System.Runtime.InteropServices.Marshal]::GetProcAddress([System.Runtime.InteropServices.Marshal]::GetModuleHandle('kernel32.dll'), 'CreateRemoteThread'); ^
         $result = [System.Runtime.InteropServices.Marshal]::GetDelegateForFunctionPointer($thread, [Func[IntPtr,IntPtr,UInt32,IntPtr,UInt32,UInt32,IntPtr,IntPtr]]); ^
         $hThread = $result.Invoke($hProc.DangerousGetHandle(), 0, 0, $loadLibrary, $mem, 0, [IntPtr]::Zero, [IntPtr]::Zero); ^
         if ($hThread -ne 0) { ^
             Write-Host 'SUCCESS: Injected into PID' $pid; ^
         } else { ^
             Write-Host 'FAILED: CreateRemoteThread failed'; ^
         } ^
     } else { ^
         Write-Host 'FAILED: OpenProcess failed'; ^
     }"

echo.
echo Done. Check Minecraft for Nemotron Client.
pause