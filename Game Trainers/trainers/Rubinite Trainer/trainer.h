// trainer.h
#pragma once

#include "MonoBase.h"

class Trainer : public MonoBase
{
public:
    Trainer() : MonoBase(L"Rubinite.exe") {} // x64
    virtual ~Trainer() {}

    static inline const wchar_t *moduleName = L"mono-2.0-bdwgc.dll";

    bool toggleGodMode(bool enable, std::string &message)
    {
        return invokeCheat("ToggleGodMode", {enable}, message);
    }

    bool toggleInfiniteHealth(bool enable, std::string &message)
    {
        return invokeCheat("ToggleInfiniteHealth", {enable}, message);
    }

    bool setMoveSpeed(bool enable, float value, std::string &message)
    {
        return invokeCheat("SetMoveSpeed", {enable, value}, message);
    }

    bool setDamageMultiplier(bool enable, float value, std::string &message)
    {
        return invokeCheat("SetDamageMultiplier", {enable, value}, message);
    }

    bool setTalismanSlots(bool enable, int value, std::string &message)
    {
        return invokeCheat("SetTalismanSlots", {enable, value}, message);
    }

    bool setBloodDust(int value, std::string &message)
    {
        return invokeCheat("SetBloodDust", {value}, message);
    }

    bool setImpureCrystal(int value, std::string &message)
    {
        return invokeCheat("SetImpureCrystal", {value}, message);
    }

private:
    /// Every cheat, toggle and apply alike, answers on the response channel with "OK" or with a
    /// message for the user. The message is a translation key owned by the injected assembly, so the
    /// caller only has to run it through t().
    bool invokeCheat(const std::string &methodName, const std::vector<Param> &params, std::string &message)
    {
        message.clear();

        if (!initializeDllInjection())
            return false;

        const std::string response = invokeMethodReturn("", "GCMInjection", methodName, params);
        if (response == "OK")
            return true;

        message = response;
        return false;
    }
};
