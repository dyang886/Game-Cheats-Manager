// trainer.h
#pragma once

#include "Il2CppBase.h"

class Trainer : public Il2CppBase
{
public:
    Trainer() : Il2CppBase(L"Dancing Line.exe") {} // x64
    ~Trainer() override = default;

    bool setStars(int value)
    {
        if (initializeDllInjection())
            return invokeMethod<int>("SetStars", value);

        return false;
    }

    bool setNotes(int value)
    {
        if (initializeDllInjection())
            return invokeMethod<int>("SetNotes", value);

        return false;
    }

    bool finishLevelPerfectly()
    {
        if (initializeDllInjection())
            return invokeMethod("FinishLevelPerfectly");

        return false;
    }

    bool enableRemovedLevels()
    {
        if (initializeDllInjection())
            return invokeMethod("EnableRemovedLevels");

        return false;
    }

    bool testRemovedLevelBundles()
    {
        if (initializeDllInjection())
            return invokeMethod("TestRemovedLevelBundles");

        return false;
    }

    bool unlockAllSkinsAndDecorations()
    {
        if (initializeDllInjection())
            return invokeMethod("UnlockAllSkinsAndDecorations");

        return false;
    }

    bool toggleNoCollision(bool enable)
    {
        if (initializeDllInjection())
            return invokeMethod<bool>("ToggleNoCollision", enable);

        return false;
    }
};
