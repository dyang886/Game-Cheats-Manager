// trainer.h
#pragma once

#include "Il2CppBase.h"

class Trainer : public Il2CppBase
{
public:
    Trainer() : Il2CppBase(L"JSB.exe") {} // x64
    ~Trainer() override = default;

    // Constants
    // static inline const wchar_t *moduleName = L"GameAssembly.dll";

    bool godMode(bool enable)
    {
        if (initializeDllInjection())
        {
            return invokeMethod<bool>("GodMode", enable);
        }
        return false;
    }

    bool addBeatPoints(int beatPoints)
    {
        if (initializeDllInjection())
        {
            return invokeMethod<int>("AddBeatPoints", beatPoints);
        }
        return false;
    }

    bool finishLevel()
    {
        if (initializeDllInjection())
        {
            return invokeMethod("FinishLevel");
        }
        return false;
    }
};