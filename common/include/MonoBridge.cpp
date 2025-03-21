#include <windows.h>
#include <string>
#include <fstream>

#define MONO_API extern "C" __declspec(dllimport)

// Define Mono function prototypes
typedef void* MonoDomain;
typedef void* MonoAssembly;
typedef void* MonoImage;
typedef void* MonoClass;
typedef void* MonoMethod;
typedef void* MonoObject;
typedef void* MonoThread;
typedef void* MonoString;

MONO_API MonoDomain* mono_get_root_domain();
MONO_API MonoThread* mono_thread_attach(MonoDomain* domain);
MONO_API MonoAssembly* mono_assembly_open(const char* filename, void* status);
MONO_API MonoAssembly* mono_domain_assembly_open(MonoDomain* domain, const char* name);
MONO_API MonoImage* mono_assembly_get_image(MonoAssembly* assembly);
MONO_API MonoClass* mono_class_from_name(MonoImage* image, const char* name_space, const char* name);
MONO_API MonoMethod* mono_class_get_method_from_name(MonoClass* klass, const char* name, int param_count);
MONO_API MonoObject* mono_runtime_invoke(MonoMethod* method, void* obj, void** params, MonoObject** exc);
MONO_API MonoString* mono_object_to_string(MonoObject* obj, MonoObject** exc);
MONO_API char* mono_string_to_utf8(MonoString* str);
MONO_API void mono_free(void* ptr);

// Structure for function pointers and parameters (unchanged)
struct FunctionPointers { LPVOID LoadAssemblyThread; LPVOID InvokeMethodThread; };
struct LoadAssemblyParams { const char* path; };
struct InvokeMethodParams { const char* className; const char* methodName; int param; };

// Forward declarations (unchanged)
extern "C" __declspec(dllexport) DWORD WINAPI LoadAssemblyThread(LPVOID lpParam);
extern "C" __declspec(dllexport) DWORD WINAPI InvokeMethodThread(LPVOID lpParam);
extern "C" __declspec(dllexport) DWORD WINAPI GetFunctionPointersThread(LPVOID lpParam);

// Global variables
MonoDomain* domain = nullptr;

// Logging function
void LogToFile(const char* message) {
    std::ofstream logFile("D:\\MonoBridge_log.txt", std::ios::app);
    if (logFile.is_open()) {
        logFile << "[MonoBridge] " << message << std::endl;
        logFile.close();
    }
}

// Initialize Mono domain
void InitializeMono() {
    if (domain) return;
    HMODULE monoModule = GetModuleHandleA("mono-2.0-bdwgc.dll");
    if (!monoModule) { LogToFile("Failed to find mono-2.0-bdwgc.dll"); return; }
    auto getRootDomain = (MonoDomain*(*)())GetProcAddress(monoModule, "mono_get_root_domain");
    auto threadAttach = (MonoThread*(*)(MonoDomain*))GetProcAddress(monoModule, "mono_thread_attach");
    if (!getRootDomain || !threadAttach) { LogToFile("Failed to get Mono functions"); return; }
    domain = getRootDomain();
    if (!domain) { LogToFile("Failed to get root domain"); return; }
    threadAttach(domain);
    LogToFile("Mono domain initialized and thread attached");
}

// Load C# assembly (unchanged)
void LoadAssembly(const char* path) {
    InitializeMono();
    if (!domain) return;
    HMODULE monoModule = GetModuleHandleA("mono-2.0-bdwgc.dll");
    auto assemblyOpen = (MonoAssembly*(*)(const char*, void*))GetProcAddress(monoModule, "mono_assembly_open");
    MonoAssembly* assembly = assemblyOpen(path, nullptr);
    if (!assembly) { LogToFile("Failed to load assembly"); } else { LogToFile("Assembly loaded successfully"); }
}

