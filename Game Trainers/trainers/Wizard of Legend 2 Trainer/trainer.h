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
        return holdValue("SetHealth", {moduleName, {0x085B21D0, 0x0, 0xBB0, 0x58, 0x310, 0x538, 0x48}}, newVal);
    }

    inline bool setMaxHealth(float newVal)
    {
        return holdValue("SetMaxHealth", {moduleName, {0x08542310, 0xC0, 0x218, 0x130, 0x310, 0x538, 0x5C}}, newVal);
    }

    inline bool setSpeed(float newVal)
    {
        return holdValue("SetSpeed", {moduleName, {0x08542310, 0x60, 0x310, 0x130, 0x3C0, 0xEC}}, newVal);
    }

    inline bool setCoins(float newVal)
    {
        return holdValue("SetCoins", {moduleName, {0x085424B0, 0x10, 0x560, 0x18, 0x20, 0x3C0, 0x258}}, newVal);
    }

    inline bool setGems(int newVal)
    {
        return holdValue("SetGems", {moduleName, {0x08542310, 0x180, 0x380, 0x40, 0x70}}, newVal);
    }

    inline bool setArcaneChromo(int newVal)
    {
        return holdValue("SetArcaneChromo", {moduleName, {0x08542310, 0x180, 0x380, 0x40, 0x40}}, newVal);
    }
};
