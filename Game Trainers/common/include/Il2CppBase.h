#pragma once

#include "TrainerBase.h"
#include <psapi.h>

class Il2CppBase : public TrainerBase
{
public:
    HMODULE hInjectedDll = NULL;
    void *remoteArgMemory = nullptr;
    std::string injectedDllPath;
    uintptr_t remoteModuleBase = 0;
    uintptr_t localModuleBase = 0;

    // Response communication with injected DLL
    HANDLE hResponseMapFile = NULL;
    LPVOID responseBuffer = nullptr;
    HANDLE hResponseEvent = NULL;
    const size_t responseBufferSize = 1024 * 1024;

    const char *DLL_RESOURCE_NAME = "IL2CPP_DLL";

    Il2CppBase(const std::wstring &processIdentifier, bool useWindowTitle = false)
        : TrainerBase(processIdentifier, useWindowTitle)
    {
    }

    virtual ~Il2CppBase()
    {
        unloadDll();
    }

    // Extract DLL from resources to a temp file with unique name per trainer
    std::string extractDllFromResource()
    {
        HMODULE hModule = GetModuleHandle(nullptr);
        HRSRC hResource = FindResourceA(hModule, DLL_RESOURCE_NAME, MAKEINTRESOURCEA(10));
        if (!hResource)
        {
            std::cerr << "[!] Failed to find resource: " << DLL_RESOURCE_NAME << "\n";
            return "";
        }

        HGLOBAL hData = LoadResource(hModule, hResource);
        if (!hData)
        {
            std::cerr << "[!] Failed to load resource: " << DLL_RESOURCE_NAME << "\n";
            return "";
        }

        LPVOID pData = LockResource(hData);
        if (!pData)
        {
            std::cerr << "[!] Failed to lock resource: " << DLL_RESOURCE_NAME << "\n";
            return "";
        }

        DWORD dwSize = SizeofResource(hModule, hResource);
        char tempPath[MAX_PATH];
        GetTempPathA(MAX_PATH, tempPath);

        char dllPath[MAX_PATH];
        strcpy_s(dllPath, MAX_PATH, tempPath);
        // Create unique filename with current process ID to avoid conflicts between trainer instances
        char uniqueName[64];
        snprintf(uniqueName, sizeof(uniqueName), "IL2CPP_%lu.dll", GetCurrentProcessId());
        strcat_s(dllPath, MAX_PATH, uniqueName);

        HANDLE hFile = CreateFileA(dllPath, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (hFile == INVALID_HANDLE_VALUE)
        {
            std::cerr << "[!] Failed to create DLL file: " << dllPath << "\n";
            return "";
        }

        DWORD bytesWritten;
        WriteFile(hFile, pData, dwSize, &bytesWritten, nullptr);
        CloseHandle(hFile);

        if (bytesWritten != dwSize)
        {
            std::cerr << "[!] Failed to write DLL resource to file.\n";
            DeleteFileA(dllPath);
            return "";
        }

        std::cout << "[+] DLL extracted to: " << dllPath << std::endl;
        return std::string(dllPath);
    }

    /** Injects the DLL into the target process and retrieves its base address */
    bool initializeDllInjection()
    {
        if (hInjectedDll != NULL)
            return true;

        // 1. Extract DLL to temp location
        if (injectedDllPath.empty())
        {
            injectedDllPath = extractDllFromResource();
            if (injectedDllPath.empty())
            {
                std::cerr << "[!] Failed to extract DLL from resources.\n";
                return false;
            }
        }

        // 2. Allocate memory in target process for DLL path
        size_t pathLength = injectedDllPath.length() + 1;
        LPVOID pRemotePath = VirtualAllocEx(hProcess, nullptr, pathLength, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (!pRemotePath)
        {
            std::cerr << "[!] Failed to allocate memory in target process.\n";
            return false;
        }

        // 3. Write DLL path to remote memory
        if (!WriteProcessMemory(hProcess, pRemotePath, injectedDllPath.c_str(), pathLength, nullptr))
        {
            std::cerr << "[!] Failed to write DLL path to target process.\n";
            VirtualFreeEx(hProcess, pRemotePath, 0, MEM_RELEASE);
            return false;
        }

        // 4. Get LoadLibraryA address
        HMODULE kernel32 = GetModuleHandleA("kernel32.dll");
        FARPROC loadLibraryA = GetProcAddress(kernel32, "LoadLibraryA");
        if (!loadLibraryA)
        {
            std::cerr << "[!] Failed to get LoadLibraryA address.\n";
            VirtualFreeEx(hProcess, pRemotePath, 0, MEM_RELEASE);
            return false;
        }

        // 5. Create remote thread to load DLL
        HANDLE hRemoteThread = CreateRemoteThread(
            hProcess,
            nullptr,
            0,
            (LPTHREAD_START_ROUTINE)loadLibraryA,
            pRemotePath,
            0,
            nullptr);

        if (!hRemoteThread)
        {
            std::cerr << "[!] Failed to create remote thread.\n";
            VirtualFreeEx(hProcess, pRemotePath, 0, MEM_RELEASE);
            return false;
        }

        // 6. Wait for remote thread
        DWORD waitResult = WaitForSingleObject(hRemoteThread, 5000);
        if (waitResult == WAIT_TIMEOUT)
        {
            std::cerr << "[!] Remote thread timed out.\n";
            TerminateThread(hRemoteThread, 0);
            CloseHandle(hRemoteThread);
            VirtualFreeEx(hProcess, pRemotePath, 0, MEM_RELEASE);
            return false;
        }

        // 7. Get module base from exit code
        DWORD exitCode = 0;
        GetExitCodeThread(hRemoteThread, &exitCode);
        remoteModuleBase = static_cast<uintptr_t>(exitCode);

        CloseHandle(hRemoteThread);
        VirtualFreeEx(hProcess, pRemotePath, 0, MEM_RELEASE);

        // For 64-bit processes, the exit code is truncated, so we need to find the actual module base
        // Use EnumProcessModules with a short timeout to find the injected DLL
        remoteModuleBase = 0;
        std::string dllName = injectedDllPath.substr(injectedDllPath.find_last_of("\\/") + 1);

        // Poll for module with timeout
        DWORD pollStart = GetTickCount();
        const DWORD POLL_TIMEOUT = 2000; // 2 second timeout

        while (!remoteModuleBase && (GetTickCount() - pollStart) < POLL_TIMEOUT)
        {
            HMODULE modules[1024];
            DWORD needed = 0;
            if (EnumProcessModules(hProcess, modules, sizeof(modules), &needed))
            {
                int numModules = needed / sizeof(HMODULE);
                for (int i = 0; i < numModules; i++)
                {
                    char modulePath[MAX_PATH];
                    if (GetModuleFileNameExA(hProcess, modules[i], modulePath, MAX_PATH))
                    {
                        std::string modPath(modulePath);
                        std::string modName = modPath.substr(modPath.find_last_of("\\/") + 1);
                        if (modName == dllName)
                        {
                            remoteModuleBase = reinterpret_cast<uintptr_t>(modules[i]);
                            break;
                        }
                    }
                }
            }

            if (!remoteModuleBase)
                Sleep(50); // Small delay before retry
        }

        if (!remoteModuleBase)
        {
            std::cerr << "[!] Failed to find injected DLL in target process.\n";
            return false;
        }

        // 8. Load DLL locally for offset calculations
        HMODULE hLocalDll = LoadLibraryA(injectedDllPath.c_str());
        if (!hLocalDll)
        {
            std::cerr << "[!] Failed to load DLL locally.\n";
            return false;
        }

        localModuleBase = reinterpret_cast<uintptr_t>(hLocalDll);
        hInjectedDll = hLocalDll;

        std::cout << "[+] DLL injected successfully.\n";
        std::cout << "[+] Remote base: 0x" << std::hex << remoteModuleBase << std::dec << "\n";

        // 9. Initialize response communication channel
        if (!initializeResponseChannel())
        {
            std::cerr << "[!] Failed to initialize response communication channel.\n";
            return false;
        }

        return true;
    }

    /** Initialize shared memory and event for receiving responses from injected DLL */
    bool initializeResponseChannel()
    {
        std::string responseSharedMemName = "IL2CppResponseSharedMemory_" + std::to_string(procId);
        hResponseMapFile = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, responseBufferSize, responseSharedMemName.c_str());
        if (hResponseMapFile == NULL)
        {
            std::cerr << "[!] Failed to create response file mapping: " << GetLastError() << "\n";
            return false;
        }

        responseBuffer = MapViewOfFile(hResponseMapFile, FILE_MAP_ALL_ACCESS, 0, 0, 0);
        if (responseBuffer == NULL)
        {
            std::cerr << "[!] Failed to map response buffer: " << GetLastError() << "\n";
            CloseHandle(hResponseMapFile);
            hResponseMapFile = NULL;
            return false;
        }

        std::string eventName = "IL2CppResponseEvent_" + std::to_string(procId);
        hResponseEvent = CreateEventA(NULL, FALSE, FALSE, eventName.c_str());
        if (hResponseEvent == NULL)
        {
            std::cerr << "[!] Failed to create response event: " << GetLastError() << "\n";
            UnmapViewOfFile(responseBuffer);
            CloseHandle(hResponseMapFile);
            hResponseMapFile = NULL;
            responseBuffer = NULL;
            return false;
        }

        std::cout << "[+] Response communication channel initialized.\n";
        return true;
    }

    void unloadDll()
    {
        if (hInjectedDll)
        {
            hInjectedDll = NULL;
        }
        if (remoteArgMemory)
        {
            VirtualFreeEx(hProcess, remoteArgMemory, 0, MEM_RELEASE);
            remoteArgMemory = nullptr;
        }
        if (!injectedDllPath.empty())
            DeleteFileA(injectedDllPath.c_str());
    }

    /** Calls an exported function in the injected DLL with no arguments */
    bool invokeMethod(const char *functionName)
    {
        char dummy = 0;
        return invokeMethod<char>(functionName, dummy);
    }

    /** Calls an exported function in the injected DLL with the given arguments */
    template <typename T>
    bool invokeMethod(const char *functionName, T args)
    {
        if (!hInjectedDll)
        {
            std::cerr << "[!] DLL not injected.\n";
            return false;
        }

        if (!localModuleBase)
        {
            std::cerr << "[!] Local module base not set.\n";
            return false;
        }

        // Get function address from locally loaded DLL
        FARPROC localFunc = GetProcAddress(hInjectedDll, functionName);
        if (!localFunc)
        {
            std::cerr << "[!] Function '" << functionName << "' not found.\n";
            return false;
        }

        // Calculate offset from local DLL base
        uintptr_t offset = reinterpret_cast<uintptr_t>(localFunc) - localModuleBase;

        // Calculate remote function address
        uintptr_t remoteFuncAddr = remoteModuleBase + offset;

        // Allocate memory in remote process for arguments
        if (!remoteArgMemory)
        {
            size_t allocSize = sizeof(T) + 16;
            remoteArgMemory = VirtualAllocEx(hProcess, nullptr, allocSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
            if (!remoteArgMemory)
            {
                std::cerr << "[!] Failed to allocate memory in target process.\n";
                return false;
            }
        }

        // Write arguments to remote memory
        if (!WriteProcessMemory(hProcess, remoteArgMemory, &args, sizeof(T), nullptr))
        {
            std::cerr << "[!] Failed to write arguments to remote process.\n";
            return false;
        }

        // Create remote thread to execute function
        HANDLE hThread = CreateRemoteThread(
            hProcess,
            nullptr,
            0,
            (LPTHREAD_START_ROUTINE)remoteFuncAddr,
            remoteArgMemory,
            0,
            nullptr);

        if (!hThread)
        {
            std::cerr << "[!] Failed to create remote thread.\n";
            return false;
        }

        // Wait for execution with timeout
        DWORD waitResult = WaitForSingleObject(hThread, 5000);
        if (waitResult == WAIT_TIMEOUT)
        {
            std::cerr << "[!] Remote function timed out.\n";
            TerminateThread(hThread, 0);
            CloseHandle(hThread);
            return false;
        }
        else if (waitResult != WAIT_OBJECT_0)
        {
            std::cerr << "[!] WaitForSingleObject failed.\n";
            CloseHandle(hThread);
            return false;
        }

        DWORD exitCode = 0;
        GetExitCodeThread(hThread, &exitCode);
        CloseHandle(hThread);

        std::cout << "[+] Function invoked successfully.\n";
        return exitCode == 1;
    }

    /** Invoke an exported function and wait for a string response from the DLL */
    std::string invokeMethodReturn(const char *functionName)
    {
        if (!hInjectedDll)
        {
            std::cerr << "[!] DLL not injected.\n";
            return "";
        }

        if (!hResponseEvent)
        {
            std::cerr << "[!] Response channel not initialized.\n";
            return "";
        }

        // Get function address from locally loaded DLL
        FARPROC localFunc = GetProcAddress(hInjectedDll, functionName);
        if (!localFunc)
        {
            std::cerr << "[!] Function '" << functionName << "' not found.\n";
            return "";
        }

        // Calculate offset from local DLL base
        uintptr_t offset = reinterpret_cast<uintptr_t>(localFunc) - localModuleBase;

        // Calculate remote function address
        uintptr_t remoteFuncAddr = remoteModuleBase + offset;

        // Create remote thread to execute function (no arguments needed for response functions)
        HANDLE hThread = CreateRemoteThread(
            hProcess,
            nullptr,
            0,
            (LPTHREAD_START_ROUTINE)remoteFuncAddr,
            nullptr,
            0,
            nullptr);

        if (!hThread)
        {
            std::cerr << "[!] Failed to create remote thread.\n";
            return "";
        }

        // Wait for thread execution
        WaitForSingleObject(hThread, INFINITE);
        CloseHandle(hThread);

        // Wait for the response event with a timeout
        DWORD waitResult = WaitForSingleObject(hResponseEvent, 5000); // 5 seconds timeout
        if (waitResult == WAIT_TIMEOUT)
        {
            std::cerr << "[!] Response timeout after 5 seconds.\n";
            return "";
        }
        else if (waitResult == WAIT_FAILED)
        {
            std::cerr << "[!] WaitForSingleObject failed: " << GetLastError() << "\n";
            return "";
        }

        // Read the response from shared memory
        size_t length = *reinterpret_cast<size_t *>(responseBuffer);
        if (length >= responseBufferSize - sizeof(size_t))
        {
            std::cerr << "[!] Response length exceeds buffer size.\n";
            return "";
        }

        char *msgPtr = static_cast<char *>(responseBuffer) + sizeof(size_t);
        std::string response(msgPtr, length);
        return response;
    }
};