// Invoke static method
void InvokeMethod(const char* className, const char* methodName, int param) {
    InitializeMono();
    if (!domain) { LogToFile("Mono domain not initialized"); return; }

    HMODULE monoModule = GetModuleHandleA("mono-2.0-bdwgc.dll");
    if (!monoModule) { LogToFile("Failed to get mono-2.0-bdwgc.dll handle"); return; }

    auto domainAssemblyOpen = (MonoAssembly*(*)(MonoDomain*, const char*))GetProcAddress(monoModule, "mono_domain_assembly_open");
    auto getImage = (MonoImage*(*)(MonoAssembly*))GetProcAddress(monoModule, "mono_assembly_get_image");
    auto classFromName = (MonoClass*(*)(MonoImage*, const char*, const char*))GetProcAddress(monoModule, "mono_class_from_name");
    auto getMethod = (MonoMethod*(*)(MonoClass*, const char*, int))GetProcAddress(monoModule, "mono_class_get_method_from_name");
    auto runtimeInvoke = (MonoObject*(*)(MonoMethod*, void*, void**, MonoObject**))GetProcAddress(monoModule, "mono_runtime_invoke");
    auto objectToString = (MonoString*(*)(MonoObject*, MonoObject**))GetProcAddress(monoModule, "mono_object_to_string");
    auto stringToUtf8 = (char*(*)(MonoString*))GetProcAddress(monoModule, "mono_string_to_utf8");
    auto freeMono = (void(*)(void*))GetProcAddress(monoModule, "mono_free");

    MonoAssembly* assembly = domainAssemblyOpen(domain, "D:/Resources/Software/Game Trainers/build/bin/Debug/GCMInjection.dll");
    if (!assembly) { LogToFile("Failed to open assembly"); return; }

    MonoImage* image = getImage(assembly);
    if (!image) { LogToFile("Failed to get image"); return; }

    MonoClass* klass = classFromName(image, "", className); // No namespace, as confirmed
    if (!klass) { LogToFile("Failed to find class"); return; }

    MonoMethod* method = getMethod(klass, methodName, 1);
    if (!method) { LogToFile("Failed to find method"); return; }

    void* args[1] = { &param };
    MonoObject* exc = nullptr;
    MonoObject* result = runtimeInvoke(method, nullptr, args, &exc);
    if (exc) {
        MonoString* excStr = objectToString(exc, nullptr);
        if (excStr) {
            char* excMsg = stringToUtf8(excStr);
            LogToFile("Exception during invocation: ");
            LogToFile(excMsg);
            freeMono(excMsg);
        } else {
            LogToFile("Exception occurred but failed to get message");
        }
    } else {
        LogToFile("Method invoked successfully");
    }
}

// Exported functions (unchanged)
extern "C" __declspec(dllexport) DWORD WINAPI GetFunctionPointersThread(LPVOID lpParam) {
    FunctionPointers* ptrs = static_cast<FunctionPointers*>(lpParam);
    ptrs->LoadAssemblyThread = LoadAssemblyThread;
    ptrs->InvokeMethodThread = InvokeMethodThread;
    return 0;
}

extern "C" __declspec(dllexport) DWORD WINAPI LoadAssemblyThread(LPVOID lpParam) {
    LoadAssemblyParams* params = static_cast<LoadAssemblyParams*>(lpParam);
    LoadAssembly(params->path);
    return 0;
}

extern "C" __declspec(dllexport) DWORD WINAPI InvokeMethodThread(LPVOID lpParam) {
    InvokeMethodParams* params = static_cast<InvokeMethodParams*>(lpParam);
    HMODULE monoModule = GetModuleHandleA("mono-2.0-bdwgc.dll");
    if (monoModule) {
        auto threadAttach = (MonoThread*(*)(MonoDomain*))GetProcAddress(monoModule, "mono_thread_attach");
        if (threadAttach && domain) {
            threadAttach(domain);
            LogToFile("Thread attached to Mono domain");
        }
    }
    InvokeMethod(params->className, params->methodName, params->param);
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        LogToFile("MonoBridge.dll injected");
    }
    return TRUE;
}