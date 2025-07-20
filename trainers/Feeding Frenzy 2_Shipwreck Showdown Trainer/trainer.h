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

    inline bool setLives(int newVal)
    {
        std::vector<unsigned int> offsets = {0x001AC624, 0x3C, 0x10};
        return createPointerToggle(moduleName, "SetLives", offsets, newVal);
    }

    inline bool setGrowth(int newVal)
    {
        std::vector<unsigned int> offsets = {0x001AC624, 0x3C, 0x40};
        return WriteToDynamicAddress(moduleName, offsets, newVal);
    }
};
