// trainer.h
#pragma once

#include "TrainerBase.h"

// Ratchet & Clank: Rift Apart v3.630.1.0 (Steam build 23987138). Every site below was found and
// verified live with the native discovery pipeline - see docs/native_trainer_discovery.md. The AOBs
// locate code; the RVAs are data this build stores at fixed addresses and move with any update.
class Trainer : public TrainerBase
{
public:
    Trainer() : TrainerBase(L"RiftApart.exe") {} // x64

    static inline const wchar_t *moduleName = L"RiftApart.exe";

    // Bolts: the game reads the count through a getter keyed by the bolts item hash, many times a
    // frame. Writing the value at its `mov eax,[rax+38]` keeps the count there. The getter is one of
    // three byte-identical currency getters, so the signature runs into the code after it.
    bool setBolts(bool enable, int value)
    {
        if (!enable)
            return disable("SetBolts");
        return currencyHook("SetBolts", "8B 40 38 48 83 C4 28 C3 48 83 C4 28 C3 CC CC CC CC CC CC CC CC CC CC CC CC CC CC "
                                        "48 8B 05 ?? ?? ?? ?? 48 89 01 48 8B C1 C3 CC CC 48 8B 05",
                            value);
    }

    // Raritanium: the same getter shape, keyed by the raritanium hash.
    bool setRaritanium(bool enable, int value)
    {
        if (!enable)
            return disable("SetRaritanium");
        return currencyHook("SetRaritanium", "8B 40 38 48 83 C4 28 C3 48 83 C4 28 C3 CC CC CC CC CC CC CC CC CC CC CC CC CC CC "
                                             "48 8B 05 ?? ?? ?? ?? 48 89 01 48 8B C1 C3 CC CC 48 89 5C 24 08",
                            value);
    }

    // Ammo: weapons keep it as a float at item+0x498 behind a handle, and the HUD asks the current
    // weapon for it every frame through GetAmmo. Writing the value where GetAmmo has the resolved
    // item in rax keeps the equipped weapon at it.
    bool setAmmo(bool enable, int value)
    {
        if (!enable)
            return disable("SetAmmo");

        const float ammo = static_cast<float>(value);
        return hookCode("SetAmmo", CodeSite::aob(moduleName, "48 8B C8 48 8B 5C 24 30 48 83 C4 20 5F E9 ?? ?? ?? ?? 48 8B 5C 24 30 0F 57 C0"), 8,
                        [ammo](CaveBuilder &cave)
                        {
                            cave.bytes({0xC7, 0x80, 0x98, 0x04, 0x00, 0x00}).value(ammo) // mov dword ptr [rax+498],ammo
                                .original()                                             // mov rcx,rax ; mov rbx,[rsp+30]
                                .jmpBack();
                        });
    }

    // Infinite Health: HeroHealth::SetHealth clamps the incoming value to [0, max] before comparing
    // it with the current health (+0x88). Clamping it to the current value as well means damage
    // changes nothing - so the death checks that follow never fire - while healing still works. The
    // function is shared with the base Health class, so the cave only acts on HeroHealth.
    bool toggleInfiniteHealth(bool enable)
    {
        if (!enable)
            return disable("InfiniteHealth");

        const uintptr_t heroHealth = moduleAddress(moduleName, HERO_HEALTH_VTABLE);
        return hookCode("InfiniteHealth", CodeSite::aob(moduleName, "F3 0F 5F F1 F3 0F 5D B3 80 00 00 00"), 12,
                        [heroHealth](CaveBuilder &cave)
                        {
                            cave.original() // maxss xmm6,xmm1 ; minss xmm6,[rbx+80]
                                .vtableCheck(CaveBuilder::RBX, heroHealth)
                                .jumpTo(CaveBuilder::Jcc::NE, "done")
                                .bytes({0xF3, 0x0F, 0x5F, 0xB3, 0x88, 0x00, 0x00, 0x00}) // maxss xmm6,[rbx+88]
                                .label("done")
                                .jmpBack();
                        });
    }

    // God Mode: damage reaches the hero as packets through HeroHealth::ReceiveDamagePacket. Rejecting
    // every packet that is not a heal (type 5) leaves enemies attacking normally while nothing lands -
    // no damage, no hit reaction. Making the hero immune instead would stop enemies attacking at all.
    bool toggleGodMode(bool enable)
    {
        if (!enable)
            return disable("GodMode");

        const uintptr_t heroHealth = moduleAddress(moduleName, HERO_HEALTH_VTABLE);
        return hookCode("GodMode", CodeSite::aob(moduleName, "48 89 74 24 10 57 48 83 EC 20 48 8B F9 48 8B F2 48 8B 49 60 48 85 C9 75 41"), 10,
                        [heroHealth](CaveBuilder &cave)
                        {
                            cave.vtableCheck(CaveBuilder::RCX, heroHealth)
                                .jumpTo(CaveBuilder::Jcc::NE, "original")
                                .bytes({0x83, 0xBA, 0x3C, 0x01, 0x00, 0x00, 0x05}) // cmp dword ptr [rdx+13C],5
                                .jumpTo(CaveBuilder::Jcc::E, "original")
                                .bytes({0x32, 0xC0, 0xC3})                         // xor al,al ; ret
                                .label("original")
                                .original() // mov [rsp+10],rsi ; push rdi ; sub rsp,20
                                .jmpBack();
                        });
    }

