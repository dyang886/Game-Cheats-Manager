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
    bool godMode(bool enable)
    {
        if (initializeDllInjection())
        {
            return invokeMethod("", "GCMInjection", "GodMode", {enable});
        }
        return false;
    }

    std::string getItemList()
    {
        if (initializeDllInjection())
        {
            return invokeMethodReturn("", "GCMInjection", "GetItemList", {});
        }
        return "";
    }

    bool spawnItem(int itemIndex)
    {
        if (initializeDllInjection())
        {
            return invokeMethod("", "GCMInjection", "SpawnItem", {itemIndex});
        }
        return false;
    }

    bool addFunds(float amount)
    {
        if (initializeDllInjection())
        {
            return invokeMethod("", "GCMInjection", "AddFunds", {amount});
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

    bool restockAll()
    {
        if (initializeDllInjection())
        {
            return invokeMethod("", "GCMInjection", "RestockAll", {});
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
            return invokeMethod("", "GCMInjection", "FreezeTime", {freeze});
        }
        return false;
    }
};