# Creating a C++ Trainer with DLL Injection for Unity Mono Games: A Detailed Journey

## Overview

The goal was to build a **C++ trainer** capable of injecting a DLL into a Unity game (DREDGE.exe), interfacing with its Mono runtime, and executing remote function calls in a C# assembly (GCMInjection.dll) to manipulate in-game behavior (e.g., spawning items). This involved:

- Injecting a custom C++ DLL (MonoBridge.dll) into the game process.
- Loading a C# assembly (GCMInjection.dll) into the game’s Mono runtime.
- Invoking a method (SpawnItem) with a parameter (e.g., 273) to spawn an item in the game.

The process required understanding Windows process injection, the Mono runtime, Unity’s threading model, and integrating C++ with C# code. Here’s how we did it.

------

## Step 1: Project Setup and Architecture

### Objective

Create a trainer that works with DREDGE.exe, an x86 Unity game using the Mono runtime.

### Tools and Prerequisites

- **Visual Studio Code with CMake**: For building the C++ components.
- **CMake**: To manage the build process for the trainer and MonoBridge.dll.
- **.NET SDK**: To compile the C# assembly (GCMInjection.dll).
- **Windows API**: For process manipulation and DLL injection.
- **Mono API**: To interact with the game’s C# runtime.

### Project Structure