    // Unlimited Jumps: two limits stack. The transition manager rejects a jump whose index is past
    // the hero's maximum (1 - the double jump); clamping the requested index to the maximum instead
    // makes every further press another double jump. And each jump's launch speed is computed to
    // reach a fixed height above where the jump chain took off, which caps chained jumps at that
    // height; dropping the subtraction gives every jump its full height from wherever it starts.
    bool toggleUnlimitedJumps(bool enable)
    {
        if (!enable)
        {
            bool cap = disable("UnlimitedJumps");
            bool height = disable("UnlimitedJumpsHeight");
            return cap && height;
        }

        bool cap = hookCode("UnlimitedJumps", CodeSite::aob(moduleName, "41 0F B6 46 19 3B 82 FC 06 00 00"), 11,
                            [](CaveBuilder &cave)
                            {
                                cave.bytes({0x41, 0x0F, 0xB6, 0x46, 0x19})       // movzx eax,byte ptr [r14+19]
                                    .bytes({0x3B, 0x82, 0xFC, 0x06, 0x00, 0x00}) // cmp eax,[rdx+6FC]
                                    .jumpTo(CaveBuilder::Jcc::BE, "within")
                                    .bytes({0x8B, 0x82, 0xFC, 0x06, 0x00, 0x00}) // mov eax,[rdx+6FC]
                                    .bytes({0x41, 0x88, 0x46, 0x19})             // mov [r14+19],al
                                    .label("within")
                                    .bytes({0x3B, 0x82, 0xFC, 0x06, 0x00, 0x00}) // cmp eax,[rdx+6FC] - the ja after it no longer rejects
                                    .jmpBack();
                            });
        if (!cap)
            return false;

        if (!patchCode("UnlimitedJumpsHeight", CodeSite::aob(moduleName, "F3 41 0F 5C CB F3 0F 59 C8 0F 57 C0 F3 41 0F 5F CC"),
                       {0x90, 0x90, 0x90, 0x90, 0x90})) // subss xmm1,xmm11 -> nop
        {
            disable("UnlimitedJumps");
            return false;
        }
        return true;
    }

    // Movement Speed Multiplier: ground speed comes from a run-state tunable (1.2 in this build) and
    // hoverboot sprint from the active HeroHoverbootConfig's min/max speed. Both are scaled. Scaling
    // the position step itself does not work in this game: locomotion feeds the measured speed back
    // into the next frame's step, so any extra distance compounds every frame.
    bool setMovementSpeedMultiplier(bool enable, float multiplier, std::string &message)
    {
        if (!enable)
        {
            bool run = disable("MovementSpeedRun");
            bool sprint = disable("MovementSpeedSprint");
            return run && sprint;
        }

        if (!scaleValues("MovementSpeedSprint", HOVERBOOT_CONFIG, {0x30, 0x34}, multiplier))
        {
            message = "Load a save first.";
            return false;
        }
        if (!scaleValues("MovementSpeedRun", RUN_SPEED_SCALE, {0x0}, multiplier))
        {
            disable("MovementSpeedSprint");
            return false;
        }
        return true;
    }

private:
    // RVAs into RiftApart.exe for this build
    static constexpr uintptr_t HERO_HEALTH_VTABLE = 0x2F569B8; // HeroHealth's vtable (RTTI)
    static constexpr uintptr_t LOCAL_HERO = 0x6003FE0;         // Hero::HeroLocal *
    static inline const PointerChain RUN_SPEED_SCALE = {moduleName, {0x61D04D0}}; // float, run-state speed tunable
    // local hero -> +0x90 HeroPointerCache -> +0x70 HeroConfigManager -> +0x6E8 active HeroHoverbootConfig
    static inline const PointerChain HOVERBOOT_CONFIG = {moduleName, {LOCAL_HERO, 0x90, 0x70, 0x6E8, 0x0}};

    // A currency getter ends `mov eax,[rax+38] ; add rsp,28 ; ret`, with rax the item's count block.
    bool currencyHook(const std::string &name, const std::string &signature, int value)
    {
        return hookCode(name, CodeSite::aob(moduleName, signature), 7,
                        [value](CaveBuilder &cave)
                        {
                            cave.bytes({0xC7, 0x40, 0x38}).value(value) // mov dword ptr [rax+38],value
                                .original()                             // mov eax,[rax+38] ; add rsp,28
                                .jmpBack();
                        });
    }
};
