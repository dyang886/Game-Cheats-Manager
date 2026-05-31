#include <Windows.h>
#include <cstring>
#include <string>
#include <vector>

using MonoDomain = void;
using MonoAssembly = void;
using MonoImage = void;
using MonoClass = void;
using MonoMethod = void;
using MonoObject = void;
using MonoString = void;
using MonoThread = void;

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

struct SharedMemoryHeader
{
    volatile size_t head;
    volatile size_t tail;
};

struct MonoApi
{
    MonoDomain *(*getRootDomain)();
    MonoThread *(*threadAttach)(MonoDomain *);
    MonoAssembly *(*assemblyOpen)(const char *, void *);
    MonoImage *(*getImage)(MonoAssembly *);
    MonoClass *(*classFromName)(MonoImage *, const char *, const char *);
    MonoMethod *(*getMethod)(MonoClass *, const char *, int);
    MonoObject *(*runtimeInvoke)(MonoMethod *, void *, void **, MonoObject **);
    MonoString *(*stringNew)(MonoDomain *, const char *);
    MonoClass *(*objectGetClass)(MonoObject *);
    char *(*stringToUtf8)(MonoString *);
    void (*monoFree)(void *);
};

static constexpr size_t kSharedBufferSize = 1024 * 1024;

static MonoDomain *g_domain = nullptr;
static MonoAssembly *g_loadedAssembly = nullptr;
static HANDLE g_loggingMapFile = nullptr;
static LPVOID g_loggingBuffer = nullptr;
static HANDLE g_responseMapFile = nullptr;
static LPVOID g_responseBuffer = nullptr;

static HMODULE GetMonoModule()
{
    return GetModuleHandleA("mono-2.0-bdwgc.dll");
}

template <typename T>
static T GetMonoProc(HMODULE module, const char *name)
{
    return reinterpret_cast<T>(GetProcAddress(module, name));
}

static bool ResolveMonoApi(MonoApi &api)
{
    HMODULE monoModule = GetMonoModule();
    if (!monoModule)
        return false;

    api.getRootDomain = GetMonoProc<MonoDomain *(*)()>(monoModule, "mono_get_root_domain");
    api.threadAttach = GetMonoProc<MonoThread *(*)(MonoDomain *)>(monoModule, "mono_thread_attach");
    api.assemblyOpen = GetMonoProc<MonoAssembly *(*)(const char *, void *)>(monoModule, "mono_assembly_open");
    api.getImage = GetMonoProc<MonoImage *(*)(MonoAssembly *)>(monoModule, "mono_assembly_get_image");
    api.classFromName = GetMonoProc<MonoClass *(*)(MonoImage *, const char *, const char *)>(monoModule, "mono_class_from_name");
    api.getMethod = GetMonoProc<MonoMethod *(*)(MonoClass *, const char *, int)>(monoModule, "mono_class_get_method_from_name");
    api.runtimeInvoke = GetMonoProc<MonoObject *(*)(MonoMethod *, void *, void **, MonoObject **)>(monoModule, "mono_runtime_invoke");
    api.stringNew = GetMonoProc<MonoString *(*)(MonoDomain *, const char *)>(monoModule, "mono_string_new");
    api.objectGetClass = GetMonoProc<MonoClass *(*)(MonoObject *)>(monoModule, "mono_object_get_class");
    api.stringToUtf8 = GetMonoProc<char *(*)(MonoString *)>(monoModule, "mono_string_to_utf8");
    api.monoFree = GetMonoProc<void (*)(void *)>(monoModule, "mono_free");

    return api.getRootDomain && api.threadAttach && api.assemblyOpen && api.getImage &&
           api.classFromName && api.getMethod && api.runtimeInvoke && api.stringNew;
}

static LPVOID OpenSharedBuffer(HANDLE &mapping, const std::string &name)
{
    if (mapping)
        return MapViewOfFile(mapping, FILE_MAP_ALL_ACCESS, 0, 0, 0);

    mapping = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, name.c_str());
    if (!mapping)
        return nullptr;

    LPVOID buffer = MapViewOfFile(mapping, FILE_MAP_ALL_ACCESS, 0, 0, 0);
    if (!buffer)
    {
        CloseHandle(mapping);
        mapping = nullptr;
    }
    return buffer;
}

