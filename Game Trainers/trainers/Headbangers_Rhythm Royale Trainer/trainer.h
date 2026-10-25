// trainer.h
#pragma once

#include "TrainerBase.h"

class Trainer : public TrainerBase
{
public:
    Trainer() : TrainerBase(L"Headbangers.exe") {} // x64
    ~Trainer() override = default;

    // Constants
    static inline const wchar_t *moduleName = L"GameAssembly.dll";

    inline bool setBread(int newVal)
    {
        return holdValue("SetBread", {moduleName, {0x036E2A88, 0xA0, 0xE60, 0x48, 0x18, 0x10, 0x30, 0x20}}, newVal);
    }

    inline bool setWins(int newVal)
    {
        return holdValue("SetWins", {moduleName, {0x036E2A88, 0xA0, 0xE18, 0x188, 0x48, 0xD8, 0x678, 0x78}}, newVal);
    }

    // The XP store is `mov [rsi+20],ebx`; the cave loads ebx with the new value first.
    inline bool setXP(int newVal)
    {
        return hookCode("SetXP", CodeSite::aob(moduleName, "7A 00 84 C0 75 69 89 5E 20 48 8B 47 20", 6), 7,
                        [newVal](CaveBuilder &cave)
                        {
                            cave.ripRelative({0x8B, 0x1D}, "xp") // mov ebx,[xp]
                                .original()                      // mov [rsi+20],ebx ; mov rax,[rdi+20]
                                .jmpBack()
                                .label("xp")
                                .value(newVal);
                        });
    }
};
