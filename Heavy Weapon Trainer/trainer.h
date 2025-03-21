// trainer.h
#pragma once

#include "TrainerBase.h"

class Trainer : public TrainerBase
{
public:
    Trainer() : TrainerBase(L"Heavy Weapon Deluxe 1.0", true) {} // x86
    ~Trainer() override = default;

    static inline const wchar_t *moduleName = L"$re$^pop[A-Za-z0-9]+\\.tmp$";

    inline bool setLife(int newVal)
    {
        std::vector<unsigned int> offsets = {0x16BB04};
        return createPointerToggle(moduleName, "Life", offsets, newVal);
    }

    inline bool setShield(int newVal)
    {
        std::vector<unsigned int> offsets = {0x16BB0C};
        return createPointerToggle(moduleName, "Shield", offsets, newVal);
    }

    inline bool setNuke(int newVal)
    {
        std::vector<unsigned int> offsets = {0x16BB08};
        return createPointerToggle(moduleName, "Nuke", offsets, newVal);
    }

    inline bool setDefenseOrbs(int newVal)
    {
        std::vector<unsigned int> offsets = {0x16BB20};
        return createPointerToggle(moduleName, "DefenseOrbs", offsets, newVal);
    }

    inline bool setHomingMissile(int newVal)
    {
        std::vector<unsigned int> offsets = {0x16BB24};
        return createPointerToggle(moduleName, "HomingMissile", offsets, newVal);
    }

    inline bool setLaser(int newVal)
    {
        std::vector<unsigned int> offsets = {0x16BB28};
        return createPointerToggle(moduleName, "Laser", offsets, newVal);
    }

    inline bool setRockets(int newVal)
    {
        std::vector<unsigned int> offsets = {0x16BB2C};
        return createPointerToggle(moduleName, "Rockets", offsets, newVal);
    }

    inline bool setFlakCannon(int newVal)
    {
        std::vector<unsigned int> offsets = {0x16BB30};
        return createPointerToggle(moduleName, "FlakCannon", offsets, newVal);
    }

    inline bool setThunderStrike(int newVal)
    {
        std::vector<unsigned int> offsets = {0x16BB34};
        return createPointerToggle(moduleName, "ThunderStrike", offsets, newVal);
    }
};
