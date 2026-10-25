// trainer.h
#pragma once

#include "TrainerBase.h"

class Trainer : public TrainerBase
{
public:
    Trainer() : TrainerBase(L"Arrow a Row.exe") {} // x64
    ~Trainer() override = default;

    // Constants
    static inline const wchar_t *moduleName = L"GameAssembly.dll";

    inline bool setCoin(int newVal)
    {
        return holdValue("Coin", {moduleName, {0x018E43C8, 0x440, 0x230, 0x230, 0x268, 0xEB8, 0x70, 0xE38}}, newVal);
    }

    inline bool setHealth(float newVal)
    {
        return holdValue("Health", {moduleName, {0x01918328, 0xC0, 0x90, 0x40, 0xB8, 0x20, 0x18, 0x28}}, newVal);
    }

    inline bool setHorizontalSpeed(float newVal)
    {
        return storeFloatHook("HorizontalSpeed", "F3 0F 11 40 20 E9 DA", MOVSS_XMM0, newVal); // movss [rax+20],xmm0
    }

    inline bool setArrowDamage(float newVal)
    {
        return storeFloatHook("ArrowDamage", "F3 0F 11 43 44 0F", MOVSS_XMM0, newVal); // movss [rbx+44],xmm0
    }

    inline bool setArrowFrequency(float newVal)
    {
        return storeFloatHook("ArrowFrequency", "F3 0F 11 73 4C", MOVSS_XMM6, newVal); // movss [rbx+4C],xmm6
    }

    inline bool setArrowSpeed(float newVal)
    {
        return storeFloatHook("ArrowSpeed", "F3 0F 11 7B 38 83", MOVSS_XMM7, newVal); // movss [rbx+38],xmm7
    }

    inline bool setArrowDistance(float newVal)
    {
        return storeFloatHook("ArrowDistance", "F3 0F 11 73 40", MOVSS_XMM6, newVal); // movss [rbx+40],xmm6
    }

    // The count is read as `mov eax,[rbx+48] ; cmp eax,0F`; the cave stores the cheat's count there
    // first and compares that instead.
    inline bool setArrowCount(int newVal)
    {
        return hookCode("ArrowCount", CodeSite::aob(moduleName, "8B 43 48 83 F8 0F"), 6,
                        [newVal](CaveBuilder &cave)
                        {
                            cave.ripRelative({0x8B, 0x05}, "count") // mov eax,[count]
                                .bytes({0x89, 0x43, 0x48})          // mov [rbx+48],eax
                                .original(3, 3)                     // cmp eax,0F
                                .jmpBack()
                                .label("count")
                                .value(newVal);
                        });
    }

    inline bool setSwordDamage(float newVal)
    {
        return storeFloatHook("SwordDamage", "F3 44 0F 11 00", MOVSS_XMM8, newVal); // movss [rax],xmm8
    }

    inline bool setSwordCoolDown(float newVal)
    {
        return storeFloatHook("SwordCoolDown", "F3 0F 11 79 04", MOVSS_XMM7, newVal); // movss [rcx+04],xmm7
    }

    inline bool setSwordSpeed(float newVal)
    {
        return storeFloatHook("SwordSpeed", "F3 0F 11 41 14 48 8B 05", MOVSS_XMM0, newVal); // movss [rcx+14],xmm0
    }

    inline bool setSwordDistance(float newVal)
    {
        return storeFloatHook("SwordDistance", "F3 0F 11 71 08", MOVSS_XMM6, newVal); // movss [rcx+08],xmm6
    }

private:
    // movss xmmN,[rip+disp32] for the register the hooked store writes from
    static inline const std::vector<BYTE> MOVSS_XMM0 = {0xF3, 0x0F, 0x10, 0x05};
    static inline const std::vector<BYTE> MOVSS_XMM6 = {0xF3, 0x0F, 0x10, 0x35};
    static inline const std::vector<BYTE> MOVSS_XMM7 = {0xF3, 0x0F, 0x10, 0x3D};
    static inline const std::vector<BYTE> MOVSS_XMM8 = {0xF3, 0x44, 0x0F, 0x10, 0x05};

    // Each float stat is stored by a 5-byte `movss [reg+off],xmmN`. The cave loads xmmN with the
    // cheat's value and then runs the store, so the stat is written with that value every time.
    inline bool storeFloatHook(const std::string &name, const std::string &signature, const std::vector<BYTE> &loadOpcode, float value)
    {
        return hookCode(name, CodeSite::aob(moduleName, signature), 5,
                        [&loadOpcode, value](CaveBuilder &cave)
                        {
                            cave.ripRelative(loadOpcode, "value") // movss xmmN,[value]
                                .original()                       // the store
                                .jmpBack()
                                .label("value")
                                .value(value);
                        });
    }
};
