/**
 * ==============================================================================
 * IL2CPP BASE (Il2CppBase.h)
 * ==============================================================================
 *
 * This is the CLIENT-SIDE base class for an IL2CPP trainer.
 * It is structured just like your MonoBase.h.
 *
 * It handles:
 * 1. Extracting Il2CppBridge.dll from resources.
 * 2. Injecting the DLL into the game process.
 * 3. Calling exported functions from the injected DLL.
 *
 * This file should NOT include any Aetherim headers.
 */
#pragma once

#include "TrainerBase.h" // Your existing base class
#include <FL/Fl.H>       // For Fl::repeat_timeout
#include <mutex>
#include <shlwapi.h>
#include <sstream>
#include <variant>

// --- Contract Structs ---
// These structs must be defined *identically* in Il2CppBridge.cpp
// This is the "contract" between the client (.exe) and server (.dll).

// Defines the function pointers we will get from the remote DLL.
struct Bridge_FunctionPointers
{
    LPVOID pBridge_Initialize;
    LPVOID pBridge_InvokeMethod;
};

// Defines the layout of the parameters we will write into the target process.
struct Bridge_InvokeMethod_Params
{
    const char *imageName;
    const char *namespaceName;
    const char *className;
    const char *methodName;
    int argCount;
    void **args; // This points to an array of pointers
};

// --- Main Client-Side Base Class ---
class Il2CppBase : public TrainerBase
{
public:
    // This is the C++ equivalent of your MonoBase::Param
    using Param = std::variant<int, float, std::string>;

    Il2CppBase(const std::wstring &processIdentifier, bool useWindowTitle = false)
        : TrainerBase(processIdentifier, useWindowTitle)
    {
    }

    virtual ~Il2CppBase()
    {
        cleanUp();
    }

    void cleanUp() override
    {
        TrainerBase::cleanUp(); // Call parent cleanup

        if (!bridgeDllPath.empty())
        {
            DeleteFileW(bridgeDllPath.c_str());
            // Remove the subdirectory
            WCHAR dirPath[MAX_PATH];
            wcscpy_s(dirPath, MAX_PATH, bridgeDllPath.c_str());
            PathRemoveFileSpecW(dirPath);
            RemoveDirectoryW(dirPath);
            bridgeDllPath.clear();
        }

        dllInjected = false;
        bridgeDllBase = 0;
        funcPtrs.Initialize = nullptr;
        funcPtrs.InvokeMethod = nullptr;
    }

