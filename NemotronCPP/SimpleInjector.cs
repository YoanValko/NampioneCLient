using System;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.IO;

namespace NemotronInjector
{
    class Program
    {
        [DllImport("kernel32.dll", SetLastError = true)]
        static extern IntPtr OpenProcess(uint dwDesiredAccess, bool bInheritHandle, int dwProcessId);

        [DllImport("kernel32.dll", SetLastError = true)]
        static extern IntPtr GetProcAddress(IntPtr hModule, string lpProcName);

        [DllImport("kernel32.dll", SetLastError = true)]
        static extern IntPtr GetModuleHandle(string lpModuleName);

        [DllImport("kernel32.dll", SetLastError = true)]
        static extern IntPtr VirtualAllocEx(IntPtr hProcess, IntPtr lpAddress, uint dwSize, uint flAllocationType, uint flProtect);

        [DllImport("kernel32.dll", SetLastError = true)]
        static extern bool WriteProcessMemory(IntPtr hProcess, IntPtr lpBaseAddress, byte[] lpBuffer, uint nSize, out IntPtr lpNumberOfBytesWritten);

        [DllImport("kernel32.dll", SetLastError = true)]
        static extern IntPtr CreateRemoteThread(IntPtr hProcess, IntPtr lpThreadAttributes, uint dwStackSize, IntPtr lpStartAddress, IntPtr lpParameter, uint dwCreationFlags, out IntPtr lpThreadId);

        [DllImport("kernel32.dll", SetLastError = true)]
        static extern bool CloseHandle(IntPtr hObject);

        [DllImport("kernel32.dll", SetLastError = true)]
        static extern uint WaitForSingleObject(IntPtr hHandle, uint dwMilliseconds);

        [DllImport("kernel32.dll", SetLastError = true)]
        static extern bool GetExitCodeThread(IntPtr hThread, out uint lpExitCode);

        const uint PROCESS_ALL_ACCESS = 0x1F0FFF;
        const uint MEM_COMMIT = 0x1000;
        const uint MEM_RESERVE = 0x2000;
        const uint PAGE_READWRITE = 0x04;
        const uint INFINITE = 0xFFFFFFFF;

        static void Main(string[] args)
        {
            Console.WriteLine("========================================");
            Console.WriteLine("Nemotron Client Injector");
            Console.WriteLine("========================================");
            Console.WriteLine();

            // Check admin
            if (!IsAdministrator())
            {
                Console.WriteLine("ERROR: Please run as Administrator!");
                Console.WriteLine("Right-click -> Run as administrator");
                Console.ReadKey();
                return;
            }

            // Find DLL
            string dllPath = Path.GetFullPath("NemotronDLL.dll");
            if (!File.Exists(dllPath))
            {
                Console.WriteLine("ERROR: NemotronDLL.dll not found!");
                Console.WriteLine("Expected at: " + dllPath);
                Console.WriteLine("Make sure you built the project first.");
                Console.ReadKey();
                return;
            }

            Console.WriteLine("DLL Path: " + dllPath);
            Console.WriteLine();

            // Find Minecraft process
            Console.WriteLine("Searching for Minecraft 1.8.9...");
            int pid = FindMinecraftProcess();
            
            if (pid == 0)
            {
                Console.WriteLine();
                Console.WriteLine("ERROR: Minecraft 1.8.9 not found!");
                Console.WriteLine("Make sure Minecraft 1.8.9 is running.");
                Console.WriteLine();
                Console.WriteLine("All javaw.exe processes:");
                ListJavaProcesses();
                Console.ReadKey();
                return;
            }

            Console.WriteLine("Found Minecraft PID: " + pid);
            Console.WriteLine("Injecting NemotronDLL.dll...");

            // Inject
            if (InjectDLL(pid, dllPath))
            {
                Console.WriteLine();
                Console.WriteLine("SUCCESS: NemotronDLL.dll injected successfully!");
                Console.WriteLine("Nemotron Client is now running in Minecraft.");
                Console.WriteLine("Launch the external GUI (gui\\run_gui.bat) to control modules.");
            }
            else
            {
                Console.WriteLine();
                Console.WriteLine("FAILED: Injection failed!");
            }

            Console.ReadKey();
        }