- **Trainer Executable (DREDGE Trainer.exe)**: The main C++ program that injects the DLL and triggers remote calls.
- **MonoBridge DLL (MonoBridge.dll)**: A C++ DLL injected into DREDGE.exe to bridge the native and managed (C#) worlds.
- **GCMInjection DLL (GCMInjection.dll)**: A C# assembly containing the game logic (e.g., SpawnItem).
- **CMakeLists.txt**: Configures the build process for all components.

#### CMakeLists.txt

```cmake
set(GAME_NAME DREDGE)

# Define MonoBridge DLL
add_library(MonoBridge SHARED 
    ${CMAKE_SOURCE_DIR}/common/include/MonoBridge.cpp
    ${CMAKE_SOURCE_DIR}/common/include/MonoBridge.def
)
set_target_properties(MonoBridge PROPERTIES 
    OUTPUT_NAME "MonoBridge"
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Define C# compilation
set(CSHARP_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/GCMInjection.cs")
set(CSHARP_DLL "${CMAKE_BINARY_DIR}/bin/GCMInjection.dll")
add_custom_command(
    OUTPUT ${CSHARP_DLL}
    COMMAND "dotnet" build "${CMAKE_CURRENT_SOURCE_DIR}/GCMInjection.csproj" -c Release -o "${CMAKE_BINARY_DIR}/bin"
    DEPENDS ${CSHARP_SOURCE} "${CMAKE_CURRENT_SOURCE_DIR}/GCMInjection.csproj"
    COMMENT "Compiling GCMInjection.cs to GCMInjection.dll"
)
add_custom_target(GCMInjection ALL DEPENDS ${CSHARP_DLL})

# Define the trainer executable
add_executable(${GAME_NAME} main.cpp)
target_link_libraries(${GAME_NAME} PRIVATE Common)
add_dependencies(${GAME_NAME} MonoBridge GCMInjection)
set_target_properties(${GAME_NAME} PROPERTIES 
    OUTPUT_NAME "DREDGE Trainer"
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
```

- **Architecture**: Set to x86 via CMake configuration (-A Win32) to match DREDGE.exe.

------

## Step 2: Writing the Trainer (main.cpp and MonoBase.h)

### Objective

Develop a C++ executable to inject MonoBridge.dll into DREDGE.exe and call functions remotely.

#### main.cpp

```c++
#include "MonoBase.h"
#include <windows.h>

int main() {
    MonoBase trainer(L"DREDGE.exe");
    std::wstring dllPath = L"D:\\Resources\\Software\\Game Trainers\\build\\bin\\Debug\\MonoBridge.dll";

    while (!trainer.isProcessRunning()) {
        printf("Waiting for DREDGE.exe...\n");
        Sleep(1000);
    }

    if (!trainer.injectBridgeDLL(dllPath)) {
        printf("DLL injection failed.\n");
        return 1;
    }

    if (!trainer.getFunctionPointers()) {
        printf("Failed to get function pointers.\n");
        return 1;
    }

    std::wstring assemblyPath = L"D:\\Resources\\Software\\Game Trainers\\build\\bin\\Debug\\GCMInjection.dll";
    if (!trainer.loadAssembly(assemblyPath)) {
        printf("Failed to load assembly.\n");
        return 1;
    }

    if (!trainer.invokeMethod("GCMInjection", "SpawnItem", 273)) {
        printf("Failed to invoke method.\n");
        return 1;
    }

    printf("Trainer executed successfully.\n");
    return 0;
}
```

- **Flow**: Wait for DREDGE.exe, inject MonoBridge.dll, retrieve function pointers, load GCMInjection.dll, and invoke SpawnItem.

#### MonoBase.h (Simplified Key Functions)

```c++
class MonoBase {
    HANDLE hProcess;
    std::wstring bridgeDllPath;
    uintptr_t bridgeDllBase;
    struct FunctionPointers { LPVOID LoadAssemblyThread; LPVOID InvokeMethodThread; } funcPtrs;

public:
    MonoBase(const std::wstring& processName) {
        // Open process handle to DREDGE.exe
    }

    bool injectBridgeDLL(const std::wstring& dllPath) {
        // Use CreateRemoteThread and LoadLibraryW to inject MonoBridge.dll
        LPVOID allocMem = VirtualAllocEx(hProcess, nullptr, dllPath.size() * 2 + 2, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        WriteProcessMemory(hProcess, allocMem, dllPath.c_str(), dllPath.size() * 2 + 2, nullptr);
        HANDLE hThread = CreateRemoteThread(hProcess, nullptr, 0, (LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "LoadLibraryW"), allocMem, 0, nullptr);
        WaitForSingleObject(hThread, INFINITE);
        DWORD exitCode;
        GetExitCodeThread(hThread, &exitCode);
        bridgeDllBase = exitCode;
        CloseHandle(hThread);
        VirtualFreeEx(hProcess, allocMem, 0, MEM_RELEASE);
        return bridgeDllBase != 0;
    }

    bool getFunctionPointers() {
        // Load MonoBridge.dll locally, calculate RVA, and retrieve function pointers remotely
    }

    bool loadAssembly(const std::wstring& path) {
        // Call LoadAssemblyThread remotely
    }

    bool invokeMethod(const char* className, const char* methodName, int param) {
        // Call InvokeMethodThread remotely with parameters
    }
};
```

- **Injection**: Uses classic DLL injection with CreateRemoteThread and LoadLibraryW.
- **Remote Calls**: Executes functions in MonoBridge.dll via remote threads.

------

## Step 3: Implementing the Bridge DLL (MonoBridge.cpp)

### Objective

Create MonoBridge.dll to interface with the Mono runtime and execute C# code.

#### Initial Version

```c++
#include <windows.h>

typedef void* MonoDomain;
typedef void* MonoAssembly;
typedef void* MonoImage;
typedef void* MonoClass;
typedef void* MonoMethod;
typedef void* MonoObject;
typedef void* MonoThread;

extern "C" __declspec(dllimport) MonoDomain* mono_get_root_domain();
extern "C" __declspec(dllimport) MonoThread* mono_thread_attach(MonoDomain* domain);
// ... other Mono API declarations ...

struct FunctionPointers { LPVOID LoadAssemblyThread; LPVOID InvokeMethodThread; };
struct LoadAssemblyParams { const char* path; };
struct InvokeMethodParams { const char* className; const char* methodName; int param; };

MonoDomain* domain = nullptr;

void InitializeMono() {
    if (domain) return;
    HMODULE monoModule = GetModuleHandleA("mono-2.0-bdwgc.dll");
    auto getRootDomain = (MonoDomain*(*)())GetProcAddress(monoModule, "mono_get_root_domain");
    auto threadAttach = (MonoThread*(*)(MonoDomain*))GetProcAddress(monoModule, "mono_thread_attach");
    domain = getRootDomain();
    threadAttach(domain);
}

void LoadAssembly(const char* path) {
    InitializeMono();
    auto assemblyOpen = (MonoAssembly*(*)(const char*, void*))GetProcAddress(GetModuleHandleA("mono-2.0-bdwgc.dll"), "mono_assembly_open");
    assemblyOpen(path, nullptr);
}

void InvokeMethod(const char* className, const char* methodName, int param) {
    InitializeMono();
    auto domainAssemblyOpen = (MonoAssembly*(*)(MonoDomain*, const char*))GetProcAddress(GetModuleHandleA("mono-2.0-bdwgc.dll"), "mono_domain_assembly_open");
    auto getImage = (MonoImage*(*)(MonoAssembly*))GetProcAddress(GetModuleHandleA("mono-2.0-bdwgc.dll"), "mono_assembly_get_image");
    auto classFromName = (MonoClass*(*)(MonoImage*, const char*, const char*))GetProcAddress(GetModuleHandleA("mono-2.0-bdwgc.dll"), "mono_class_from_name");
    auto getMethod = (MonoMethod*(*)(MonoClass*, const char*, int))GetProcAddress(GetModuleHandleA("mono-2.0-bdwgc.dll"), "mono_class_get_method_from_name");
    auto runtimeInvoke = (MonoObject*(*)(MonoMethod*, void*, void**, MonoObject**))GetProcAddress(GetModuleHandleA("mono-2.0-bdwgc.dll"), "mono_runtime_invoke");

    MonoAssembly* assembly = domainAssemblyOpen(domain, "D:/Resources/Software/Game Trainers/build/bin/Debug/GCMInjection.dll");
    MonoImage* image = getImage(assembly);
    MonoClass* klass = classFromName(image, "", className);
    MonoMethod* method = getMethod(klass, methodName, 1);
    void* args[1] = { ¶m };
    MonoObject* exc = nullptr;
    runtimeInvoke(method, nullptr, args, &exc);
}

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
    InvokeMethod(params->className, params->methodName, params->param);
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    return TRUE;
}
```

- **Purpose**: Acts as a bridge to load GCMInjection.dll and call SpawnItem using Mono API functions.

------

## Step 4: Writing the C# Assembly (GCMInjection.cs)

### Objective

Define the game logic to spawn items in DREDGE.

#### GCMInjection.cs

```c#
using UnityEngine;

public class GCMInjection
{
    public static void SpawnItem(int iindex)
    {
        int i = 0;
        foreach (ItemData itemData in GameManager.Instance.ItemManager.allItems)
        {
            if (i == iindex)
            {
                SpatialItemInstance spatialItemInstance = GameManager.Instance.ItemManager.CreateItem<SpatialItemInstance>(itemData);
                Vector3Int position;
                if (GameManager.Instance.SaveData.Inventory.FindPositionForObject(spatialItemInstance.GetItemData<SpatialItemData>(), out position, 0, false))
                {
                    GameManager.Instance.ItemManager.AddObjectToInventory(spatialItemInstance, position, true);
                }
            }
            i++;
        }
    }
}
```

- **Logic**: Iterates through allItems, spawns the item at index iindex (e.g., 273), and adds it to the inventory.

------

## Step 5: Challenges and Debugging

### Initial Issues

1. DLL Injection Failure:

   - **Problem**: LoadLibraryW failed with exit code 0.
   - **Fix**: Used absolute paths for MonoBridge.dll and ensured DREDGE.exe was running.

2. Function Pointer Retrieval:

   - **Problem**: GetProcAddress couldn’t find GetFunctionPointersThread.

   - Fix: Discovered name decoration (_GetFunctionPointersThread@4) via dumpbin /exports. Switched to a .def file (MonoBridge.def) to export undecorated names:

     ```
     LIBRARY MonoBridge
     EXPORTS
         GetFunctionPointersThread
         LoadAssemblyThread
         InvokeMethodThread
     ```

3. Game Crash:

   - **Problem**: Despite successful injection and invocation, DREDGE.exe crashed at mono_runtime_invoke.
   - **Symptoms**: Stack trace showed the crash in InvokeMethod (line 144).

#### Debugging Process

- Logging: Added file logging to MonoBridge.cpp (LogToFile) to track execution:

  ```c++
  void LogToFile(const char* message) {
      std::ofstream logFile("D:\\MonoBridge_log.txt", std::ios::app);
      if (logFile.is_open()) {
          logFile << "[MonoBridge] " << message << std::endl;
          logFile.close();
      }
  }
  ```

- **Output**: Confirmed injection and assembly loading succeeded, but the crash persisted.

------

## Step 6: Resolving the Crash

### Root Cause: Threading Conflict

- **Discovery**: The crash occurred because InvokeMethodThread ran on a separate thread (via CreateRemoteThread), but Unity requires game logic to execute on the main thread or a thread attached to the Mono runtime.
- **Evidence**: Consistent stack traces pointing to mono_runtime_invoke and no logged exceptions.

### Final Fix

#### Updated InvokeMethodThread

```c++
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
```

- **Change**: Added mono_thread_attach to attach the invocation thread to the Mono domain, aligning it with Unity’s threading model.
- **Result**: The crash disappeared, and SpawnItem executed successfully, spawning the item in DREDGE.

------

## Final Implementation

### Workflow

1. **Trainer Launches**: DREDGE Trainer.exe waits for DREDGE.exe and injects MonoBridge.dll.
2. **DLL Injection**: MonoBridge.dll is loaded into DREDGE.exe’s process space.
3. **Function Pointers**: The trainer retrieves addresses of LoadAssemblyThread and InvokeMethodThread.
4. **Assembly Loading**: MonoBridge.dll loads GCMInjection.dll into the Mono runtime.
5. **Method Invocation**: InvokeMethodThread attaches to the Mono domain and calls GCMInjection.SpawnItem(273).

### Output

- Terminal:

  ```
  Attempting to inject DLL: D:/Resources/Software/Game Trainers/build/bin/Debug/MonoBridge.dll
  Bridge DLL injected successfully at base: 0x7BC10000.
  Attempting to retrieve function pointers.
  Function pointers retrieved successfully.
  Attempting to load assembly: D:/Resources/Software/Game Trainers/build/bin/Debug/GCMInjection.dll
  Assembly load thread completed.
  Attempting to invoke GCMInjection.SpawnItem with param 273
  Method invocation thread completed.
  ```

- MonoBridge Log:

  ```
  [MonoBridge] MonoBridge.dll injected
  [MonoBridge] MonoBridge.dll injected
  [MonoBridge] Mono domain initialized and thread attached
  [MonoBridge] Assembly loaded successfully
  [MonoBridge] Thread attached to Mono domain
  ```

------

## Key Lessons Learned

1. **DLL Injection**: Classic Windows API techniques (CreateRemoteThread, LoadLibraryW) work for Unity games but require careful path and permission handling.
2. **Mono Integration**: Interfacing with Mono requires dynamic function loading (GetProcAddress) and proper domain management.
3. **Threading**: Unity’s Mono runtime demands thread attachment (mono_thread_attach) for safe execution of game logic.
4. **Debugging**: Logging (to files or DebugView) and tools like dumpbin and dnSpy were critical for diagnosing issues.

------

## Conclusion

We successfully built a C++ trainer that injects MonoBridge.dll into DREDGE.exe, loads GCMInjection.dll, and invokes SpawnItem to manipulate the game. The journey involved overcoming injection failures, export mismatches, and a threading-related crash, solved by attaching the invocation thread to the Mono domain. This foundation can now be extended to other Unity Mono games by adapting the method names, parameters, and game-specific logic in GCMInjection.dll.