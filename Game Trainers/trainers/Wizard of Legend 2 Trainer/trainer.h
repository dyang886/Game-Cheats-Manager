// trainer.h
#pragma once

#include "TrainerBase.h"

class Trainer : public TrainerBase
{
public:
    Trainer() : TrainerBase(L"WOL2-Win64-Shipping.exe") {} // x64
    ~Trainer() override = default;

    // Constants
    static inline const wchar_t *moduleName = L"WOL2-Win64-Shipping.exe";

    inline bool setHealth(float newVal)
    {
        std::vector<unsigned int> offsets = {0x085B21D0, 0x0, 0xBB0, 0x58, 0x310, 0x538, 0x48};
        return createPointerToggle(moduleName, "SetHealth", offsets, newVal);
    }

    inline bool setMaxHealth(float newVal)
    {
        std::vector<unsigned int> offsets = {0x08542310, 0xC0, 0x218, 0x130, 0x310, 0x538, 0x5C};
        return createPointerToggle(moduleName, "SetMaxHealth", offsets, newVal);
    }

    inline bool setSpeed(float newVal)
    {
        std::vector<unsigned int> offsets = {0x08542310, 0x60, 0x310, 0x130, 0x3C0, 0xEC};
        return createPointerToggle(moduleName, "SetSpeed", offsets, newVal);
    }

    inline bool setCoins(float newVal)
    {
        std::vector<unsigned int> offsets = {0x085424B0, 0x10, 0x560, 0x18, 0x20, 0x3C0, 0x258};
        return createPointerToggle(moduleName, "SetCoins", offsets, newVal);
    }

    inline bool setGems(int newVal)
    {
        std::vector<unsigned int> offsets = {0x08542310, 0x180, 0x380, 0x40, 0x70};
        return createPointerToggle(moduleName, "SetGems", offsets, newVal);
    }

    inline bool setArcaneChromo(int newVal)
    {
        std::vector<unsigned int> offsets = {0x08542310, 0x180, 0x380, 0x40, 0x40};
        return createPointerToggle(moduleName, "SetArcaneChromo", offsets, newVal);
    }
};
