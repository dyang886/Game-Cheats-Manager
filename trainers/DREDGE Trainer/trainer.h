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

            dllInjected = true;
        }
        return dllInjected;
    }

    bool spawnItem(int itemIndex)
    {
        if (initializeDllInjection())
        {
            // return invokeMethod("", "GCMInjection", "Dump", std::vector<MonoBase::Param>{});
            return invokeMethod("", "GCMInjection", "SpawnItem", std::vector<MonoBase::Param>{itemIndex});
        }
        return false;
    }

private:
    bool dllInjected = false;
};