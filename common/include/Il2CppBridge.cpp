/**
 * ==============================================================================
 * IL2CPP BRIDGE (Il2CppBridge.cpp) - SINGLE FILE
 * ==============================================================================
 *
 * This is a generic, self-contained DLL to be injected into an IL2CPP game.
 * It is the "server" and contains all the Aetherim library code.
 *
 * It is structured just like your MonoBridge.cpp.
 * It exports C-style functions that the "client" (.exe) will call.
 */

// --- Standard Windows Includes ---
#include <Windows.h>
#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include <atomic>
#include <mutex>

// --- Aetherim Library ---
// Includes the Aetherim headers from a subfolder, as requested.
#include "Aetherim/wrapper.hpp"

// --- Contract Structs ---
// These structs must be defined *identically* in Il2CppBase.h
// This is the "contract" between the client (.exe) and server (.dll).

// Defines the function pointers we will send to the client.
struct Bridge_FunctionPointers
{
    LPVOID pBridge_Initialize;
    LPVOID pBridge_InvokeMethod;
};

// Defines the layout of the parameters we receive from the client.
struct Bridge_InvokeMethod_Params
{
    const char *imageName;
    const char *namespaceName;
    const char *className;
    const char *methodName;
    int argCount;
    void **args; // This points to an array of pointers
};

/**
 * ==============================================================================
 * IL2CPP ENGINE (Internal Class)
 * ==============================================================================
 *
 * This is the "engine" that runs *inside* the injected DLL.
 * It manages the Aetherim library, initializes IL2CPP,
 * and provides helper functions to find classes and methods.
 * It is NOT exported and is only used internally by this file.
 */
class Il2CppEngine
{
public:
    Il2CppEngine() : m_isInitialized(false) {}
    ~Il2CppEngine() = default;

    /**
     * @brief Ensures IL2CPP is initialized, thread-safe.
     * @return True if initialized, false on failure.
     */
    bool ensureInitialized()
    {
        if (m_isInitialized)
            return true;

        std::lock_guard<std::mutex> lock(m_initMutex);
        if (m_isInitialized)
            return true;

        try
        {
            // This is the Aetherim call that must be run inside the game
            // It finds GameAssembly.dll in its *own* process space.
            Il2cpp::initialize();
        }
        catch (const std::exception &e)
        {
            std::cerr << "[!] Il2cpp::initialize() failed: " << e.what() << std::endl;
            return false;
        }

        m_il2cpp_wrapper = std::make_unique<Wrapper>();
        if (!m_il2cpp_wrapper)
        {
            std::cerr << "[!] Failed to create IL2CPP Wrapper." << std::endl;
            return false;
        }

        m_isInitialized = true;
        return true;
    }

    /**
     * @brief Gets the initialization status.
     */
    bool isInitialized() const
    {
        return m_isInitialized;
    }

    /**
     * @brief Finds a class by name.
     */
    Class *findClass(const char *imageName, const char *namespaceName, const char *className)
    {
        if (!m_isInitialized)
            return nullptr;
        Image *pImage = m_il2cpp_wrapper->get_image(imageName);
        if (!pImage)
        {
            std::cerr << "[!] findClass: Failed to find image: " << imageName << std::endl;
            return nullptr;
        }
        Class *pClass = pImage->get_class(className, namespaceName);
        if (!pClass)
        {
            std::cerr << "[!] findClass: Failed to find class: " << namespaceName << "." << className << std::endl;
        }
        return pClass;
    }

    /**
     * @brief Finds a method by name.
     */
    Method *findMethod(Class *pClass, const char *methodName, int argCount)
    {
        if (!m_isInitialized || !pClass)
            return nullptr;
        Method *pMethod = pClass->get_method(methodName, argCount);
        if (!pMethod)
        {
            std::cerr << "[!] findMethod: Failed to find method: " << methodName << std::endl;
        }
        return pMethod;
    }

    /**
     * @brief Gets a singleton instance of a class.
     */
    void *getSingletonInstance(Class *pClass)
    {
        if (!pClass)
            return nullptr;

        // Common singleton patterns
        Field *pField = pClass->get_field("instance");
        if (!pField)
            pField = pClass->get_field("Instance");
        if (!pField)
            pField = pClass->get_field("_instance");

        if (pField)
        {
            return pField->get_as_static();
        }

        std::cerr << "[!] getSingletonInstance: Failed to find common 'instance' field." << std::endl;
        return nullptr;
    }

private:
    std::unique_ptr<Wrapper> m_il2cpp_wrapper;
    std::atomic<bool> m_isInitialized;
    std::mutex m_initMutex;
};

// --- Global Engine Instance ---
std::unique_ptr<Il2CppEngine> g_pIl2CppEngine;

/**
 * @brief Thread entry point for initialization.
 */
DWORD WINAPI InitializeThread(LPVOID lpParam)
{
    // Optional: Allocate a console for debugging
    // AllocConsole();
    // FILE* pFile;
    // freopen_s(&pFile, "CONOUT$", "w", stdout);
    // freopen_s(&pFile, "CONOUT$", "w", stderr);

    std::cout << "[+] Il2CppBridge DLL Injected. Initializing engine..." << std::endl;

    // Create the "engine" object.
    g_pIl2CppEngine = std::make_unique<Il2CppEngine>();

    // Call ensureInitialized()
    // This calls Il2cpp::initialize() and creates the wrapper
    if (g_pIl2CppEngine->ensureInitialized())
    {
        std::cout << "[+] IL2CPP Engine Initialized successfully." << std::endl;
        return 1; // Return 1 for success
    }
    else
    {
        std::cerr << "[!] Failed to initialize IL2CPP Engine." << std::endl;
        return 0; // Return 0 for failure
    }
}

