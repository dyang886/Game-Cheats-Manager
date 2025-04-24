// MonoBase.h
#pragma once

#include "TrainerBase.h"
#include <FL/Fl.H>
#include <variant>
#include <shlwapi.h>
#include <sstream>

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
        cleanUp();
    }

    void cleanUp() override
    {
        // Fl::remove_timeout(check_data_available, this);

        // if (dllInjected && hProcess && bridgeDllBase != 0)
        // {
        //     HMODULE kernel32 = GetModuleHandleW(L"kernel32.dll");
        //     if (kernel32)
        //     {
        //         LPVOID freeLibraryAddr = reinterpret_cast<LPVOID>(GetProcAddress(kernel32, "FreeLibrary"));
        //         if (freeLibraryAddr)
        //         {
        //             HANDLE hThread = CreateRemoteThread(hProcess, nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(freeLibraryAddr), reinterpret_cast<LPVOID>(bridgeDllBase), 0, nullptr);
        //             if (hThread)
        //             {
        //                 WaitForSingleObject(hThread, INFINITE);
        //                 CloseHandle(hThread);
        //             }
        //             else
        //             {
        //                 std::cerr << "[!] Failed to create remote thread for FreeLibrary.\n";
        //             }
        //         }
        //         else
        //         {
        //             std::cerr << "[!] Failed to get FreeLibrary address.\n";
        //         }
        //     }
        //     else
        //     {
        //         std::cerr << "[!] Failed to get kernel32.dll handle.\n";
        //     }
        //     bridgeDllBase = 0;
        // }

        TrainerBase::cleanUp();

        if (dataBuffer)
        {
            UnmapViewOfFile(dataBuffer);
            dataBuffer = NULL;
        }

        if (hMapFile)
        {
            CloseHandle(hMapFile);
            hMapFile = NULL;
        }

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

        if (extractedFiles.size() > 0)
        {
            for (const auto &file : extractedFiles)
            {
                DeleteFileW(file.c_str());
            }
            extractedFiles.clear();
        }

        dllInjected = false;
    }

    bool initializeDllInjection()
    {
        if (!dllInjected)
        {
            if (!injectBridgeDLL())
                return false;

            if (!getFunctionPointers())
                return false;

            if (!loadAssembly("GCMINJECTION_DLL"))
                return false;

            if (!invokeMethod("", "GCMInjection", "Initialize", {}))
            {
                std::cerr << "[!] Failed to initialize dispatcher.\n";
                return false;
            }

            if (!create_shared_memory())
            {
                std::cerr << "[!] Failed to create shared memory.\n";
                return false;
            }

            dllInjected = true;
        }
        return dllInjected;
    }

    bool create_shared_memory()
    {
        const char *sharedMemName = "TrainerSharedMemory";
        hMapFile = CreateFileMappingA(
            INVALID_HANDLE_VALUE,
            NULL,
            PAGE_READWRITE,
            0,
            bufferSize,
            sharedMemName);

        if (hMapFile == NULL)
        {
            std::cerr << "[!] Could not create file mapping object: " << GetLastError() << "\n";
            return false;
        }

        dataBuffer = MapViewOfFile(
            hMapFile,
            FILE_MAP_ALL_ACCESS,
            0,
            0,
            bufferSize);

        if (dataBuffer == NULL)
        {
            std::cerr << "[!] Could not map view of file: " << GetLastError() << "\n";
            CloseHandle(hMapFile);
            hMapFile = NULL;
            return false;
        }

        return true;
    }

    static void check_data_available(void *data)
    {
        MonoBase *self = static_cast<MonoBase *>(data);

        if (self->dataBuffer != nullptr)
        {
            volatile bool *available = (volatile bool *)((char *)self->dataBuffer + 0);
            int *lengthPtr = (int *)((char *)self->dataBuffer + 4);
            char *msgPtr = (char *)((char *)self->dataBuffer + 8);

            if (*available)
            {
                int length = *lengthPtr;
                if (length > 0 && length < self->bufferSize - 8)
                {
                    std::string message(msgPtr, length);
                    std::cout << message << "\n";
                    *available = false;
                }
                else
                {
                    std::cerr << "[!] Invalid message length: " << length << "\n";
                }
            }
        }

        Fl::repeat_timeout(0.1, check_data_available, data);
    }

    /** Injects the C++ bridge DLL into the target process from embedded resources
     * @return True if injection succeeds, false otherwise
     */
    bool injectBridgeDLL()
    {
        // Extract the DLL from resources
        bridgeDllPath = extractResourceToTempFile("MONOBRIDGE_DLL");
        if (bridgeDllPath.empty())
        {
            std::cerr << "[!] Failed to extract MonoBridge.dll from resources.\n";
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

        // Combine kernel32 handle and LoadLibraryW address retrieval
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

        HMODULE hLocalDll = LoadLibraryW(bridgeDllPath.c_str());
        if (!hLocalDll)
        {
            std::cerr << "[!] Failed to load bridge DLL locally from " << wstringToString(bridgeDllPath) << std::endl;
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

        LPVOID allocMem = VirtualAllocEx(hProcess, nullptr, sizeof(FunctionPointers), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (!allocMem)
        {
            std::cerr << "[!] Failed to allocate memory for function pointers.\n";
            FreeLibrary(hLocalDll);
            return false;
        }

        HANDLE hThread = CreateRemoteThread(hProcess, nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(remoteGetFuncPtrs), allocMem, 0, nullptr);
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

        HANDLE hThread = CreateRemoteThread(hProcess, nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(funcPtrs.LoadAssemblyThread), allocMem, 0, nullptr);
        if (!hThread)
        {
            std::cerr << "[!] Failed to create remote thread to load assembly.\n";
            VirtualFreeEx(hProcess, allocMem, 0, MEM_RELEASE);
            return false;
        }

        WaitForSingleObject(hThread, INFINITE);
        CloseHandle(hThread);
        VirtualFreeEx(hProcess, allocMem, 0, MEM_RELEASE);
        return true;
    }

    /** Invokes a static method in the loaded C# assembly
     * @param namespaceName Namespace of the class (e.g., "MyNamespace")
     * @param className Class name (e.g., "MyClass")
     * @param methodName Method name (e.g., "MyMethod")
     * @param params Vector of parameters
     * @return True if successful, false otherwise
     */
    bool invokeMethod(const std::string &namespaceName, const std::string &className, const std::string &methodName, const std::vector<Param> &params)
    {
        if (!hProcess || !funcPtrs.InvokeMethodThread)
        {
            std::cerr << "[!] Process handle invalid or InvokeMethodThread pointer missing.\n";
            return false;
        }

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

        HANDLE hThread = CreateRemoteThread(hProcess, nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(funcPtrs.InvokeMethodThread), allocMem, 0, nullptr);
        if (!hThread)
        {
            std::cerr << "[!] Failed to create remote thread for method invocation.\n";
            VirtualFreeEx(hProcess, allocMem, 0, MEM_RELEASE);
            return false;
        }

        std::stringstream ss;
        ss << "[+] Invoked method " << (namespaceName.empty() ? "" : namespaceName + ".") << className << "." << methodName << "(";
        for (size_t i = 0; i < params.size(); ++i)
        {
            if (i > 0)
                ss << ", ";
            if (std::holds_alternative<int>(params[i]))
                ss << std::get<int>(params[i]);
            else if (std::holds_alternative<float>(params[i]))
                ss << std::get<float>(params[i]);
            else if (std::holds_alternative<std::string>(params[i]))
                ss << "\"" << std::get<std::string>(params[i]) << "\"";
        }
        ss << ")\n";
        std::cout << ss.str();

        WaitForSingleObject(hThread, INFINITE);
        CloseHandle(hThread);
        VirtualFreeEx(hProcess, allocMem, 0, MEM_RELEASE);
        return true;
    }

private:
    bool dllInjected = false;
    std::vector<std::wstring> extractedFiles;
    std::wstring bridgeDllPath;
    uintptr_t bridgeDllBase = 0;
    HANDLE hMapFile = NULL;

    // For injected dll communication
    LPVOID dataBuffer = nullptr;
    size_t bufferSize = 1024 * 1024;

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
        WCHAR tempPath[MAX_PATH];

        if (resourceName == "MONOBRIDGE_DLL")
        {
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
            wcscat_s(dllPath, MAX_PATH, L"MonoBridge.dll");

            HANDLE hFile = CreateFileW(dllPath, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (hFile == INVALID_HANDLE_VALUE)
            {
                std::cerr << "[!] Failed to create MonoBridge file: " << wstringToString(dllPath) << "\n";
                return L"";
            }

            DWORD bytesWritten;
            WriteFile(hFile, pData, dwSize, &bytesWritten, nullptr);
            CloseHandle(hFile);

            if (bytesWritten != dwSize)
            {
                std::cerr << "[!] Failed to write MonoBridge resource to file: " << wstringToString(dllPath) << "\n";
                DeleteFileW(dllPath);
                return L"";
            }

            return std::wstring(dllPath);
        }
        else
        {
            // Default behavior: use a temporary filename
            WCHAR tempFileName[MAX_PATH];
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
    }
};