        static bool IsAdministrator()
        {
            var identity = System.Security.Principal.WindowsIdentity.GetCurrent();
            var principal = new System.Security.Principal.WindowsPrincipal(identity);
            return principal.IsInRole(System.Security.Principal.WindowsBuiltInRole.Administrator);
        }

        static int FindMinecraftProcess()
        {
            Process[] processes = Process.GetProcessesByName("javaw");
            foreach (Process proc in processes)
            {
                try
                {
                    string cmdLine = GetCommandLine(proc.Id);
                    if (cmdLine != null && cmdLine.ToLower().Contains("minecraft"))
                    {
                        return proc.Id;
                    }
                }
                catch { }
            }
            return 0;
        }

        static string GetCommandLine(int processId)
        {
            try
            {
                using (System.Management.ManagementObjectSearcher searcher = 
                    new System.Management.ManagementObjectSearcher("SELECT CommandLine FROM Win32_Process WHERE ProcessId = " + processId))
                {
                    foreach (System.Management.ManagementObject obj in searcher.Get())
                    {
                        return obj["CommandLine"]?.ToString();
                    }
                }
            }
            catch { }
            return null;
        }

        static void ListJavaProcesses()
        {
            Process[] processes = Process.GetProcessesByName("javaw");
            foreach (Process proc in processes)
            {
                try
                {
                    string cmdLine = GetCommandLine(proc.Id);
                    Console.WriteLine("  PID: " + proc.Id + " | " + (cmdLine ?? "N/A"));
                }
                catch
                {
                    Console.WriteLine("  PID: " + proc.Id + " | (access denied)");
                }
            }
        }

        static bool InjectDLL(int processId, string dllPath)
        {
            IntPtr hProcess = OpenProcess(PROCESS_ALL_ACCESS, false, processId);
            if (hProcess == IntPtr.Zero)
            {
                Console.WriteLine("OpenProcess failed: " + Marshal.GetLastWin32Error());
                return false;
            }

            try
            {
                IntPtr loadLibraryAddr = GetProcAddress(GetModuleHandle("kernel32.dll"), "LoadLibraryW");
                if (loadLibraryAddr == IntPtr.Zero)
                {
                    Console.WriteLine("GetProcAddress failed");
                    return false;
                }

                // Allocate memory for DLL path
                byte[] dllPathBytes = System.Text.Encoding.Unicode.GetBytes(dllPath + "\0");
                uint size = (uint)dllPathBytes.Length;
                IntPtr remotePath = VirtualAllocEx(hProcess, IntPtr.Zero, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
                if (remotePath == IntPtr.Zero)
                {
                    Console.WriteLine("VirtualAllocEx failed: " + Marshal.GetLastWin32Error());
                    return false;
                }

                // Write DLL path
                IntPtr bytesWritten;
                if (!WriteProcessMemory(hProcess, remotePath, dllPathBytes, size, out bytesWritten))
                {
                    Console.WriteLine("WriteProcessMemory failed: " + Marshal.GetLastWin32Error());
                    VirtualFreeEx(hProcess, remotePath, 0, 0x8000); // MEM_RELEASE
                    return false;
                }

                // Create remote thread
                IntPtr threadId;
                IntPtr hThread = CreateRemoteThread(hProcess, IntPtr.Zero, 0, loadLibraryAddr, remotePath, 0, out threadId);
                if (hThread == IntPtr.Zero)
                {
                    Console.WriteLine("CreateRemoteThread failed: " + Marshal.GetLastWin32Error());
                    VirtualFreeEx(hProcess, remotePath, 0, 0x8000);
                    return false;
                }

                // Wait for thread to finish
                WaitForSingleObject(hThread, 5000);
                GetExitCodeThread(hThread, out uint exitCode);

                // Cleanup
                VirtualFreeEx(hProcess, remotePath, 0, 0x8000);
                CloseHandle(hThread);

                Console.WriteLine("Injection thread exit code: " + exitCode);
                return exitCode != 0;
            }
            finally
            {
                CloseHandle(hProcess);
            }
        }

        // P/Invoke for VirtualFreeEx
        [DllImport("kernel32.dll", SetLastError = true)]
        static extern bool VirtualFreeEx(IntPtr hProcess, IntPtr lpAddress, uint dwSize, uint dwFreeType);
    }
}