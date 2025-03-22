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

    bool spawnItem(int itemIndex)
    {
        if (initializeDllInjection())
        {
            return invokeMethod("", "GCMInjection", "SpawnItem", std::vector<MonoBase::Param>{itemIndex});
        }
        return false;
    }
};