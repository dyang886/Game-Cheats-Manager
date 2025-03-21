// MonoBase.h
#pragma once

#include "TrainerBase.h"
#include <string>
#include <windows.h>

// For logging
#include <stdio.h> // For sprintf and printf

class MonoBase : public TrainerBase
{
public:
    MonoBase(const std::wstring &processIdentifier, bool useWindowTitle = false)
        : TrainerBase(processIdentifier, useWindowTitle),
          bridgeDllPath(L"D:/Resources/Software/Game Trainers/build/bin/Debug/MonoBridge.dll")
    {
        // Log initialization
        printf("MonoBase initialized.\n");
    }

    virtual ~MonoBase()
    {
        printf("MonoBase destructor called.\n");
        // Cleanup handled by TrainerBase::~TrainerBase via cleanUp()
    }

    /** Injects the C++ bridge DLL into the target process.
     * @param dllPath Path to MonoBridge.dll
     * @return True if injection succeeds, false otherwise
     */
    bool injectBridgeDLL(const std::wstring &dllPath)
    {
        char logMsg[256];

        if (!hProcess || !isProcessRunning())
        {
            printf("Error: Process not running or handle invalid.\n");
            return false;
        }

        sprintf(logMsg, "Attempting to inject DLL: %S\n", dllPath.c_str());
        printf("%s", logMsg);

        size_t pathSize = (dllPath.size() + 1) * sizeof(wchar_t);
        LPVOID allocMem = VirtualAllocEx(hProcess, nullptr, pathSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (!allocMem)
        {
            printf("Error: Failed to allocate memory in target process. Error code: %lu\n", GetLastError());
            return false;
        }

        if (!WriteProcessMemory(hProcess, allocMem, dllPath.c_str(), pathSize, nullptr))
        {
            printf("Error: Failed to write DLL path to target process. Error code: %lu\n", GetLastError());
            VirtualFreeEx(hProcess, allocMem, 0, MEM_RELEASE);
            return false;
        }

        HMODULE kernel32 = GetModuleHandleW(L"kernel32.dll");
        if (!kernel32)
        {
            printf("Error: Failed to get handle to kernel32.dll.\n");
            VirtualFreeEx(hProcess, allocMem, 0, MEM_RELEASE);
            return false;
        }

        LPVOID loadLibraryAddr = reinterpret_cast<LPVOID>(GetProcAddress(kernel32, "LoadLibraryW"));
        if (!loadLibraryAddr)
        {
            printf("Error: Failed to get address of LoadLibraryW.\n");
            VirtualFreeEx(hProcess, allocMem, 0, MEM_RELEASE);
            return false;
        }

        HANDLE hThread = CreateRemoteThread(hProcess, nullptr, 0,
                                            reinterpret_cast<LPTHREAD_START_ROUTINE>(loadLibraryAddr),
                                            allocMem, 0, nullptr);
        if (!hThread)
        {
            printf("Error: Failed to create remote thread for LoadLibraryW. Error code: %lu\n", GetLastError());
            VirtualFreeEx(hProcess, allocMem, 0, MEM_RELEASE);
            return false;
        }

        WaitForSingleObject(hThread, INFINITE);
        DWORD exitCode = 0;
        GetExitCodeThread(hThread, &exitCode);
        CloseHandle(hThread);

        if (exitCode == 0)
        {
            // Allocate space to retrieve remote error code
            LPVOID errorMem = VirtualAllocEx(hProcess, nullptr, sizeof(DWORD), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
            if (errorMem)
            {
                LPVOID getLastErrorAddr = reinterpret_cast<LPVOID>(GetProcAddress(kernel32, "GetLastError"));
                if (getLastErrorAddr)
                {
                    HANDLE errorThread = CreateRemoteThread(hProcess, nullptr, 0,
                                                            reinterpret_cast<LPTHREAD_START_ROUTINE>(getLastErrorAddr),
                                                            nullptr, 0, nullptr);
                    if (errorThread)
                    {
                        WaitForSingleObject(errorThread, INFINITE);
                        DWORD remoteError = 0;
                        GetExitCodeThread(errorThread, &remoteError);
                        CloseHandle(errorThread);
                        sprintf(logMsg, "Error: LoadLibraryW failed in target process. Remote error code: %lu\n", remoteError);
                        printf("%s", logMsg);
                    }
                }
                VirtualFreeEx(hProcess, errorMem, 0, MEM_RELEASE);
            }
            else
            {
                printf("Error: Failed to allocate memory for error code retrieval.\n");
            }
            VirtualFreeEx(hProcess, allocMem, 0, MEM_RELEASE);
            return false;
        }

        bridgeDllBase = static_cast<uintptr_t>(exitCode);
        VirtualFreeEx(hProcess, allocMem, 0, MEM_RELEASE);
        sprintf(logMsg, "Bridge DLL injected successfully at base: 0x%IX.\n", bridgeDllBase);
        printf("%s", logMsg);
        return true;
    }

    /** Retrieves function pointers from the injected bridge DLL.
     * @return True if pointers are successfully retrieved, false otherwise
     */
    bool getFunctionPointers()
    {
        char logMsg[256];

        if (!hProcess || !bridgeDllBase)
        {
            printf("Error: Process handle invalid or bridge DLL not injected.\n");
            return false;
        }

        printf("Attempting to retrieve function pointers.\n");

        HMODULE hLocalDll = LoadLibraryW(bridgeDllPath.c_str());
        if (!hLocalDll)
        {
            printf("Error: Failed to load bridge DLL locally.\n");
            return false;
        }

        FARPROC localGetFuncPtrs = GetProcAddress(hLocalDll, "GetFunctionPointersThread");
        if (!localGetFuncPtrs)
        {
            printf("Error: Failed to get address of GetFunctionPointersThread.\n");
            FreeLibrary(hLocalDll);
            return false;
        }

        uintptr_t localBase = reinterpret_cast<uintptr_t>(hLocalDll);
        uintptr_t localAddr = reinterpret_cast<uintptr_t>(localGetFuncPtrs);
        uintptr_t rva = localAddr - localBase;
        LPVOID remoteGetFuncPtrs = reinterpret_cast<LPVOID>(bridgeDllBase + rva);

        LPVOID allocMem = VirtualAllocEx(hProcess, nullptr, sizeof(FunctionPointers),
                                         MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (!allocMem)
        {
            printf("Error: Failed to allocate memory for function pointers.\n");
            FreeLibrary(hLocalDll);
            return false;
        }

        HANDLE hThread = CreateRemoteThread(hProcess, nullptr, 0,
                                            reinterpret_cast<LPTHREAD_START_ROUTINE>(remoteGetFuncPtrs),
                                            allocMem, 0, nullptr);
        if (!hThread)
        {
            printf("Error: Failed to create remote thread for function pointers.\n");
            VirtualFreeEx(hProcess, allocMem, 0, MEM_RELEASE);
            FreeLibrary(hLocalDll);
            return false;
        }

        WaitForSingleObject(hThread, INFINITE);
        CloseHandle(hThread);

        if (!ReadProcessMemory(hProcess, allocMem, &funcPtrs, sizeof(FunctionPointers), nullptr))
        {
            printf("Error: Failed to read function pointers from target process.\n");
            VirtualFreeEx(hProcess, allocMem, 0, MEM_RELEASE);
            FreeLibrary(hLocalDll);
            return false;
        }

        VirtualFreeEx(hProcess, allocMem, 0, MEM_RELEASE);
        FreeLibrary(hLocalDll);
        printf("Function pointers retrieved successfully.\n");
        return true;
    }

    /** Loads a C# assembly into the Mono runtime via the bridge DLL.
     * @param assemblyPath Path to the C# DLL (e.g., "GCMInjection.dll")
     * @return True if loading succeeds, false otherwise
     */
    bool loadAssembly(const std::wstring &assemblyPath)
    {
        char logMsg[256];

        // Validate process handle and function pointer
        if (!hProcess || !funcPtrs.LoadAssemblyThread)
        {
            printf("Error: Process handle invalid or LoadAssemblyThread pointer missing.\n");
            return false;
        }

        // Log the attempt
        sprintf(logMsg, "Attempting to load assembly: %S\n", assemblyPath.c_str());
        printf("%s", logMsg);

        // Convert std::wstring to std::string using WideCharToMultiByte
        int len = WideCharToMultiByte(CP_ACP, 0, assemblyPath.c_str(), -1, nullptr, 0, nullptr, nullptr);
        if (len == 0)
        {
            printf("Error: Failed to convert assembly path to multi-byte string.\n");
            return false;
        }

        std::string pathStr(len, '\0');
        WideCharToMultiByte(CP_ACP, 0, assemblyPath.c_str(), -1, &pathStr[0], len, nullptr, nullptr);
        pathStr.resize(len - 1); // Remove the null terminator from the string

        // Allocate memory in the target process
        size_t pathLen = pathStr.size() + 1;
        size_t paramsSize = sizeof(LoadAssemblyParams) + pathLen;

        LPVOID allocMem = VirtualAllocEx(hProcess, nullptr, paramsSize,
                                         MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (!allocMem)
        {
            printf("Error: Failed to allocate memory for assembly path.\n");
            return false;
        }

        // Write the path to the allocated memory
        LPVOID pathMem = reinterpret_cast<LPVOID>(reinterpret_cast<uintptr_t>(allocMem) + sizeof(LoadAssemblyParams));
        if (!WriteProcessMemory(hProcess, pathMem, pathStr.c_str(), pathLen, nullptr))
        {
            printf("Error: Failed to write assembly path to target process.\n");
            VirtualFreeEx(hProcess, allocMem, 0, MEM_RELEASE);
            return false;
        }

        // Set up parameters and write them
        LoadAssemblyParams params;
        params.path = reinterpret_cast<const char *>(pathMem);
        if (!WriteProcessMemory(hProcess, allocMem, &params, sizeof(LoadAssemblyParams), nullptr))
        {
            printf("Error: Failed to write LoadAssemblyParams to target process.\n");
            VirtualFreeEx(hProcess, allocMem, 0, MEM_RELEASE);
            return false;
        }

        // Create a remote thread to load the assembly
        HANDLE hThread = CreateRemoteThread(hProcess, nullptr, 0,
                                            reinterpret_cast<LPTHREAD_START_ROUTINE>(funcPtrs.LoadAssemblyThread),
                                            allocMem, 0, nullptr);
        if (!hThread)
        {
            printf("Error: Failed to create remote thread to load assembly.\n");
            VirtualFreeEx(hProcess, allocMem, 0, MEM_RELEASE);
            return false;
        }

        // Wait for the thread to complete and clean up
        WaitForSingleObject(hThread, INFINITE);
        CloseHandle(hThread);
        VirtualFreeEx(hProcess, allocMem, 0, MEM_RELEASE);
        printf("Assembly load thread completed.\n");
        return true;
    }

    /** Invokes a static method in the loaded C# assembly.
     * @param className Fully qualified class name (e.g., "GCMInjection")
     * @param methodName Method name (e.g., "SpawnItem")
     * @param param Integer parameter to pass to the method
     * @return True if invocation succeeds, false otherwise
     */
    bool invokeMethod(const std::string &className, const std::string &methodName, int param)
    {
        char logMsg[256];

        if (!hProcess || !funcPtrs.InvokeMethodThread)
        {
            printf("Error: Process handle invalid or InvokeMethodThread pointer missing.\n");
            return false;
        }

        sprintf(logMsg, "Attempting to invoke %s.%s with param %d\n", className.c_str(), methodName.c_str(), param);
        printf("%s", logMsg);

        size_t classNameLen = className.size() + 1;
        size_t methodNameLen = methodName.size() + 1;
        size_t paramsSize = sizeof(InvokeMethodParams) + classNameLen + methodNameLen;

        LPVOID allocMem = VirtualAllocEx(hProcess, nullptr, paramsSize,
                                         MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (!allocMem)
        {
            printf("Error: Failed to allocate memory for method invocation.\n");
            return false;
        }

        LPVOID classNameMem = reinterpret_cast<LPVOID>(reinterpret_cast<uintptr_t>(allocMem) + sizeof(InvokeMethodParams));
        if (!WriteProcessMemory(hProcess, classNameMem, className.c_str(), classNameLen, nullptr))
        {
            printf("Error: Failed to write class name to target process.\n");
            VirtualFreeEx(hProcess, allocMem, 0, MEM_RELEASE);
            return false;
        }

        LPVOID methodNameMem = reinterpret_cast<LPVOID>(reinterpret_cast<uintptr_t>(classNameMem) + classNameLen);
        if (!WriteProcessMemory(hProcess, methodNameMem, methodName.c_str(), methodNameLen, nullptr))
        {
            printf("Error: Failed to write method name to target process.\n");
            VirtualFreeEx(hProcess, allocMem, 0, MEM_RELEASE);
            return false;
        }

        InvokeMethodParams params;
        params.className = reinterpret_cast<const char *>(classNameMem);
        params.methodName = reinterpret_cast<const char *>(methodNameMem);
        params.param = param;
        if (!WriteProcessMemory(hProcess, allocMem, &params, sizeof(InvokeMethodParams), nullptr))
        {
            printf("Error: Failed to write InvokeMethodParams to target process.\n");
            VirtualFreeEx(hProcess, allocMem, 0, MEM_RELEASE);
            return false;
        }

        HANDLE hThread = CreateRemoteThread(hProcess, nullptr, 0,
                                            reinterpret_cast<LPTHREAD_START_ROUTINE>(funcPtrs.InvokeMethodThread),
                                            allocMem, 0, nullptr);
        if (!hThread)
        {
            printf("Error: Failed to create remote thread to invoke method.\n");
            VirtualFreeEx(hProcess, allocMem, 0, MEM_RELEASE);
            return false;
        }

        WaitForSingleObject(hThread, INFINITE);
        CloseHandle(hThread);
        VirtualFreeEx(hProcess, allocMem, 0, MEM_RELEASE);
        printf("Method invocation thread completed.\n");
        return true;
    }

private:
    std::wstring bridgeDllPath;
    uintptr_t bridgeDllBase = 0;

    struct FunctionPointers
    {
        LPVOID LoadAssemblyThread;
        LPVOID InvokeMethodThread;
    } funcPtrs = {nullptr, nullptr};

    struct LoadAssemblyParams
    {
        const char *path;
    };

    struct InvokeMethodParams
    {
        const char *className;
        const char *methodName;
        int param;
    };
};