extern "C" __declspec(dllexport) void SendData(const char *message)
{
    if (!message)
        return;

    if (!g_loggingBuffer)
    {
        std::string name = "TrainerLoggingSharedMemory_" + std::to_string(GetCurrentProcessId());
        g_loggingBuffer = OpenSharedBuffer(g_loggingMapFile, name);
        if (!g_loggingBuffer)
            return;
    }

    const size_t length = std::strlen(message);
    if (length == 0 || length + sizeof(size_t) > kSharedBufferSize)
        return;

    auto *header = static_cast<SharedMemoryHeader *>(g_loggingBuffer);
    char *bufferStart = static_cast<char *>(g_loggingBuffer) + sizeof(SharedMemoryHeader);
    const size_t capacity = kSharedBufferSize - sizeof(SharedMemoryHeader);
    const size_t itemSize = sizeof(size_t) + length;
    if (itemSize > capacity)
        return;

    if (header->head + itemSize > capacity)
    {
        if (header->head + sizeof(size_t) <= capacity)
            *reinterpret_cast<size_t *>(bufferStart + header->head) = 0;
        header->head = 0;
    }

    size_t nextHead = header->head + itemSize;
    if (nextHead >= capacity)
        nextHead = 0;

    if ((header->head < header->tail && nextHead >= header->tail) || nextHead == header->tail)
        return;

    *reinterpret_cast<size_t *>(bufferStart + header->head) = length;
    std::memcpy(bufferStart + header->head + sizeof(size_t), message, length);
    header->head = nextHead;
}

static void Log(const char *message)
{
    if (!message)
        return;

    std::string formatted = "[MonoBridge] ";
    formatted += message;
    SendData(formatted.c_str());
}

extern "C" __declspec(dllexport) void SendResponse(const char *message)
{
    if (!message)
    {
        Log("[!] Null response message");
        return;
    }

    if (!g_responseBuffer)
    {
        std::string name = "TrainerResponseSharedMemory_" + std::to_string(GetCurrentProcessId());
        g_responseBuffer = OpenSharedBuffer(g_responseMapFile, name);
        if (!g_responseBuffer)
        {
            Log("[!] Could not open response shared memory");
            return;
        }
    }

    const size_t length = std::strlen(message);
    if (length + sizeof(size_t) > kSharedBufferSize)
    {
        Log("[!] Response too large for buffer");
        return;
    }

    *reinterpret_cast<size_t *>(g_responseBuffer) = length;
    std::memcpy(static_cast<char *>(g_responseBuffer) + sizeof(size_t), message, length);

    std::string eventName = "TrainerResponseEvent_" + std::to_string(GetCurrentProcessId());
    HANDLE event = OpenEventA(EVENT_MODIFY_STATE, FALSE, eventName.c_str());
    if (!event)
    {
        Log("[!] Could not open response event");
        return;
    }

    SetEvent(event);
    CloseHandle(event);
}

static bool InitializeMono()
{
    if (g_domain)
        return true;

    MonoApi api{};
    if (!ResolveMonoApi(api))
    {
        Log("[!] Failed to resolve Mono API");
        return false;
    }

    g_domain = api.getRootDomain();
    if (!g_domain)
    {
        Log("[!] Failed to get Mono root domain");
        return false;
    }

    api.threadAttach(g_domain);
    return true;
}

static bool LoadAssembly(const char *path)
{
    if (!path || !InitializeMono())
        return false;

    MonoApi api{};
    if (!ResolveMonoApi(api))
    {
        Log("[!] Failed to resolve Mono API");
        return false;
    }

    g_loadedAssembly = api.assemblyOpen(path, nullptr);
    if (!g_loadedAssembly)
    {
        Log("[!] Failed to load assembly");
        return false;
    }

    return true;
}

extern "C" __declspec(dllexport) DWORD WINAPI LoadAssemblyThread(LPVOID lpParam)
{
    auto *params = static_cast<LoadAssemblyParams *>(lpParam);
    return LoadAssembly(params ? params->path : nullptr) ? 0 : 1;
}

