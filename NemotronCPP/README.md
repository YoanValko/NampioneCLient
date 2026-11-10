# NemotronCPP - C++ Injector for Minecraft 1.8.9

A complete C++ injector with external GUI for Minecraft 1.8.9 featuring AutoClicker, ESP, and Humanized Aimbot.

## Features

- **C++ DLL Injection** - No `-XX:+AllowAttachListener` required
- **JNI Integration** - Direct Java method calls from C++
- **External Python GUI** - Control modules from separate window
- **Named Pipe IPC** - Real-time communication between GUI and injected DLL
- **Modules:**
  - 🖱 **AutoClicker** - Configurable CPS, jitter, weapon check
  - 👁 **ESP** - Players, mobs, animals, chests, tracers, boxes
  - 🎯 **Humanized Aimbot** - Smooth aiming, prediction, anti-cheat bypass

## Architecture

```
NemotronCPP/
├── injector.exe          # Finds MC process, injects DLL
├── NemotronDLL.dll       # Injected into MC, uses JNI
├── gui/nemotron_gui.py   # External Python GUI
└── build/                # Build output
```

## Requirements

- **Visual Studio 2019/2022** (with C++ workload)
- **CMake 3.15+**
- **Java JDK 8+** (for JNI headers)
- **Python 3.7+** (for external GUI)
- **pywin32** (auto-installed by GUI launcher)

## Building

### Option 1: Build Script (Easiest)
```cmd
cd NemotronCPP
build.bat
```

### Option 2: Manual CMake
```cmd
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

## Usage

### 1. Start Minecraft 1.8.9
- Use Forge or vanilla 1.8.9
- No special JVM flags needed

### 2. Inject the DLL
- Run `build\bin\Release\NemotronInjector.exe` **as Administrator**
- It will auto-detect the Minecraft process and inject

### 3. Launch External GUI
- Run `gui\run_gui.bat`
- GUI connects automatically via named pipe
- Toggle modules and configure settings in real-time

## Module Details

### AutoClicker
- **CPS Range**: 1-20 clicks per second
- **Jitter**: ±15% randomization
- **Modes**: Left click, Right click, Both
- **Weapon Check**: Only clicks when holding sword/tool
- **Hit Delay**: Additional delay after hit

### ESP
- **Entities**: Players, Mobs, Animals, Chests
- **Visuals**: Boxes, Tracers, Health, Names
- **Max Distance**: Configurable (10-500 blocks)

### Humanized Aimbot
- **FOV Limiter**: Only aims within your view
- **Smooth Aiming**: Configurable smoothing factor
- **Movement Prediction**: Leads moving targets
- **Humanization**:
  - Reaction time simulation
  - Overshoot correction
  - Micro-jitter (natural hand tremor)
  - Target switching delay
- **Hitbox Selection**: Head, Body, Random
- **Visible Check**: Only targets visible entities

## IPC Protocol

Named pipe: `\\.\pipe\NemotronIPC`

**Commands:**
- `INIT` (0) - Get initial status
- `TOGGLE_MODULE` (1) - Enable/disable module
- `SET_SETTING` (2) - Update module settings
- `GET_STATUS` (3) - Request current status
- `EXECUTE_JAVA` (4) - Execute Java code
- `SHUTDOWN` (5) - Unload DLL

**Module Types:**
- `AUTOMATIC_CLICKER` (1)
- `ESP` (2)
- `AIMBOT` (3)

## Troubleshooting

| Issue | Solution |
|-------|----------|
| "Access denied" | Run injector as Administrator |
| "Process not found" | Make sure Minecraft 1.8.9 is running |
| "DLL not found" | Build project first, check `build\bin\Release\` |
| GUI won't connect | Make sure injector ran successfully, restart GUI |
| "pywin32 not found" | Run `pip install pywin32` |

## Anti-Cheat Notes

This client includes humanization features but **no client is undetectable**. Use at your own risk on servers with anti-cheat.

## Legal Disclaimer

This is for educational purposes only. Use on servers you own or have permission to test on. The authors are not responsible for any bans or consequences.

## License

Educational use only.