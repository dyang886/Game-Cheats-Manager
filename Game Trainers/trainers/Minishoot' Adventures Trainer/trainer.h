// trainer.h
#pragma once

#include "MonoBase.h"

class Trainer : public MonoBase
{
public:
    Trainer() : MonoBase(L"Minishoot.exe") {} // x64
    virtual ~Trainer() {}

    static inline const wchar_t *moduleName = L"mono-2.0-bdwgc.dll";

    bool toggleGodMode(bool enable)
    {
        if (!initializeDllInjection())
            return false;

        return invokeMethod("", "GCMInjection", "ToggleGodMode", {enable});
    }

    bool setHealth(bool enable, int value)
    {
        if (!initializeDllInjection())
            return false;

        return invokeMethod("", "GCMInjection", "SetHealth", {enable, value});
    }

    bool toggleInfiniteEnergy(bool enable)
    {
        if (!initializeDllInjection())
            return false;

        return invokeMethod("", "GCMInjection", "ToggleInfiniteEnergy", {enable});
    }

    bool setMovementSpeedMultiplier(bool enable, float value)
    {
        if (!initializeDllInjection())
            return false;

        return invokeMethod("", "GCMInjection", "SetMovementSpeedMultiplier", {enable, value});
    }

    bool setDamageMultiplier(bool enable, float value)
    {
        if (!initializeDllInjection())
            return false;

        return invokeMethod("", "GCMInjection", "SetDamageMultiplier", {enable, value});
    }

    bool setFireRateMultiplier(bool enable, float value)
    {
        if (!initializeDllInjection())
            return false;

        return invokeMethod("", "GCMInjection", "SetFireRateMultiplier", {enable, value});
    }

    bool setBulletSpeedMultiplier(bool enable, float value)
    {
        if (!initializeDllInjection())
            return false;

        return invokeMethod("", "GCMInjection", "SetBulletSpeedMultiplier", {enable, value});
    }

    bool setFireRangeMultiplier(bool enable, float value)
    {
        if (!initializeDllInjection())
            return false;

        return invokeMethod("", "GCMInjection", "SetFireRangeMultiplier", {enable, value});
    }

    bool setSpiritPowerMultiplier(bool enable, float value)
    {
        if (!initializeDllInjection())
            return false;

        return invokeMethod("", "GCMInjection", "SetSpiritPowerMultiplier", {enable, value});
    }

    bool toggleFloatOverVoidWater(bool enable)
    {
        if (!initializeDllInjection())
            return false;

        return invokeMethod("", "GCMInjection", "ToggleFloatOverVoidWater", {enable});
    }

    bool toggleNoPowerCooldown(bool enable)
    {
        if (!initializeDllInjection())
            return false;

        return invokeMethod("", "GCMInjection", "ToggleNoPowerCooldown", {enable});
    }

    bool setStatsPoints(int value)
    {
        if (!initializeDllInjection())
            return false;

        return invokeMethod("", "GCMInjection", "SetStatsPoints", {value});
    }

    bool setSuperCrystals(int value)
    {
        if (!initializeDllInjection())
            return false;

        return invokeMethod("", "GCMInjection", "SetSuperCrystals", {value});
    }
};
