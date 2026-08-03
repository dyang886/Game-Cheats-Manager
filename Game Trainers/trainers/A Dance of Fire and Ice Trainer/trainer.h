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

    bool toggleGodMode(bool enable)
    {
        if (!initializeDllInjection())
            return false;

        return invokeMethod("", "GCMInjection", "ToggleGodMode", {enable});
    }

    bool setGameSpeedMultiplier(bool enable, float value)
    {
        if (!initializeDllInjection())
            return false;

        return invokeMethod("", "GCMInjection", "SetGameSpeedMultiplier", {enable, value});
    }

    /// Apply actions reply with "OK", or with a message for the user. The message is a translation
    /// key owned by the injected assembly, so the caller only has to run it through t().
    bool finishLevelPerfectly(std::string &message)
    {
        return invokeApply("FinishLevelPerfectly", message);
    }

private:
    bool invokeApply(const std::string &methodName, std::string &message)
    {
        message.clear();

        if (!initializeDllInjection())
            return false;

        const std::string response = invokeMethodReturn("", "GCMInjection", methodName, {});
        if (response == "OK")
            return true;

        message = response;
        return false;
    }
};
