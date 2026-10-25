// trainer.h
#pragma once

#include "TrainerBase.h"

class Trainer : public TrainerBase
{
public:
    Trainer() : TrainerBase(L"Outland.exe") {} // x86
    ~Trainer() override = default;

    // Constants
    static inline const wchar_t *moduleName = L"Outland.exe";

    inline bool setHealth(float newVal)
    {
        return holdValue("SetHealth", {moduleName, {0x005024F0, 0x5C, 0x14, 0x0, 0x14, 0x18, 0x3C}}, newVal);
    }

    inline bool setMaxHealth(float newVal)
    {
        return holdValue("SetMaxHealth", {moduleName, {0x005024F0, 0x5C, 0x14, 0x0, 0x14, 0x18, 0x40}}, newVal);
    }

    inline bool setCoins(int newVal)
    {
        return holdValue("SetCoins", {moduleName, {0x004C72A0, 0x4C, 0x44, 0x0, 0x4, 0x14, 0x0, 0xF8}}, newVal);
    }
};
