#include <windows.h>
#include <string>
#include <vector>

#define MONO_API extern "C" __declspec(dllimport)

typedef void *MonoDomain;
typedef void *MonoAssembly;
typedef void *MonoImage;
typedef void *MonoClass;
typedef void *MonoMethod;
typedef void *MonoObject;
typedef void *MonoThread;
typedef void *MonoString;

MONO_API MonoDomain *mono_get_root_domain();
MONO_API MonoThread *mono_thread_attach(MonoDomain *domain);
MONO_API MonoAssembly *mono_assembly_open(const char *filename, void *status);
MONO_API MonoAssembly *mono_domain_assembly_open(MonoDomain *domain, const char *name);
MONO_API MonoImage *mono_assembly_get_image(MonoAssembly *assembly);
MONO_API MonoClass *mono_class_from_name(MonoImage *image, const char *name_space, const char *name);
MONO_API MonoMethod *mono_class_get_method_from_name(MonoClass *klass, const char *name, int param_count);
MONO_API MonoObject *mono_runtime_invoke(MonoMethod *method, void *obj, void **params, MonoObject **exc);
MONO_API MonoString *mono_string_new(MonoDomain *domain, const char *str);

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

struct FunctionPointers
{
    LPVOID LoadAssemblyThread;
    LPVOID InvokeMethodThread;
};
struct LoadAssemblyParams
{
    const char *path;
};
struct InvokeMethodParams
{
    const char *namespaceName;
    const char *className;
    const char *methodName;
    int paramCount;
    ParamValue *paramValues;
};

MonoDomain *domain = nullptr;
MonoAssembly *loadedAssembly = nullptr;

static HANDLE hMapFile = NULL;
static LPVOID pSharedMemory = NULL;
static const size_t bufferSize = 1024 * 1024; // 1MB
static const char *sharedMemoryName = "TrainerSharedMemory";

extern "C" __declspec(dllexport) void SendData(const char *message)
{
    if (pSharedMemory == NULL)
    {
        hMapFile = OpenFileMappingA(
            FILE_MAP_ALL_ACCESS,
            FALSE,
            sharedMemoryName
        );

        if (hMapFile == NULL)
        {
            DWORD errorCode = GetLastError();
            return;
        }

        pSharedMemory = MapViewOfFile(
            hMapFile,
            FILE_MAP_ALL_ACCESS,
            0,
            0,
            0
        );

        if (pSharedMemory == NULL)
        {
            DWORD errorCode = GetLastError();
            CloseHandle(hMapFile);
            hMapFile = NULL;
            return;
        }
    }

    // Validate the message
    if (message == NULL)
    {
        return;
    }
    size_t length = strlen(message);
    if (length + 8 > bufferSize)
    {
        return;
    }

    volatile bool *available = (volatile bool *)pSharedMemory;
    int *lengthPtr = (int *)((char *)pSharedMemory + 4);
    char *msgPtr = (char *)((char *)pSharedMemory + 8);

    *available = false;
    *lengthPtr = static_cast<int>(length);
    memcpy(msgPtr, message, length);
    *available = true;
}

void Log(const char *message)
{
    std::string formattedMessage = "[MonoBridge] " + std::string(message);
    SendData(formattedMessage.c_str());
}

void LogToFile(const char *message)
{
    std::string formattedMessage = "[MonoBridge] " + std::string(message) + "\n";
    HANDLE hFile = CreateFileA("monobridge_log.txt", FILE_APPEND_DATA, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        DWORD written;
        WriteFile(hFile, formattedMessage.c_str(), static_cast<DWORD>(formattedMessage.size()), &written, NULL);
        CloseHandle(hFile);
    }
}

void InitializeMono()
{
    if (domain)
        return;
    HMODULE monoModule = GetModuleHandleA("mono-2.0-bdwgc.dll");
    if (!monoModule)
    {
        Log("[!] Failed to find mono-2.0-bdwgc.dll");
        return;
    }
    auto getRootDomain = (MonoDomain * (*)()) GetProcAddress(monoModule, "mono_get_root_domain");
    auto threadAttach = (MonoThread * (*)(MonoDomain *)) GetProcAddress(monoModule, "mono_thread_attach");
    if (!getRootDomain || !threadAttach)
    {
        Log("[!] Failed to get Mono functions");
        return;
    }
    domain = getRootDomain();
    if (!domain)
    {
        Log("[!] Failed to get root domain");
        return;
    }
    threadAttach(domain);
}

