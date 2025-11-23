#pragma once
#include <Windows.h>
#include <vector>

class MEMORY
{
public:
    // Find a DLL module base address
    void *find_module(const char *name)
    {
        return (void *)GetModuleHandleA(name);
    }

    // Find a function address (exported or internal)
    template <typename T>
    T find_function(void *module, const char *name)
    {
        return (T)GetProcAddress((HMODULE)module, name);
    }

    // Simple pattern scan helper (Optional, but often used in these cheats)
    uintptr_t find_pattern(const char *module_name, const char *pattern)
    {
        // Implementation omitted for brevity as your current code
        // seems to rely on GetProcAddress/IL2CPP API mostly.
        // If you need pattern scanning later, standard sigscan code goes here.
        return 0;
    }
};