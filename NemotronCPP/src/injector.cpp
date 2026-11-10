#include <windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <Psapi.h>

#pragma comment(lib, "Psapi.lib")

namespace Nemotron {

    // Find Minecraft process
    DWORD FindMinecraftProcess() {
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snapshot == INVALID_HANDLE_VALUE) return 0;
        
        PROCESSENTRY32W entry;
        entry.dwSize = sizeof(entry);
        
        if (Process32FirstW(snapshot, &entry)) {
            do {
                std::wstring name = entry.szExeFile;
                if (name.find(L"java") != std::wstring::npos ||
                    name.find(L"javaw") != std::wstring::npos) {
                    
                    // Verify it's Minecraft by checking command line
                    HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ, FALSE, entry.th32ProcessID);
                    if (process) {
                        wchar_t cmdLine[4096];
                        DWORD size = 4096;
                        if (GetProcessImageFileNameW(process, cmdLine, 4096)) {
                            std::wstring cmd(cmdLine);
                            if (cmd.find(L"minecraft") != std::wstring::npos ||
                                cmd.find(L"Minecraft") != std::wstring::npos ||
                                cmd.find(L"forge") != std::wstring::npos ||
                                cmd.find(L"fabric") != std::wstring::npos) {
                                CloseHandle(process);
                                CloseHandle(snapshot);
                                return entry.th32ProcessID;
                            }
                        }
                        CloseHandle(process);
                    }
                }
            } while (Process32NextW(snapshot, &entry));
        }
        
        CloseHandle(snapshot);
        return 0;
    }
    
    // Inject DLL into process
    bool InjectDLL(DWORD processId, const std::wstring& dllPath) {
        HANDLE process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
        if (!process) {
            std::cerr << "Failed to open process: " << GetLastError() << std::endl;
            return false;
        }
        
        // Allocate memory in target process
        size_t pathSize = (dllPath.length() + 1) * sizeof(wchar_t);
        LPVOID remotePath = VirtualAllocEx(process, nullptr, pathSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (!remotePath) {
            std::cerr << "VirtualAllocEx failed: " << GetLastError() << std::endl;
            CloseHandle(process);
            return false;
        }
        
        // Write DLL path to target process
        if (!WriteProcessMemory(process, remotePath, dllPath.c_str(), pathSize, nullptr)) {
            std::cerr << "WriteProcessMemory failed: " << GetLastError() << std::endl;
            VirtualFreeEx(process, remotePath, 0, MEM_RELEASE);
            CloseHandle(process);
            return false;
        }
        
        // Get LoadLibraryW address
        HMODULE kernel32 = GetModuleHandleW(L"kernel32.dll");
        LPVOID loadLibraryAddr = GetProcAddress(kernel32, "LoadLibraryW");
        if (!loadLibraryAddr) {
            std::cerr << "GetProcAddress failed: " << GetLastError() << std::endl;
            VirtualFreeEx(process, remotePath, 0, MEM_RELEASE);
            CloseHandle(process);
            return false;
        }
        
        // Create remote thread
        HANDLE thread = CreateRemoteThread(process, nullptr, 0, 
            reinterpret_cast<LPTHREAD_START_ROUTINE>(loadLibraryAddr), 
            remotePath, 0, nullptr);
        
        if (!thread) {
            std::cerr << "CreateRemoteThread failed: " << GetLastError() << std::endl;
            VirtualFreeEx(process, remotePath, 0, MEM_RELEASE);
            CloseHandle(process);
            return false;
        }
        
        // Wait for thread to finish
        WaitForSingleObject(thread, 5000);
        
        DWORD exitCode;
        GetExitCodeThread(thread, &exitCode);
        
        // Cleanup
        VirtualFreeEx(process, remotePath, 0, MEM_RELEASE);
        CloseHandle(thread);
        CloseHandle(process);
        
        return exitCode != 0;
    }
    
    // Eject DLL from process
    bool EjectDLL(DWORD processId, const std::wstring& dllName) {
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, processId);
        if (snapshot == INVALID_HANDLE_VALUE) return false;
        
        MODULEENTRY32W entry;
        entry.dwSize = sizeof(entry);
        
        HMODULE targetModule = nullptr;
        if (Module32FirstW(snapshot, &entry)) {
            do {
                if (_wcsicmp(entry.szModule, dllName.c_str()) == 0) {
                    targetModule = entry.hModule;
                    break;
                }
            } while (Module32NextW(snapshot, &entry));
        }
        CloseHandle(snapshot);
        
        if (!targetModule) return false;
        
        HANDLE process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
        if (!process) return false;
        
        HMODULE kernel32 = GetModuleHandleW(L"kernel32.dll");
        LPVOID freeLibraryAddr = GetProcAddress(kernel32, "FreeLibrary");
        
        HANDLE thread = CreateRemoteThread(process, nullptr, 0,
            reinterpret_cast<LPTHREAD_START_ROUTINE>(freeLibraryAddr),
            targetModule, 0, nullptr);
        
        if (!thread) {
            CloseHandle(process);
            return false;
        }
        
        WaitForSingleObject(thread, 5000);
        CloseHandle(thread);
        CloseHandle(process);
        return true;
    }

} // namespace Nemotron

int main(int argc, char* argv[]) {
    std::cout << "========================================" << std::endl;
    std::cout << "Nemotron C++ Injector v1.0" << std::endl;
    std::cout << "========================================" << std::endl;
    
    // Get DLL path
    std::filesystem::path dllPath = std::filesystem::current_path() / "NemotronDLL.dll";
    if (!std::filesystem::exists(dllPath)) {
        dllPath = std::filesystem::path(argv[0]).parent_path() / "NemotronDLL.dll";
    }
    
    if (!std::filesystem::exists(dllPath)) {
        std::cerr << "ERROR: NemotronDLL.dll not found!" << std::endl;
        std::cerr << "Expected at: " << dllPath << std::endl;
        return 1;
    }
    
    std::wstring wDllPath = dllPath.wstring();
    std::wcout << L"DLL Path: " << wDllPath << std::endl;
    
    // Find Minecraft
    std::cout << "Searching for Minecraft process..." << std::endl;
    DWORD pid = Nemotron::FindMinecraftProcess();
    
    if (pid == 0) {
        std::cerr << "ERROR: Minecraft process not found!" << std::endl;
        std::cerr << "Make sure Minecraft 1.8.9 is running." << std::endl;
        return 1;
    }
    
    std::cout << "Found Minecraft PID: " << pid << std::endl;
    
    // Inject
    std::cout << "Injecting NemotronDLL.dll..." << std::endl;
    if (Nemotron::InjectDLL(pid, wDllPath)) {
        std::cout << "SUCCESS: DLL injected successfully!" << std::endl;
        std::cout << "Nemotron Client is now running in Minecraft." << std::endl;
        std::cout << "Use the external GUI to control modules." << std::endl;
        return 0;
    } else {
        std::cerr << "FAILED: DLL injection failed!" << std::endl;
        return 1;
    }
}