/**
 * @brief DllMain: The entry point for the DLL.
 */
BOOL APIENTRY DllMain(HMODULE hModule,
                      DWORD ul_reason_for_call,
                      LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        // Do nothing here to prevent deadlocks.
        // The client will explicitly call Bridge_Initialize.
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    case DLL_PROCESS_DETACH:
        // Clean up when the DLL is unloaded or process detaches
        g_pIl2CppEngine.reset();
        break;
    }
    return TRUE;
}

//
// ==============================================================================
// --- EXPORTED FUNCTIONS (THE C-STYLE API FOR THE TRAINER) ---
// ==============================================================================
//
//  We must wrap all exported functions in extern "C"
//  to prevent C++ name mangling, which causes the LNK2001 error.
//

// --- FORWARD DECLARATIONS ---
// We must declare these functions *before* Bridge_GetFunctionPointers
// to fix the C2065 "undeclared identifier" compiler error.
extern "C" __declspec(dllexport) DWORD WINAPI Bridge_Initialize(LPVOID lpParam);
extern "C" __declspec(dllexport) void *Bridge_InvokeMethod(LPVOID lpParam);

/**
 * @brief Client calls this to get addresses of other functions.
 * (This is the robust MonoBase.h pattern)
 */
extern "C" __declspec(dllexport) DWORD WINAPI Bridge_GetFunctionPointers(LPVOID lpParam)
{
    // lpParam is a pointer to a Bridge_FunctionPointers struct
    // that the client wants us to fill.
    if (!lpParam)
    {
        return 0; // Failure
    }

    Bridge_FunctionPointers *pOutPointers = (Bridge_FunctionPointers *)lpParam;

    // Assign the function names directly, just like in MonoBridge.cpp
    pOutPointers->pBridge_Initialize = (LPVOID)Bridge_Initialize;
    pOutPointers->pBridge_InvokeMethod = (LPVOID)Bridge_InvokeMethod;

    return 0; // Return 0 for success, just like MonoBridge.cpp
}

/**
 * @brief Initializes the IL2CPP engine by creating a thread.
 * Exported for the client .exe to call.
 */
extern "C" __declspec(dllexport) DWORD WINAPI Bridge_Initialize(LPVOID lpParam)
{
    // Create a thread to do the initialization
    HANDLE hThread = CreateThread(nullptr, 0, InitializeThread, nullptr, 0, nullptr);
    if (hThread)
    {
        // Wait for the thread to finish
        WaitForSingleObject(hThread, INFINITE);
        DWORD exitCode = 0;
        GetExitCodeThread(hThread, &exitCode);
        CloseHandle(hThread);
        return exitCode; // Will be 1 for success, 0 for failure
    }
    return 0; // Failed to create thread
}

/**
 * @brief A generic function to invoke any IL2CPP method.
 * Exported for the client .exe to call.
 */
extern "C" __declspec(dllexport) void *Bridge_InvokeMethod(LPVOID lpParam)
{
    // We expect lpParam to be a pointer to a Bridge_InvokeMethod_Params
    // structure that the .exe has written into the game's memory.
    Bridge_InvokeMethod_Params *pParams = (Bridge_InvokeMethod_Params *)lpParam;

    // This check is CRITICAL to prevent crashes.
    if (!g_pIl2CppEngine || !g_pIl2CppEngine->isInitialized())
    {
        std::cerr << "[!] Bridge_InvokeMethod called before initialization." << std::endl;
        return nullptr;
    }

    if (!pParams)
    {
        std::cerr << "[!] Bridge_InvokeMethod called with NULL parameters." << std::endl;
        return nullptr;
    }

    try
    {
        // Find the class
        Class *pClass = g_pIl2CppEngine->findClass(pParams->imageName, pParams->namespaceName, pParams->className);
        if (!pClass)
            return nullptr;

        // Find the method
        Method *pMethod = g_pIl2CppEngine->findMethod(pClass, pParams->methodName, pParams->argCount);
        if (!pMethod)
            return nullptr;

        // Invoke
        void *pInstance = nullptr;
        if (pMethod->is_instance())
        {
            pInstance = g_pIl2CppEngine->getSingletonInstance(pClass);
            if (!pInstance)
            {
                std::cerr << "[!] Method is instance, but failed to get singleton." << std::endl;
                return nullptr;
            }
            // We cast the const away, which is acceptable for bridge invoking
            return const_cast<void *>(pMethod->invoke(pInstance, pParams->args));
        }
        else // Method is static
        {
            return pMethod->invoke_static(pParams->args);
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "[!] Exception in Bridge_InvokeMethod: " << e.what() << std::endl;
        return nullptr;
    }
    catch (...)
    {
        std::cerr << "[!] Unknown exception in Bridge_InvokeMethod." << std::endl;
        return nullptr;
    }
}
