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
        return holdValue("Life", {moduleName, {0x16BB04}}, newVal);
    }

    inline bool setShield(int newVal)
    {
        return holdValue("Shield", {moduleName, {0x16BB0C}}, newVal);
    }

    inline bool setNuke(int newVal)
    {
        return holdValue("Nuke", {moduleName, {0x16BB08}}, newVal);
    }

    inline bool setDefenseOrbs(int newVal)
    {
        return holdValue("DefenseOrbs", {moduleName, {0x16BB20}}, newVal);
    }

    inline bool setHomingMissile(int newVal)
    {
        return holdValue("HomingMissile", {moduleName, {0x16BB24}}, newVal);
    }

    inline bool setLaser(int newVal)
    {
        return holdValue("Laser", {moduleName, {0x16BB28}}, newVal);
    }

    inline bool setRockets(int newVal)
    {
        return holdValue("Rockets", {moduleName, {0x16BB2C}}, newVal);
    }

    inline bool setFlakCannon(int newVal)
    {
        return holdValue("FlakCannon", {moduleName, {0x16BB30}}, newVal);
    }

    inline bool setThunderStrike(int newVal)
    {
        return holdValue("ThunderStrike", {moduleName, {0x16BB34}}, newVal);
    }
};
