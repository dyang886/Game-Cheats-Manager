#pragma once
#include <Windows.h>
#include "memory.h"
#include "MinHook.h"

struct HOOK {
    void* target_func;
    void* detour_func;
    void* original_func;
    bool active;

    // Constructor matches usage in il2cpp.h
    HOOK(MEMORY mem, void* target, void* detour) {
        this->target_func = target;
        this->detour_func = detour;
        this->original_func = nullptr;
        this->active = false;

        // Ensure MinHook is initialized (safe to call multiple times)
        static bool initialized = false;
        if (!initialized) {
            MH_Initialize();
            initialized = true;
        }
    }

    // Applies the hook
    void load() {
        if (MH_CreateHook(target_func, detour_func, &original_func) == MH_OK) {
            if (MH_EnableHook(target_func) == MH_OK) {
                active = true;
            }
        }
    }

    // Helper to get the original function pointer to call inside the hook
    template <class T>
    T get_original() {
        return (T)original_func;
    }

    // Destructor to clean up
    ~HOOK() {
        if (active) {
            MH_DisableHook(target_func);
        }
    }
};