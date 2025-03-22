#pragma once

#include "TrainerBase.h"
#include <string>
#include <variant>
#include <vector>
#include <windows.h>
#include <iostream>

class MonoBase : public TrainerBase
{
public:
    using Param = std::variant<int, float, std::string>;

    MonoBase(const std::wstring &processIdentifier, bool useWindowTitle = false)
        : TrainerBase(processIdentifier, useWindowTitle)
    {
    }

    virtual ~MonoBase()
    {
        if (!tempDllPath.empty())
        {
            DeleteFileW(tempDllPath.c_str());
        }
    }

    /** Injects the C++ bridge DLL into the target process from embedded resources
     * @return True if injection succeeds, false otherwise
     */
    bool injectBridgeDLL()
    {
        // Extract the DLL from resources
        tempDllPath = extractResourceToTempFile("MONOBRIDGE_DLL");
        if (tempDllPath.empty())
        {
            std::cerr << "[!] Failed to extract MonoBridge.dll from resources.\n";
            return false;
        }

        std::cout << "[+] Extracted MonoBridge.dll to " << wstringToString(tempDllPath) << std::endl;

        if (!hProcess || !isProcessRunning())
        {
            std::cerr << "[!] Process not running or handle invalid.\n";
            return false;
        }

        size_t pathSize = (tempDllPath.size() + 1) * sizeof(wchar_t);
        LPVOID allocMem = VirtualAllocEx(hProcess, nullptr, pathSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (!allocMem)
        {
            std::cerr << "[!] Failed to allocate memory in target process.\n";
            return false;
        }

        if (!WriteProcessMemory(hProcess, allocMem, tempDllPath.c_str(), pathSize, nullptr))
        {
            std::cerr << "[!] Failed to write DLL path to target process.\n";
            VirtualFreeEx(hProcess, allocMem, 0, MEM_RELEASE);
            return false;
        }

        // Combine kernel32 handle and LoadLibraryW address retrieval
        HMODULE kernel32 = GetModuleHandleW(L"kernel32.dll");
        LPVOID loadLibraryAddr = kernel32 ? reinterpret_cast<LPVOID>(GetProcAddress(kernel32, "LoadLibraryW")) : nullptr;
        if (!loadLibraryAddr)
        {
            std::cerr << "[!] Failed to get LoadLibraryW address.\n";
            VirtualFreeEx(hProcess, allocMem, 0, MEM_RELEASE);
            return false;
        }

        HANDLE hThread = CreateRemoteThread(hProcess, nullptr, 0,
                                            reinterpret_cast<LPTHREAD_START_ROUTINE>(loadLibraryAddr),
                                            allocMem, 0, nullptr);
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
        std::cout << "[+] Bridge DLL injected successfully at base: 0x" << std::hex << bridgeDllBase << std::dec << std::endl;
        return true;
    }

    /** Retrieves function pointers from the injected bridge DLL
     * @return True if pointers are retrieved, false otherwise
     */
    bool getFunctionPointers()
    {
        if (!hProcess || !bridgeDllBase)
        {
            std::cerr << "[!] Process handle invalid or bridge DLL not injected.\n";
            return false;
        }

        std::cout << "[+] Attempting to retrieve function pointers.\n";

        HMODULE hLocalDll = LoadLibraryW(tempDllPath.c_str());
        if (!hLocalDll)
        {
            std::cerr << "[!] Failed to load bridge DLL locally from " << wstringToString(tempDllPath) << std::endl;
            return false;
        }

        FARPROC localGetFuncPtrs = GetProcAddress(hLocalDll, "GetFunctionPointersThread");
        if (!localGetFuncPtrs)
        {
            std::cerr << "[!] Failed to get address of GetFunctionPointersThread.\n";
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
            std::cerr << "[!] Failed to allocate memory for function pointers.\n";
            FreeLibrary(hLocalDll);
            return false;
        }

        HANDLE hThread = CreateRemoteThread(hProcess, nullptr, 0,
                                            reinterpret_cast<LPTHREAD_START_ROUTINE>(remoteGetFuncPtrs),
                                            allocMem, 0, nullptr);
        if (!hThread)
        {
            std::cerr << "[!] Failed to create remote thread for function pointers.\n";
            VirtualFreeEx(hProcess, allocMem, 0, MEM_RELEASE);
            FreeLibrary(hLocalDll);
            return false;
        }

        WaitForSingleObject(hThread, INFINITE);
        CloseHandle(hThread);

        if (!ReadProcessMemory(hProcess, allocMem, &funcPtrs, sizeof(FunctionPointers), nullptr))
        {
            std::cerr << "[!] Failed to read function pointers from target process.\n";
            VirtualFreeEx(hProcess, allocMem, 0, MEM_RELEASE);
            FreeLibrary(hLocalDll);
            return false;
        }

        VirtualFreeEx(hProcess, allocMem, 0, MEM_RELEASE);
        FreeLibrary(hLocalDll);
        std::cout << "[+] Function pointers retrieved successfully.\n";
        return true;
    }

    /** Loads a C# assembly from an embedded resource into the Mono runtime
     * @param resourceName Name of the resource (e.g., "GCMINJECTION_DLL")
     * @return True if successful, false otherwise
     */
    bool loadAssembly(const std::string &resourceName)
    {
        if (!hProcess || !funcPtrs.LoadAssemblyThread)
        {
            std::cerr << "[!] Process handle invalid or LoadAssemblyThread pointer missing.\n";
            return false;
        }

        std::wstring assemblyPath = extractResourceToTempFile(resourceName);
        if (assemblyPath.empty())
        {
            std::cerr << "[!] Failed to extract assembly from resource: " << resourceName << "\n";
            return false;
        }
        extractedFiles.push_back(assemblyPath);

        std::cout << "[+] Attempting to load assembly from: " << wstringToString(assemblyPath) << "\n";

        std::string pathStr = wstringToString(assemblyPath);
        size_t pathLen = pathStr.size() + 1;
        size_t paramsSize = sizeof(LoadAssemblyParams) + pathLen;

        LPVOID allocMem = VirtualAllocEx(hProcess, nullptr, paramsSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (!allocMem)
        {
            std::cerr << "[!] Failed to allocate memory for assembly path.\n";
            return false;
        }

        LPVOID pathMem = reinterpret_cast<LPVOID>(reinterpret_cast<uintptr_t>(allocMem) + sizeof(LoadAssemblyParams));
        if (!WriteProcessMemory(hProcess, pathMem, pathStr.c_str(), pathLen, nullptr))
        {
            std::cerr << "[!] Failed to write assembly path to target process.\n";
            VirtualFreeEx(hProcess, allocMem, 0, MEM_RELEASE);
            return false;
        }

        LoadAssemblyParams params{};
        params.path = reinterpret_cast<const char *>(pathMem);
        if (!WriteProcessMemory(hProcess, allocMem, &params, sizeof(LoadAssemblyParams), nullptr))
        {
            std::cerr << "[!] Failed to write LoadAssemblyParams to target process.\n";
            VirtualFreeEx(hProcess, allocMem, 0, MEM_RELEASE);
            return false;
        }

        HANDLE hThread = CreateRemoteThread(hProcess, nullptr, 0,
                                            reinterpret_cast<LPTHREAD_START_ROUTINE>(funcPtrs.LoadAssemblyThread),
                                            allocMem, 0, nullptr);
        if (!hThread)
        {
            std::cerr << "[!] Failed to create remote thread to load assembly.\n";
            VirtualFreeEx(hProcess, allocMem, 0, MEM_RELEASE);
            return false;
        }

        WaitForSingleObject(hThread, INFINITE);
        CloseHandle(hThread);
        VirtualFreeEx(hProcess, allocMem, 0, MEM_RELEASE);
        std::cout << "[+] Assembly load thread completed.\n";
        return true;
    }

    /** Invokes a static method in the loaded C# assembly
     * @param namespaceName Namespace of the class (e.g., "MyNamespace")
     * @param className Class name (e.g., "MyClass")
     * @param methodName Method name (e.g., "MyMethod")
     * @param params Vector of parameters
     * @return True if successful, false otherwise
     */
    bool invokeMethod(const std::string &namespaceName, const std::string &className,
                      const std::string &methodName, const std::vector<Param> &params)
    {
        if (!hProcess || !funcPtrs.InvokeMethodThread)
        {
            std::cerr << "[!] Process handle invalid or InvokeMethodThread pointer missing.\n";
            return false;
        }

        std::cout << "[+] Attempting to invoke " << namespaceName << "." << className << "." << methodName
                  << " with " << params.size() << " parameters.\n";

        size_t namespaceLen = namespaceName.size() + 1;
        size_t classLen = className.size() + 1;
        size_t methodLen = methodName.size() + 1;
        size_t paramValuesSize = sizeof(ParamValue) * params.size();
        size_t totalStringSize = 0;
        for (const auto &p : params)
        {
            if (std::holds_alternative<std::string>(p))
                totalStringSize += std::get<std::string>(p).size() + 1;
        }
        size_t totalSize = sizeof(InvokeMethodParams) + namespaceLen + classLen + methodLen + paramValuesSize + totalStringSize;

        LPVOID allocMem = VirtualAllocEx(hProcess, nullptr, totalSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (!allocMem)
        {
            std::cerr << "[!] Failed to allocate memory for method invocation.\n";
            return false;
        }

        LPVOID currentMem = reinterpret_cast<LPVOID>(reinterpret_cast<uintptr_t>(allocMem) + sizeof(InvokeMethodParams));
        InvokeMethodParams invokeParams{};

        // Write namespace
        invokeParams.namespaceName = reinterpret_cast<const char *>(currentMem);
        if (!WriteProcessMemory(hProcess, currentMem, namespaceName.c_str(), namespaceLen, nullptr))
        {
            std::cerr << "[!] Failed to write namespace name.\n";
            VirtualFreeEx(hProcess, allocMem, 0, MEM_RELEASE);
            return false;
        }
        currentMem = reinterpret_cast<LPVOID>(reinterpret_cast<uintptr_t>(currentMem) + namespaceLen);

        // Write class name
        invokeParams.className = reinterpret_cast<const char *>(currentMem);
        if (!WriteProcessMemory(hProcess, currentMem, className.c_str(), classLen, nullptr))
        {
            std::cerr << "[!] Failed to write class name.\n";
            VirtualFreeEx(hProcess, allocMem, 0, MEM_RELEASE);
            return false;
        }
        currentMem = reinterpret_cast<LPVOID>(reinterpret_cast<uintptr_t>(currentMem) + classLen);

        // Write method name
        invokeParams.methodName = reinterpret_cast<const char *>(currentMem);
        if (!WriteProcessMemory(hProcess, currentMem, methodName.c_str(), methodLen, nullptr))
        {
            std::cerr << "[!] Failed to write method name.\n";
            VirtualFreeEx(hProcess, allocMem, 0, MEM_RELEASE);
            return false;
        }
        currentMem = reinterpret_cast<LPVOID>(reinterpret_cast<uintptr_t>(currentMem) + methodLen);

        // Write parameter values
        LPVOID paramValuesMem = currentMem;
        std::vector<ParamValue> paramValues(params.size());
        for (size_t i = 0; i < params.size(); ++i)
        {
            if (std::holds_alternative<int>(params[i]))
            {
                paramValues[i].type = ParamValue::INT;
                paramValues[i].i = std::get<int>(params[i]);
            }
            else if (std::holds_alternative<float>(params[i]))
            {
                paramValues[i].type = ParamValue::FLOAT;
                paramValues[i].f = std::get<float>(params[i]);
            }
            else if (std::holds_alternative<std::string>(params[i]))
            {
                paramValues[i].type = ParamValue::STRING;
                const std::string &str = std::get<std::string>(params[i]);
                paramValues[i].s = reinterpret_cast<const char *>(currentMem);
                if (!WriteProcessMemory(hProcess, currentMem, str.c_str(), str.size() + 1, nullptr))
                {
                    std::cerr << "[!] Failed to write string parameter.\n";
                    VirtualFreeEx(hProcess, allocMem, 0, MEM_RELEASE);
                    return false;
                }
                currentMem = reinterpret_cast<LPVOID>(reinterpret_cast<uintptr_t>(currentMem) + str.size() + 1);
            }
        }
        invokeParams.paramCount = static_cast<int>(params.size());
        invokeParams.paramValues = reinterpret_cast<ParamValue *>(paramValuesMem);
        if (!WriteProcessMemory(hProcess, paramValuesMem, paramValues.data(), paramValuesSize, nullptr))
        {
            std::cerr << "[!] Failed to write parameter values.\n";
            VirtualFreeEx(hProcess, allocMem, 0, MEM_RELEASE);
            return false;
        }

        // Write InvokeMethodParams
        if (!WriteProcessMemory(hProcess, allocMem, &invokeParams, sizeof(InvokeMethodParams), nullptr))
        {
            std::cerr << "[!] Failed to write InvokeMethodParams.\n";
            VirtualFreeEx(hProcess, allocMem, 0, MEM_RELEASE);
            return false;
        }

        HANDLE hThread = CreateRemoteThread(hProcess, nullptr, 0,
                                            reinterpret_cast<LPTHREAD_START_ROUTINE>(funcPtrs.InvokeMethodThread),
                                            allocMem, 0, nullptr);
        if (!hThread)
        {
            std::cerr << "[!] Failed to create remote thread for method invocation.\n";
            VirtualFreeEx(hProcess, allocMem, 0, MEM_RELEASE);
            return false;
        }

        WaitForSingleObject(hThread, INFINITE);
        CloseHandle(hThread);
        VirtualFreeEx(hProcess, allocMem, 0, MEM_RELEASE);
        std::cout << "[+] Method invocation thread completed.\n";
        return true;
    }

private:
    std::vector<std::wstring> extractedFiles;
    std::wstring tempDllPath;
    uintptr_t bridgeDllBase = 0;

    struct FunctionPointers
    {
        LPVOID LoadAssemblyThread = nullptr;
        LPVOID InvokeMethodThread = nullptr;
    } funcPtrs;

    struct LoadAssemblyParams
    {
        const char *path;
    };

    struct ParamValue
    {
        enum Type
        {
            INT,
            FLOAT,
            STRING
        } type;
        union
        {
            int i;
            float f;
            const char *s;
        };
    };

    struct InvokeMethodParams
    {
        const char *namespaceName;
        const char *className;
        const char *methodName;
        int paramCount;
        ParamValue *paramValues;
    };

    /** Extracts a DLL from embedded resources to a temporary file
     * @param resourceName The resource identifier (e.g., "MONOBRIDGE_DLL")
     * @return Path to the temporary file, or empty string if extraction fails
     */
    std::wstring extractResourceToTempFile(const std::string &resourceName)
    {
        HMODULE hModule = GetModuleHandle(nullptr);
        HRSRC hResource = FindResourceA(hModule, resourceName.c_str(), MAKEINTRESOURCEA(10));
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
        WCHAR tempPath[MAX_PATH], tempFileName[MAX_PATH];
        GetTempPathW(MAX_PATH, tempPath);
        GetTempFileNameW(tempPath, L"RES", 0, tempFileName);

        HANDLE hFile = CreateFileW(tempFileName, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (hFile == INVALID_HANDLE_VALUE)
        {
            std::cerr << "[!] Failed to create temporary file for resource.\n";
            return L"";
        }

        DWORD bytesWritten;
        WriteFile(hFile, pData, dwSize, &bytesWritten, nullptr);
        CloseHandle(hFile);

        if (bytesWritten != dwSize)
        {
            std::cerr << "[!] Failed to write resource to temporary file.\n";
            DeleteFileW(tempFileName);
            return L"";
        }

        return std::wstring(tempFileName);
    }
};