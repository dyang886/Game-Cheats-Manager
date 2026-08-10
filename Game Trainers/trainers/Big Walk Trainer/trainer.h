// trainer.h
#pragma once

#include "Il2CppBase.h"

class Trainer : public Il2CppBase
{
public:
    Trainer() : Il2CppBase(L"Big Walk.exe") {} // x64
    ~Trainer() override = default;

    // Constants
    static inline const wchar_t *moduleName = L"GameAssembly.dll";

    bool toggleAlwaysDay(bool enable)
    {
        if (initializeDllInjection())
            return invokeMethod<bool>("ToggleAlwaysDay", enable);

        return false;
    }

    bool toggleNoTripping(bool enable)
    {
        if (initializeDllInjection())
            return invokeMethod<bool>("ToggleNoTripping", enable);

        return false;
    }

    bool setMovementSpeedMultiplier(bool enable, float value)
    {
        return invokeMultiplier("SetMovementSpeedMultiplier", enable, value);
    }

    bool setJumpHeightMultiplier(bool enable, float value)
    {
        return invokeMultiplier("SetJumpHeightMultiplier", enable, value);
    }

private:
    /// Layout must match MultiplierArgs in IL2CPP.cpp
    struct MultiplierArgs
    {
        bool enable;
        float multiplier;
    };

    bool invokeMultiplier(const char *functionName, bool enable, float value)
    {
        if (initializeDllInjection())
        {
            MultiplierArgs args = {enable, value};
            return invokeMethod<MultiplierArgs>(functionName, args);
        }

        return false;
    }
};
