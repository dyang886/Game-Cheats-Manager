// Trainer.h
#pragma once

#include "MonoBase.h"

class Trainer : public MonoBase
{
public:
    Trainer() : MonoBase(L"DREDGE.exe") // x86
    {
    }

    virtual ~Trainer() {}

    // bool spawnItem(int itemIndex)
    // {
    //     if (!injected)
    //     {
    //         injectBridgeDLL(L"MonoBridge.dll");
    //         getFunctionPointers();
    //         loadAssembly(L"GCMInjection.dll"); // Adjust path as needed
    //         injected = true;
    //     }
    //     return invokeMethod("GCMInjection", "SpawnItem", itemIndex);
    // }
    bool spawnItem(int itemIndex)
    {
        if (!injected)
        {
            injectBridgeDLL(L"D:/Resources/Software/Game Trainers/build/bin/Debug/MonoBridge.dll");
            getFunctionPointers();
            loadAssembly(L"D:/Resources/Software/Game Trainers/build/bin/Debug/GCMInjection.dll"); // Adjust path as needed
            injected = true;
        }
        return invokeMethod("GCMInjection", "SpawnItem", itemIndex);
    }

private:
    bool injected = false;
};