    /**
     * @brief Main initialization function, just like in MonoBase.
     * @return True on success, false on failure.
     */
    bool initializeDllInjection()
    {
        if (dllInjected)
            return true;

        // Ensure process is running first
        if (!isProcessRunning())
            return false;

        if (!injectBridgeDLL())
        {
            std::cerr << "[!] Failed to inject IL2CPP Bridge DLL." << std::endl;
            return false;
        }

        if (!getFunctionPointers())
        {
            std::cerr << "[!] Failed to get function pointers from bridge." << std::endl;
            return false;
        }

        // Call the remote Initialize function
        HANDLE hThread = CreateRemoteThread(hProcess, nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(funcPtrs.Initialize), nullptr, 0, nullptr);
        if (!hThread)
        {
            std::cerr << "[!] Failed to create remote thread for Bridge_Initialize.\n";
            return false;
        }
        WaitForSingleObject(hThread, INFINITE);
        DWORD exitCode = 0;
        GetExitCodeThread(hThread, &exitCode);
        CloseHandle(hThread);

        if (exitCode == 0)
        {
            std::cerr << "[!] Bridge_Initialize failed in target process.\n";
            return false;
        }

        std::cout << "[+] IL2CPP Bridge successfully initialized." << std::endl;
        dllInjected = true;
        return true;
    }

protected:
    /**
     * @brief The core generic invoke function.
     * This allocates memory, writes all parameters, and calls the bridge.
     */
    void *invokeMethod(const std::string &imageName, const std::string &namespaceName, const std::string &className, const std::string &methodName, const std::vector<Param> &params)
    {
        if (!hProcess || !funcPtrs.InvokeMethod)
        {
            std::cerr << "[!] Process handle invalid or InvokeMethod pointer missing.\n";
            return nullptr;
        }

        // --- Calculate total size needed for all data ---
        size_t paramCount = params.size();
        size_t totalSize = 0;
        totalSize += sizeof(Bridge_InvokeMethod_Params); // The main struct
        totalSize += sizeof(void *) * paramCount;        // The void** args array
        totalSize += imageName.size() + 1;
        totalSize += namespaceName.size() + 1;
        totalSize += className.size() + 1;
        totalSize += methodName.size() + 1;

        std::vector<size_t> stringParamSizes;
        for (const auto &p : params)
        {
            if (std::holds_alternative<std::string>(p))
            {
                size_t len = std::get<std::string>(p).size() + 1;
                totalSize += len;
                stringParamSizes.push_back(len);
            }
            else if (std::holds_alternative<int>(p))
            {
                totalSize += sizeof(int);
            }
            else if (std::holds_alternative<float>(p))
            {
                totalSize += sizeof(float);
            }
        }

        // --- Allocate remote memory ---
        LPVOID allocMem = VirtualAllocEx(hProcess, nullptr, totalSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (!allocMem)
        {
            std::cerr << "[!] Failed to allocate memory for method invocation.\n";
            return nullptr;
        }

        // --- Write all data sequentially ---
        Bridge_InvokeMethod_Params remoteParams{};       // This is the local "map"
        std::vector<void *> remoteArgsArray(paramCount); // Local map for the void** array
        uintptr_t currentAddr = (uintptr_t)allocMem;

        // 1. Reserve space for the main struct and the args array
        LPVOID remoteArgsArrayAddr = (LPVOID)(currentAddr + sizeof(Bridge_InvokeMethod_Params));
        remoteParams.args = (void **)remoteArgsArrayAddr;
        currentAddr += sizeof(Bridge_InvokeMethod_Params);
        currentAddr += sizeof(void *) * paramCount;

        // 2. Write strings (Image, Namespace, Class, Method)
        remoteParams.imageName = (const char *)currentAddr;
        WriteProcessMemory(hProcess, (LPVOID)currentAddr, imageName.c_str(), imageName.size() + 1, nullptr);
        currentAddr += imageName.size() + 1;

        remoteParams.namespaceName = (const char *)currentAddr;
        WriteProcessMemory(hProcess, (LPVOID)currentAddr, namespaceName.c_str(), namespaceName.size() + 1, nullptr);
        currentAddr += namespaceName.size() + 1;

        remoteParams.className = (const char *)currentAddr;
        WriteProcessMemory(hProcess, (LPVOID)currentAddr, className.c_str(), className.size() + 1, nullptr);
        currentAddr += className.size() + 1;

        remoteParams.methodName = (const char *)currentAddr;
        WriteProcessMemory(hProcess, (LPVOID)currentAddr, methodName.c_str(), methodName.size() + 1, nullptr);
        currentAddr += methodName.size() + 1;

        // 3. Write all parameters and fill the remote args array
        remoteParams.argCount = (int)paramCount;
        int stringIndex = 0;
        for (size_t i = 0; i < paramCount; ++i)
        {
            if (std::holds_alternative<int>(params[i]))
            {
                int val = std::get<int>(params[i]);
                remoteArgsArray[i] = (void *)currentAddr; // Pointer to the int
                WriteProcessMemory(hProcess, (LPVOID)currentAddr, &val, sizeof(int), nullptr);
                currentAddr += sizeof(int);
            }
            else if (std::holds_alternative<float>(params[i]))
            {
                float val = std::get<float>(params[i]);
                remoteArgsArray[i] = (void *)currentAddr; // Pointer to the float
                WriteProcessMemory(hProcess, (LPVOID)currentAddr, &val, sizeof(float), nullptr);
                currentAddr += sizeof(float);
            }
            else if (std::holds_alternative<std::string>(params[i]))
            {
                const std::string &val = std::get<std::string>(params[i]);
                remoteArgsArray[i] = (void *)currentAddr; // Pointer to the string
                WriteProcessMemory(hProcess, (LPVOID)currentAddr, val.c_str(), val.size() + 1, nullptr);
                currentAddr += val.size() + 1;
            }
        }

        // 4. Write the main params struct and the args array
        WriteProcessMemory(hProcess, allocMem, &remoteParams, sizeof(Bridge_InvokeMethod_Params), nullptr);
        WriteProcessMemory(hProcess, remoteArgsArrayAddr, remoteArgsArray.data(), sizeof(void *) * paramCount, nullptr);

        // 5. Call the remote method
        HANDLE hThread = CreateRemoteThread(hProcess, nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(funcPtrs.InvokeMethod), allocMem, 0, nullptr);
        if (!hThread)
        {
            std::cerr << "[!] Failed to create remote thread for method invocation. GetLastError() = " << GetLastError() << "\n";
            VirtualFreeEx(hProcess, allocMem, 0, MEM_RELEASE);
            return nullptr;
        }

        WaitForSingleObject(hThread, INFINITE);
        DWORD exitCode = 0; // This will be the void* return
        GetExitCodeThread(hThread, &exitCode);
        CloseHandle(hThread);

        VirtualFreeEx(hProcess, allocMem, 0, MEM_RELEASE);

        return (void *)exitCode;
    }

private:
    bool dllInjected = false;
    std::wstring bridgeDllPath;
    uintptr_t bridgeDllBase = 0;

    struct FunctionPointers
    {
        LPVOID Initialize = nullptr;
        LPVOID InvokeMethod = nullptr;
    } funcPtrs;

    /**
     * @brief Injects the C++ bridge DLL into the target process from embedded resources
     * (Logic from your MonoBase.h)
     */
    bool injectBridgeDLL()
    {
        // Use a resource name your RC file will define, e.g., "IL2CPPBRIDGE_DLL"
        bridgeDllPath = extractResourceToTempFile("IL2CPPBRIDGE_DLL", L"Il2CppBridge.dll");
        if (bridgeDllPath.empty())
        {
            std::cerr << "[!] Failed to extract Il2CppBridge.dll from resources.\n";
            return false;
        }

        if (!hProcess || !isProcessRunning())
        {
            std::cerr << "[!] Process not running or handle invalid.\n";
            return false;
        }

        size_t pathSize = (bridgeDllPath.size() + 1) * sizeof(wchar_t);
        LPVOID allocMem = VirtualAllocEx(hProcess, nullptr, pathSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (!allocMem)
        {
            std::cerr << "[!] Failed to allocate memory in target process.\n";
            return false;
        }

        if (!WriteProcessMemory(hProcess, allocMem, bridgeDllPath.c_str(), pathSize, nullptr))
        {
            std::cerr << "[!] Failed to write DLL path to target process.\n";
            VirtualFreeEx(hProcess, allocMem, 0, MEM_RELEASE);
            return false;
        }

        HMODULE kernel32 = GetModuleHandleW(L"kernel32.dll");
        LPVOID loadLibraryAddr = kernel32 ? reinterpret_cast<LPVOID>(GetProcAddress(kernel32, "LoadLibraryW")) : nullptr;
        if (!loadLibraryAddr)
        {
            std::cerr << "[!] Failed to get LoadLibraryW address.\n";
            VirtualFreeEx(hProcess, allocMem, 0, MEM_RELEASE);
            return false;
        }

        HANDLE hThread = CreateRemoteThread(hProcess, nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(loadLibraryAddr), allocMem, 0, nullptr);
        if (!hThread)
        {
            std::cerr << "[!] Failed to create remote thread for LoadLibraryW.\n";
            VirtualFreeEx(hProcess, allocMem, 0, MEM_RELEASE);
            return false;
        }

        WaitForSingleObject(hThread, INFINITE);
        DWORD exitCode = 0;
        GetExitCodeThread(hThread, &exitCode);
        CloseHandle(hThread);

        if (exitCode == 0)
        {
            std::cerr << "[!] LoadLibraryW failed in target process.\n";
            VirtualFreeEx(hProcess, allocMem, 0, MEM_RELEASE);
            return false;
        }

        bridgeDllBase = static_cast<uintptr_t>(exitCode);
        VirtualFreeEx(hProcess, allocMem, 0, MEM_RELEASE);
        return true;
    }

    /**
     * @brief Retrieves function pointers from the injected bridge DLL
     * (Logic from your MonoBase.h)
     */
    bool getFunctionPointers()
    {
        if (!hProcess || !bridgeDllBase || bridgeDllPath.empty())
        {
            std::cerr << "[!] Process handle invalid or bridge DLL not injected.\n";
            return false;
        }

        // 1. Load DLL locally to get RVA of Bridge_GetFunctionPointers
        HMODULE hLocalDll = LoadLibraryW(bridgeDllPath.c_str());
        if (!hLocalDll)
        {
            std::cerr << "[!] Failed to load bridge DLL locally from " << wstringToString(bridgeDllPath) << std::endl;
            return false;
        }

        FARPROC localGetFuncPtrs = GetProcAddress(hLocalDll, "Bridge_GetFunctionPointers");
        if (!localGetFuncPtrs)
        {
            std::cerr << "[!] Failed to get address of Bridge_GetFunctionPointers.\n";
            FreeLibrary(hLocalDll);
            return false;
        }

        uintptr_t localBase = reinterpret_cast<uintptr_t>(hLocalDll);
        uintptr_t rva = reinterpret_cast<uintptr_t>(localGetFuncPtrs) - localBase;
        LPVOID remoteGetFuncPtrs = reinterpret_cast<LPVOID>(bridgeDllBase + rva);

        FreeLibrary(hLocalDll); // We have the RVA, we can free the local copy.

        // 2. Allocate memory in target process for the struct
        LPVOID allocMem = VirtualAllocEx(hProcess, nullptr, sizeof(Bridge_FunctionPointers), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (!allocMem)
        {
            std::cerr << "[!] Failed to allocate memory for function pointers.\n";
            return false;
        }

        // 3. Call the remote Bridge_GetFunctionPointers function
        HANDLE hThread = CreateRemoteThread(hProcess, nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(remoteGetFuncPtrs), allocMem, 0, nullptr);
        if (!hThread)
        {
            std::cerr << "[!] Failed to create remote thread for getFunctionPointers.\n";
            VirtualFreeEx(hProcess, allocMem, 0, MEM_RELEASE);
            return false;
        }

        WaitForSingleObject(hThread, INFINITE);
        DWORD exitCode = 0;
        GetExitCodeThread(hThread, &exitCode);
        CloseHandle(hThread);

        if (exitCode == 0)
        {
            std::cerr << "[!] Remote Bridge_GetFunctionPointers failed.\n";
            VirtualFreeEx(hProcess, allocMem, 0, MEM_RELEASE);
            return false;
        }

        // 4. Read the struct back from the allocated memory
        Bridge_FunctionPointers localPtrs;
        if (!ReadProcessMemory(hProcess, allocMem, &localPtrs, sizeof(Bridge_FunctionPointers), nullptr))
        {
            std::cerr << "[!] Failed to read function pointers from target process.\n";
            VirtualFreeEx(hProcess, allocMem, 0, MEM_RELEASE);
            return false;
        }

        // 5. Free the remote memory
        VirtualFreeEx(hProcess, allocMem, 0, MEM_RELEASE);

        // 6. Store the *remote* function pointers
        funcPtrs.Initialize = localPtrs.pBridge_Initialize;
        funcPtrs.InvokeMethod = localPtrs.pBridge_InvokeMethod;

        if (funcPtrs.Initialize == nullptr || funcPtrs.InvokeMethod == nullptr)
        {
            std::cerr << "[!] Remote bridge returned NULL function pointers.\n";
            return false;
        }

        return true;
    }

    /**
     * @brief Extracts a DLL from embedded resources to a temporary file
     * (Logic from your MonoBase.h)
     */
    std::wstring extractResourceToTempFile(const std::string &resourceName, const std::wstring &outputFileName)
    {
        HMODULE hModule = GetModuleHandle(nullptr);
        HRSRC hResource = FindResourceA(hModule, resourceName.c_str(), MAKEINTRESOURCEA(10)); // 10 is RT_RCDATA
        if (!hResource)
        {
            std::cerr << "[!] Failed to find resource: " << resourceName << "\n";
            return L"";
        }

        HGLOBAL hData = LoadResource(hModule, hResource);
        if (!hData)
        {
            std::cerr << "[!] Failed to load resource: " << resourceName << "\n";
            return L"";
        }

        LPVOID pData = LockResource(hData);
        if (!pData)
        {
            std::cerr << "[!] Failed to lock resource: " << resourceName << "\n";
            return L"";
        }

        DWORD dwSize = SizeofResource(hModule, hResource);
        WCHAR tempPath[MAX_PATH];

        // Create a unique subdirectory to avoid name collisions
        GUID guid;
        CoCreateGuid(&guid);
        WCHAR subDir[MAX_PATH];
        StringFromGUID2(guid, subDir, MAX_PATH); // Generates a unique string like "{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}"
        GetTempPathW(MAX_PATH, tempPath);
        wcscat_s(tempPath, MAX_PATH, subDir);
        CreateDirectoryW(tempPath, nullptr);

        // Construct the full path with the desired name
        WCHAR dllPath[MAX_PATH];
        wcscpy_s(dllPath, MAX_PATH, tempPath);
        wcscat_s(dllPath, MAX_PATH, L"\\");
        wcscat_s(dllPath, MAX_PATH, outputFileName.c_str());

        HANDLE hFile = CreateFileW(dllPath, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (hFile == INVALID_HANDLE_VALUE)
        {
            std::cerr << "[!] Failed to create temp file: " << wstringToString(dllPath) << "\n";
            return L"";
        }

        DWORD bytesWritten;
        WriteFile(hFile, pData, dwSize, &bytesWritten, nullptr);
        CloseHandle(hFile);

        if (bytesWritten != dwSize)
        {
            std::cerr << "[!] Failed to write resource to file: " << wstringToString(dllPath) << "\n";
            DeleteFileW(dllPath);
            return L"";
        }

        return std::wstring(dllPath);
    }
};
