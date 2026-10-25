// trainer.h
#pragma once

#include "TrainerBase.h"

class Trainer : public TrainerBase
{
public:
    Trainer() : TrainerBase(L"popcapgame1.exe") {} // x86
    ~Trainer() override = default;

    // Constants
    static inline const wchar_t *moduleName = L"popcapgame1.exe";

    inline bool setFertilizer(int newVal)
    {
        return holdValue("SetFertilizer", {moduleName, {0x00073A64, 0x44, 0x0, 0x10, 0x2C, 0x4, 0x18, 0x254}}, newVal);
    }

    inline bool setBugSpray(int newVal)
    {
        return holdValue("SetBugSpray", {moduleName, {0x00073A64, 0x44, 0x0, 0x10, 0x28, 0x4, 0x18, 0x258}}, newVal);
    }

    inline bool setChocolate(int newVal)
    {
        return holdValue("SetChocolate", {moduleName, {0x00073A64, 0x44, 0x0, 0x10, 0x28, 0x0, 0x14, 0x284}}, newVal);
    }

    inline bool addTreeFood(int newVal)
    {
        return holdValue("SetTreeFood", {moduleName, {0x0032E77C, 0x10, 0x28, 0x0, 0x4, 0x4, 0x18, 0x28C}}, newVal);
    }

    // The coin gain is `add [eax+54],edx`; the cave loads edx with the amount to add first.
    inline bool addCoin(int newVal)
    {
        return hookCode("AddCoin", CodeSite::aob(moduleName, "09 00 00 01 50 54 8B 48 54 81", 3), 6,
                        [newVal](CaveBuilder &cave)
                        {
                            cave.bytes({0x8B, 0x15}).address("amount") // mov edx,[amount]
                                .original()                            // add [eax+54],edx ; mov ecx,[eax+54]
                                .jmpBack()
                                .label("amount")
                                .value(newVal);
                        });
    }

    // The sun gain is `add [eax+00005578],ecx`; the cave loads ecx with the amount to add first.
    inline bool addSun(int newVal)
    {
        return hookCode("AddSun", CodeSite::aob(moduleName, "01 88 78 55 00 00"), 6,
                        [newVal](CaveBuilder &cave)
                        {
                            cave.bytes({0x8B, 0x0D}).address("amount") // mov ecx,[amount]
                                .original()                            // add [eax+00005578],ecx
                                .jmpBack()
                                .label("amount")
                                .value(newVal);
                        });
    }
};
