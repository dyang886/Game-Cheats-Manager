// trainer.h
#pragma once

#include "TrainerBase.h"

class Trainer : public TrainerBase
{
public:
    Trainer() : TrainerBase(L"mio.exe") {} // x64
    ~Trainer() override = default;

    static inline const wchar_t *moduleName = L"mio.exe";

    // Infinite Health: force the damage branch at mio.exe+0x9FA61C.
    inline bool infiniteHealth()
    {
        return patchCode("InfiniteHealth", CodeSite::aob(moduleName, "0F 85 ?? ?? ?? ?? 41 F6 46 1C 14 0F 84 A5 00 00"), {0x90, 0xE9});
    }

    // Unlimited Modifier Allocation: force GetUsedAllocation() (mio.exe+0x9E1C80) to return 0,
    // so the Allocation matrix (max = level*10+25, e.g. 75) is never full
    inline bool unlimitedModifierAllocation()
    {
        return patchCode("UnlimitedModifierAllocation",
                         CodeSite::aob(moduleName, "41 57 41 56 41 54 56 57 53 48 83 EC 48 48 8B 05 ?? ?? ?? ?? 48 31 E0 48 89 44 24 40 48 B8 07 00 00 00 01 00 00 00"),
                         {0x31, 0xC0, 0xC3}); // xor eax,eax ; ret  (used = 0)
    }

    // Infinite Jumps: keep the jump-used flag at [rcx+0x8D0] cleared.
    inline bool infiniteJumps()
    {
        return hookCode("InfiniteJumps", CodeSite::aob(moduleName, "80 B9 D0 08 00 00 00 75 12 80 BE 08 14 00 00 00"), 7,
                        [](CaveBuilder &cave)
                        {
                            cave.bytes({0xC6, 0x81, 0xD0, 0x08, 0x00, 0x00, 0x00}) // mov byte ptr [rcx+8D0],0
                                .original()                                     // cmp byte ptr [rcx+8D0],0
                                .jmpBack();
                        });
    }

    // Infinite Stamina: pin the static float at mio.exe+0x11119A8 to 100.0.
    inline bool infiniteStamina()
    {
        return holdValue("InfiniteStamina", {moduleName, {0x11119A8}}, 100.0f);
    }

    // Movement Speed Multiplier: both player position integrators do `pos += xmm6` where xmm6 is
    // this frame's move delta (velocity*dt). We hook each and insert `mulss xmm6,[mult]` first -
    // mulss scales only the LOW float (X), so horizontal speed is multiplied while fall/jump/
    // climb stay normal. mult=1.0 = normal.
    inline bool movementSpeedMultiplier(float mult)
    {
        auto scaleDelta = [mult](CaveBuilder &cave)
        {
            cave.ripRelative({0xF3, 0x0F, 0x59, 0x35}, "mult") // mulss xmm6,[mult]
                .original()                                   // the integrator's movsd + addps
                .jmpBack()
                .label("mult")
                .value(mult);
        };
        // integrator A: movsd xmm0,[rbx+18]; addps xmm0,xmm6; movlps [rbx+18],xmm0; ... (extended for uniqueness)
        if (!hookCode("MovementSpeedMultiplierH1",
                      CodeSite::aob(moduleName, "F2 0F 10 43 18 0F 58 C6 0F 13 43 18 0F 57 C0 F3 0F 58 43 20 F3 0F 11 43 20 48 8B 9C 24 F8 04 00 00"), 8,
                      scaleDelta))
            return false;
        // integrator B: movsd xmm0,[rsi]; addps xmm6,xmm0; movlps [rsi],xmm6
        if (!hookCode("MovementSpeedMultiplierH2", CodeSite::aob(moduleName, "F2 0F 10 06 0F 58 F0 0F 13 36"), 7, scaleDelta))
        {
            disable("MovementSpeedMultiplierH1");
            return false;
        }
        return true;
    }

    inline bool setNacreDroplets(int newVal)
    {
        return writeValue(PointerChain{moduleName, {0x1116BF8, 0x754}}, newVal);
    }

    inline bool setCrystallizedNacre(int newVal)
    {
        return writeValue(PointerChain{moduleName, {0x1116BF8, 0xDD24}}, newVal);
    }

    inline bool setOldCore(int newVal)
    {
        return writeValue(PointerChain{moduleName, {0x1116BF8, 0x15F14}}, newVal);
    }
};