static void LogManagedException(MonoObject *exc, const MonoApi &api)
{
    if (!exc || !api.objectGetClass || !api.getMethod || !api.runtimeInvoke || !api.stringToUtf8)
    {
        Log("[!] Exception during method invocation");
        return;
    }

    MonoClass *excClass = api.objectGetClass(exc);
    MonoMethod *toString = excClass ? api.getMethod(excClass, "ToString", 0) : nullptr;
    if (!toString)
    {
        Log("[!] Exception during method invocation; failed to resolve exception ToString()");
        return;
    }

    MonoObject *nestedExc = nullptr;
    MonoObject *strObj = api.runtimeInvoke(toString, exc, nullptr, &nestedExc);
    if (!strObj || nestedExc)
    {
        Log("[!] Exception during method invocation; exception ToString() failed");
        return;
    }

    char *utf8 = api.stringToUtf8(reinterpret_cast<MonoString *>(strObj));
    if (!utf8)
    {
        Log("[!] Exception during method invocation; exception string conversion failed");
        return;
    }

    std::string message = "[!] Exception during method invocation: ";
    message += utf8;
    Log(message.c_str());

    if (api.monoFree)
        api.monoFree(utf8);
}

extern "C" __declspec(dllexport) DWORD WINAPI InvokeMethodThread(LPVOID lpParam)
{
    auto *params = static_cast<InvokeMethodParams *>(lpParam);
    if (!params || !InitializeMono())
        return 1;

    MonoApi api{};
    if (!ResolveMonoApi(api))
    {
        Log("[!] Failed to resolve Mono API");
        return 1;
    }

    api.threadAttach(g_domain);

    if (!g_loadedAssembly)
    {
        Log("[!] No assembly loaded");
        return 1;
    }

    MonoImage *image = api.getImage(g_loadedAssembly);
    if (!image)
    {
        Log("[!] Failed to get assembly image");
        return 1;
    }

    MonoClass *klass = api.classFromName(image, params->namespaceName, params->className);
    if (!klass)
    {
        Log("[!] Failed to find class");
        return 1;
    }

    MonoMethod *method = api.getMethod(klass, params->methodName, params->paramCount);
    if (!method)
    {
        Log("[!] Failed to find method");
        return 1;
    }

    std::vector<void *> args(params->paramCount);
    std::vector<MonoString *> stringParams;
    for (int i = 0; i < params->paramCount; ++i)
    {
        ParamValue &pv = params->paramValues[i];
        switch (pv.type)
        {
        case ParamValue::INT:
            args[i] = &pv.i;
            break;
        case ParamValue::FLOAT:
            args[i] = &pv.f;
            break;
        case ParamValue::STRING:
        {
            MonoString *monoString = api.stringNew(g_domain, pv.s);
            if (!monoString)
            {
                Log("[!] Failed to create MonoString");
                return 1;
            }
            stringParams.push_back(monoString);
            args[i] = monoString;
            break;
        }
        default:
            Log("[!] Unsupported parameter type");
            return 1;
        }
    }

    MonoObject *exc = nullptr;
    api.runtimeInvoke(method, nullptr, args.empty() ? nullptr : args.data(), &exc);
    if (exc)
    {
        LogManagedException(exc, api);
        return 1;
    }

    return 0;
}

extern "C" __declspec(dllexport) DWORD WINAPI GetFunctionPointersThread(LPVOID lpParam)
{
    auto *ptrs = static_cast<FunctionPointers *>(lpParam);
    if (!ptrs)
        return 1;

    ptrs->LoadAssemblyThread = LoadAssemblyThread;
    ptrs->InvokeMethodThread = InvokeMethodThread;
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID reserved)
{
    UNREFERENCED_PARAMETER(hModule);
    UNREFERENCED_PARAMETER(reserved);

    if (reason == DLL_PROCESS_DETACH)
    {
        if (g_loggingBuffer)
            UnmapViewOfFile(g_loggingBuffer);
        if (g_loggingMapFile)
            CloseHandle(g_loggingMapFile);
        if (g_responseBuffer)
            UnmapViewOfFile(g_responseBuffer);
        if (g_responseMapFile)
            CloseHandle(g_responseMapFile);
    }

    return TRUE;
}
