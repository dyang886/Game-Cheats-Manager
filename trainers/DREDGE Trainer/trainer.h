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

    // DLL injection methods
    bool spawnItem(int itemIndex)
    {
        if (initializeDllInjection())
        {
            return invokeMethod("", "GCMInjection", "SpawnItem", std::vector<MonoBase::Param>{itemIndex});
        }
        return false;
    }

    bool addFunds(float amount)
    {
        if (initializeDllInjection())
        {
            return invokeMethod("", "GCMInjection", "AddFunds", std::vector<MonoBase::Param>{amount});
        }
        return false;
    }

    bool repairAll()
    {
        if (initializeDllInjection())
        {
            return invokeMethod("", "GCMInjection", "RepairAll", {});
        }
        return false;
    }

    bool clearWeather()
    {
        if (initializeDllInjection())
        {
            return invokeMethod("", "GCMInjection", "ClearWeather", {});
        }
        return false;
    }

    bool freezeTime(bool freeze)
    {
        if (initializeDllInjection())
        {
            return invokeMethod("", "GCMInjection", "FreezeTime", std::vector<MonoBase::Param>{freeze});
        }
        return false;
    }
};