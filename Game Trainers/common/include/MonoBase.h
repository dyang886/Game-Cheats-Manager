// MonoBase.h
#pragma once

#include <FL/Fl.H>
#include "TrainerBase.h"
#include <psapi.h>
#include <shlwapi.h>
#include <variant>

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
        // Note: MonoBridge.dll and GCMInjection.dll are not freed
        TrainerBase::cleanUp();

        if (loggingBuffer)
        {
            UnmapViewOfFile(loggingBuffer);
            loggingBuffer = nullptr;
        }
        if (hLoggingMapFile)
        {
            CloseHandle(hLoggingMapFile);
            hLoggingMapFile = nullptr;
        }
        if (responseBuffer)
        {
            UnmapViewOfFile(responseBuffer);
            responseBuffer = nullptr;
        }
        if (hResponseMapFile)
        {
            CloseHandle(hResponseMapFile);
            hResponseMapFile = nullptr;
        }
        if (hResponseEvent)
        {
            CloseHandle(hResponseEvent);
            hResponseEvent = nullptr;
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

        if (!extractedFiles.empty())
        {
            for (const auto &file : extractedFiles)
            {
                DeleteFileW(file.c_str());
            }
            extractedFiles.clear();
        }

        dllInjected = false;
        bridgeDllBase = 0;
        funcPtrs = {};
    }

    bool initializeDllInjection()
    {
        if (!dllInjected)
        {
            if (!injectBridgeDLL())
                return false;

            if (!getFunctionPointers())
                return false;

            if (!initializeLoggingChannel())
            {
                std::cerr << "[!] Failed to initialize data logging channel.\n";
                return false;
            }

            if (!initializeRequestChannel())
            {
                std::cerr << "[!] Failed to initialize data request channel.\n";
                return false;
            }

            if (!loadAssembly("GCMINJECTION_DLL"))
            {
                std::cerr << "[!] Failed to load injected assembly.\n";
                return false;
            }

            if (!invokeMethod("", "GCMInjection", "Initialize", {}))
            {
                std::cerr << "[!] Failed to initialize dispatcher.\n";
                return false;
            }

            dllInjected = true;
        }
        return dllInjected;
    }

    bool initializeLoggingChannel()
    {
        std::string loggingSharedMemName = "TrainerLoggingSharedMemory_" + std::to_string(procId);
        hLoggingMapFile = CreateFileMappingA(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, loggingBufferSize, loggingSharedMemName.c_str());
        if (hLoggingMapFile == nullptr)
        {
            std::cerr << "[!] Could not create file mapping object: " << GetLastError() << "\n";
            return false;
        }

        loggingBuffer = MapViewOfFile(hLoggingMapFile, FILE_MAP_ALL_ACCESS, 0, 0, 0);
        if (loggingBuffer == nullptr)
        {
            std::cerr << "[!] Could not map view of file: " << GetLastError() << "\n";
            CloseHandle(hLoggingMapFile);
            hLoggingMapFile = nullptr;
            return false;
        }

        ZeroMemory(loggingBuffer, sizeof(SharedMemoryHeader));
        return true;
    }

    static void check_logging_available(void *data)
    {
        MonoBase *self = static_cast<MonoBase *>(data);

        if (self->loggingBuffer != nullptr)
        {
            SharedMemoryHeader *header = static_cast<SharedMemoryHeader *>(self->loggingBuffer);
            char *bufferStart = static_cast<char *>(self->loggingBuffer) + sizeof(SharedMemoryHeader);
            size_t capacity = self->loggingBufferSize - sizeof(SharedMemoryHeader);

            while (header->head != header->tail)
            {
                if (header->tail + sizeof(size_t) > capacity)
                {
                    header->tail = 0;
                    continue;
                }

                size_t length = *reinterpret_cast<size_t *>(bufferStart + header->tail);
                if (length == 0)
                {
                    header->tail = 0;
                    continue;
                }
                if (length > capacity || header->tail + sizeof(size_t) + length > capacity)
                {
                    header->tail = 0;
                    continue;
                }

                char *msgPtr = bufferStart + header->tail + sizeof(size_t);
                std::string message(msgPtr, length);
                std::cout << message << "\n";
                header->tail = (header->tail + sizeof(size_t) + length) % capacity;
            }
        }

        Fl::repeat_timeout(0.1, check_logging_available, data);
    }

    bool initializeRequestChannel()
    {
        std::string responseSharedMemName = "TrainerResponseSharedMemory_" + std::to_string(procId);
        hResponseMapFile = CreateFileMappingA(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, responseBufferSize, responseSharedMemName.c_str());
        if (hResponseMapFile == nullptr)
        {
            std::cerr << "[!] Could not create response file mapping object: " << GetLastError() << "\n";
            return false;
        }

        responseBuffer = MapViewOfFile(hResponseMapFile, FILE_MAP_ALL_ACCESS, 0, 0, 0);
        if (responseBuffer == nullptr)
        {
            std::cerr << "[!] Could not map view of response file: " << GetLastError() << "\n";
            CloseHandle(hResponseMapFile);
            hResponseMapFile = nullptr;
            return false;
        }

        std::string eventName = "TrainerResponseEvent_" + std::to_string(procId);
        hResponseEvent = CreateEventA(nullptr, FALSE, FALSE, eventName.c_str());
        if (hResponseEvent == nullptr)
        {
            std::cerr << "[!] Could not create response event: " << GetLastError() << "\n";
            UnmapViewOfFile(responseBuffer);
            CloseHandle(hResponseMapFile);
            hResponseMapFile = nullptr;
            responseBuffer = nullptr;
            return false;
        }

        *reinterpret_cast<size_t *>(responseBuffer) = 0;
        return true;
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

        DWORD waitResult = WaitForSingleObject(hThread, 5000);
        CloseHandle(hThread);
        if (waitResult != WAIT_OBJECT_0)
        {
            std::cerr << "[!] Timed out while loading MonoBridge.dll in target process.\n";
            return false;
        }

        VirtualFreeEx(hProcess, allocMem, 0, MEM_RELEASE);

        if (!findInjectedBridgeBase())
        {
            std::cerr << "[!] Failed to find injected MonoBridge.dll in target process.\n";
            return false;
        }

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

        DWORD waitResult = WaitForSingleObject(hThread, 5000);
        DWORD exitCode = 0;
        GetExitCodeThread(hThread, &exitCode);
        CloseHandle(hThread);

        if (waitResult != WAIT_OBJECT_0)
        {
            std::cerr << "[!] Timed out while retrieving bridge function pointers.\n";
            FreeLibrary(hLocalDll);
            return false;
        }

        if (exitCode != 0)
        {
            std::cerr << "[!] Remote GetFunctionPointersThread failed: " << exitCode << "\n";
            VirtualFreeEx(hProcess, allocMem, 0, MEM_RELEASE);
            FreeLibrary(hLocalDll);
            return false;
        }

        if (!ReadProcessMemory(hProcess, allocMem, &funcPtrs, sizeof(FunctionPointers), nullptr))
        {
            std::cerr << "[!] Failed to read function pointers from target process.\n";
            VirtualFreeEx(hProcess, allocMem, 0, MEM_RELEASE);
            FreeLibrary(hLocalDll);
            return false;
        }

        if (!funcPtrs.LoadAssemblyThread || !funcPtrs.InvokeMethodThread)
        {
            std::cerr << "[!] Bridge returned invalid function pointers.\n";
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

        DWORD waitResult = WaitForSingleObject(hThread, 5000);
        DWORD exitCode = 0;
        GetExitCodeThread(hThread, &exitCode);
        CloseHandle(hThread);
        if (waitResult != WAIT_OBJECT_0)
        {
            std::cerr << "[!] Timed out while loading assembly in target process.\n";
            return false;
        }

        VirtualFreeEx(hProcess, allocMem, 0, MEM_RELEASE);
        if (exitCode != 0)
        {
            std::cerr << "[!] Failed to load assembly in target process.\n";
            return false;
        }
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
        if (paramValuesSize > 0 && !WriteProcessMemory(hProcess, paramValuesMem, paramValues.data(), paramValuesSize, nullptr))
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

        DWORD waitResult = WaitForSingleObject(hThread, 5000);
        DWORD exitCode = 0;
        GetExitCodeThread(hThread, &exitCode);
        CloseHandle(hThread);
        if (waitResult != WAIT_OBJECT_0)
        {
            std::cerr << "[!] Timed out while invoking method " << className << "." << methodName << ".\n";
            return false;
        }

        VirtualFreeEx(hProcess, allocMem, 0, MEM_RELEASE);
        if (exitCode != 0)
        {
            std::cerr << "[!] Failed to invoke method " << className << "." << methodName << ".\n";
            return false;
        }
        return true;
    }

    std::string invokeMethodReturn(const std::string &namespaceName, const std::string &className, const std::string &methodName, const std::vector<Param> &params)
    {
        if (!invokeMethod(namespaceName, className, methodName, params))
        {
            return "";
        }

        // Wait for the response event with a timeout
        DWORD waitResult = WaitForSingleObject(hResponseEvent, 5000); // 5 seconds timeout
        if (waitResult == WAIT_TIMEOUT)
        {
            std::cerr << "[!] WaitForSingleObject timed out after 5 seconds\n";
            return "";
        }
        else if (waitResult == WAIT_FAILED)
        {
            std::cerr << "[!] WaitForSingleObject failed: " << GetLastError() << "\n";
            return "";
        }

        // Read the response
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

private:
    bool dllInjected = false;
    std::vector<std::wstring> extractedFiles;
    std::wstring bridgeDllPath;
    uintptr_t bridgeDllBase = 0;

    // For injected dll communication
    HANDLE hLoggingMapFile = nullptr;
    LPVOID loggingBuffer = nullptr;
    const size_t loggingBufferSize = 1024 * 1024;
    HANDLE hResponseMapFile = nullptr;
    LPVOID responseBuffer = nullptr;
    HANDLE hResponseEvent = nullptr;
    const size_t responseBufferSize = 1024 * 1024;

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

    struct SharedMemoryHeader
    {
        volatile size_t head;
        volatile size_t tail;
    };

    bool findInjectedBridgeBase()
    {
        std::wstring dllName = bridgeDllPath.substr(bridgeDllPath.find_last_of(L"\\/") + 1);
        DWORD pollStart = GetTickCount();
        constexpr DWORD pollTimeout = 2000;

        while ((GetTickCount() - pollStart) < pollTimeout)
        {
            HMODULE modules[1024];
            DWORD needed = 0;
            if (EnumProcessModules(hProcess, modules, sizeof(modules), &needed))
            {
                int numModules = static_cast<int>(needed / sizeof(HMODULE));
                for (int i = 0; i < numModules; i++)
                {
                    wchar_t modulePath[MAX_PATH];
                    if (GetModuleFileNameExW(hProcess, modules[i], modulePath, MAX_PATH))
                    {
                        wchar_t *moduleName = PathFindFileNameW(modulePath);
                        if (_wcsicmp(modulePath, bridgeDllPath.c_str()) == 0 || _wcsicmp(moduleName, dllName.c_str()) == 0)
                        {
                            bridgeDllBase = reinterpret_cast<uintptr_t>(modules[i]);
                            return true;
                        }
                    }
                }
            }

            Sleep(50);
        }

        return false;
    }

    bool writeResourceToFile(const std::wstring &path, const void *data, DWORD size)
    {
        HANDLE hFile = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (hFile == INVALID_HANDLE_VALUE)
            return false;

        DWORD bytesWritten = 0;
        BOOL ok = WriteFile(hFile, data, size, &bytesWritten, nullptr);
        CloseHandle(hFile);

        if (!ok || bytesWritten != size)
        {
            DeleteFileW(path.c_str());
            return false;
        }

        return true;
    }

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
        if (dwSize == 0)
        {
            std::cerr << "[!] Empty resource: " << resourceName << "\n";
            return L"";
        }

        WCHAR tempPath[MAX_PATH];
        if (resourceName == "MONOBRIDGE_DLL")
        {
            GUID guid;
            if (FAILED(CoCreateGuid(&guid)))
            {
                std::cerr << "[!] Failed to create temporary MonoBridge directory name.\n";
                return L"";
            }

            WCHAR subDir[MAX_PATH];
            StringFromGUID2(guid, subDir, MAX_PATH);
            GetTempPathW(MAX_PATH, tempPath);
            wcscat_s(tempPath, MAX_PATH, subDir);
            if (!CreateDirectoryW(tempPath, nullptr))
            {
                std::cerr << "[!] Failed to create MonoBridge temp directory.\n";
                return L"";
            }

            WCHAR dllPath[MAX_PATH];
            wcscpy_s(dllPath, MAX_PATH, tempPath);
            wcscat_s(dllPath, MAX_PATH, L"\\");
            wcscat_s(dllPath, MAX_PATH, L"MonoBridge.dll");

            if (!writeResourceToFile(dllPath, pData, dwSize))
            {
                std::cerr << "[!] Failed to write MonoBridge resource to file: " << wstringToString(dllPath) << "\n";
                RemoveDirectoryW(tempPath);
                return L"";
            }

            return std::wstring(dllPath);
        }

        WCHAR tempFileName[MAX_PATH];
        GetTempPathW(MAX_PATH, tempPath);
        if (!GetTempFileNameW(tempPath, L"RES", 0, tempFileName))
        {
            std::cerr << "[!] Failed to create temporary file for resource.\n";
            return L"";
        }

        if (!writeResourceToFile(tempFileName, pData, dwSize))
        {
            std::cerr << "[!] Failed to write resource to temporary file.\n";
            return L"";
        }

        return std::wstring(tempFileName);
    }
};