void LoadAssembly(const char *path)
{
    InitializeMono();
    if (!domain)
    {
        Log("[!] Mono domain not initialized");
        return;
    }
    HMODULE monoModule = GetModuleHandleA("mono-2.0-bdwgc.dll");
    auto assemblyOpen = (MonoAssembly * (*)(const char *, void *)) GetProcAddress(monoModule, "mono_assembly_open");
    if (!assemblyOpen)
    {
        Log("[!] Failed to get mono_assembly_open");
        return;
    }
    loadedAssembly = assemblyOpen(path, nullptr);
    if (!loadedAssembly)
    {
        Log("[!] Failed to load assembly");
    }
}

extern "C" __declspec(dllexport) DWORD WINAPI LoadAssemblyThread(LPVOID lpParam)
{
    LoadAssemblyParams *params = static_cast<LoadAssemblyParams *>(lpParam);
    LoadAssembly(params->path);
    return 0;
}

extern "C" __declspec(dllexport) DWORD WINAPI InvokeMethodThread(LPVOID lpParam)
{
    InvokeMethodParams *params = static_cast<InvokeMethodParams *>(lpParam);
    InitializeMono();
    if (!domain)
    {
        Log("[!] Mono domain not initialized");
        return 1;
    }

    HMODULE monoModule = GetModuleHandleA("mono-2.0-bdwgc.dll");
    if (monoModule)
    {
        // CRITICAL: Attach the thread to the Mono domain
        auto threadAttach = (MonoThread * (*)(MonoDomain *)) GetProcAddress(monoModule, "mono_thread_attach");
        if (threadAttach && domain)
        {
            threadAttach(domain);
        }
    }
    else
    {
        Log("[!] Failed to get mono-2.0-bdwgc.dll handle");
        return 1;
    }

    auto getImage = (MonoImage * (*)(MonoAssembly *)) GetProcAddress(monoModule, "mono_assembly_get_image");
    auto classFromName = (MonoClass * (*)(MonoImage *, const char *, const char *)) GetProcAddress(monoModule, "mono_class_from_name");
    auto getMethod = (MonoMethod * (*)(MonoClass *, const char *, int)) GetProcAddress(monoModule, "mono_class_get_method_from_name");
    auto runtimeInvoke = (MonoObject * (*)(MonoMethod *, void *, void **, MonoObject **)) GetProcAddress(monoModule, "mono_runtime_invoke");
    auto stringNew = (MonoString * (*)(MonoDomain *, const char *)) GetProcAddress(monoModule, "mono_string_new");

    if (!getImage || !classFromName || !getMethod || !runtimeInvoke || !stringNew)
    {
        Log("[!] Failed to get required Mono functions");
        return 1;
    }

    if (!loadedAssembly)
    {
        Log("[!] No assembly loaded");
        return 1;
    }
    MonoImage *image = getImage(loadedAssembly);
    if (!image)
    {
        Log("[!] Failed to get image");
        return 1;
    }

    MonoClass *klass = classFromName(image, params->namespaceName, params->className);
    if (!klass)
    {
        Log("[!] Failed to find class");
        return 1;
    }

    MonoMethod *method = getMethod(klass, params->methodName, params->paramCount);
    if (!method)
    {
        Log("[!] Failed to find method");
        return 1;
    }

    std::vector<void *> args(params->paramCount);
    std::vector<int> intParams;
    std::vector<float> floatParams;
    std::vector<MonoString *> stringParams;

    for (int i = 0; i < params->paramCount; ++i)
    {
        ParamValue &pv = params->paramValues[i];
        switch (pv.type)
        {
        case ParamValue::INT:
            intParams.push_back(pv.i);
            args[i] = &intParams.back();
            break;
        case ParamValue::FLOAT:
            floatParams.push_back(pv.f);
            args[i] = &floatParams.back();
            break;
        case ParamValue::STRING:
        {
            MonoString *monoStr = stringNew(domain, pv.s);
            if (!monoStr)
            {
                Log("[!] Failed to create MonoString");
                return 1;
            }
            stringParams.push_back(monoStr);
            args[i] = monoStr;
            break;
        }
        default:
            Log("[!] Unsupported parameter type");
            return 1;
        }
    }

    MonoObject *exc = nullptr;
    runtimeInvoke(method, nullptr, args.empty() ? nullptr : args.data(), &exc);
    if (exc)
    {
        Log("[!] Exception during method invocation");
        return 1;
    }

    return 0;
}

extern "C" __declspec(dllexport) DWORD WINAPI GetFunctionPointersThread(LPVOID lpParam)
{
    FunctionPointers *ptrs = static_cast<FunctionPointers *>(lpParam);
    ptrs->LoadAssemblyThread = LoadAssemblyThread;
    ptrs->InvokeMethodThread = InvokeMethodThread;
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    return TRUE;
}