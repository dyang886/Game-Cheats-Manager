// trainer.h
#pragma once

#include "MonoBase.h"

class Trainer : public MonoBase
{
public:
    Trainer() : MonoBase(L"PunchTheWalls.exe") {} // x64
    virtual ~Trainer() {}

    static inline const wchar_t *moduleName = L"mono-2.0-bdwgc.dll";

    /// Held every frame while a save is loaded, so the shredder's income and shop spending both land
    /// back on the number in the box.
    bool setCoins(bool enable, int value, std::string &message)
    {
        return invokeCheat("SetCoins", {enable, value}, message);
    }

    /// Held, so placing a stick puts it straight back. The game's own count, so the hotbar slot appears
    /// with it.
    bool setDynamite(bool enable, int value, std::string &message)
    {
        return invokeCheat("SetDynamite", {enable, value}, message);
    }

    bool setC4(bool enable, int value, std::string &message)
    {
        return invokeCheat("SetC4", {enable, value}, message);
    }

    /// Held over whatever the capacity upgrades give, with suction scaled to match and uncapped;
    /// switching it off puts the upgrades' values back.
    bool setVacuumCapacity(bool enable, int value, std::string &message)
    {
        return invokeCheat("SetVacuumCapacity", {enable, value}, message);
    }

    /// Empties the tank. Refuses outside a loaded save rather than waiting to clear the next one.
    bool clearVacuum(std::string &message)
    {
        return invokeCheat("ClearVacuum", {}, message);
    }

    /// Breaking a wall releases no cubes; the ones already lying around stay.
    bool toggleNoDebris(bool enable, std::string &message)
    {
        return invokeCheat("ToggleNoDebris", {enable}, message);
    }

    /// The 50 second wait after a charged punch.
    bool toggleNoChargeCooldown(bool enable, std::string &message)
    {
        return invokeCheat("ToggleNoChargeCooldown", {enable}, message);
    }

    /// Scales what the shredder pays for both rock cubes and treasures; switching it off puts the
    /// game's own values back.
    bool setShredderPayoutsMultiplier(bool enable, float value, std::string &message)
    {
        return invokeCheat("SetShredderPayoutsMultiplier", {enable, value}, message);
    }

private:
    /// Every cheat answers with "OK" or the reason it refused, which the caller passes on as-is.
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
