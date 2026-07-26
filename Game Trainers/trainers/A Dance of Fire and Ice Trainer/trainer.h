// trainer.h
#pragma once

#include "MonoBase.h"

class Trainer : public MonoBase
{
public:
    Trainer() : MonoBase(L"A Dance of Fire and Ice.exe") {} // x64
    virtual ~Trainer() {}

    static inline const wchar_t *moduleName = L"mono-2.0-bdwgc.dll";

    bool toggleAutoPlay(bool enable)
    {
        if (!initializeDllInjection())
            return false;

        return invokeMethod("", "GCMInjection", "ToggleAutoPlay", {enable});
    }

    bool toggleNoDeath(bool enable)
    {
        if (!initializeDllInjection())
            return false;

        return invokeMethod("", "GCMInjection", "ToggleNoDeath", {enable});
    }

    bool setGameSpeedMultiplier(bool enable, float value)
    {
        if (!initializeDllInjection())
            return false;

        return invokeMethod("", "GCMInjection", "SetGameSpeedMultiplier", {enable, value});
    }

    bool finishLevelPerfectly()
    {
        if (!initializeDllInjection())
            return false;

        return invokeMethod("", "GCMInjection", "FinishLevelPerfectly", {});
    }
};
