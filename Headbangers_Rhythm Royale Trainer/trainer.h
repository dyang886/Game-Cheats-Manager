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
        std::vector<unsigned int> offsets = {0x036E2A88, 0xA0, 0xE60, 0x48, 0x18, 0x10, 0x30, 0x20};
        return createPointerToggle(moduleName, "SetBread", offsets, newVal);
    }

    inline bool setWins(int newVal)
    {
        std::vector<unsigned int> offsets = {0x036E2A88, 0xA0, 0xE18, 0x188, 0x48, 0xD8, 0x678, 0x78};
        return createPointerToggle(moduleName, "SetWins", offsets, newVal);
    }

    inline bool setXP(int newVal)
    {
        // Target instruction bytes: 89 5E 20 48 8B 47 20
        std::vector<std::string> bigPat = {"7A", "00", "84", "C0", "75", "69", "89", "5E", "20", "48", "8B", "47", "20"};

        size_t patternOffset = 6;
        size_t overwriteLen = 7;
        size_t codeSize = 0x600;

        auto buildFunc = [newVal, overwriteLen](uintptr_t base, uintptr_t hookAddr) -> std::vector<BYTE>
        {
            std::vector<BYTE> code(0x600, 0x90);

            int *pInt = reinterpret_cast<int *>(&code[0x400]);
            *pInt = newVal;

            size_t wPos = 0;
            // mov ebx,[RIP+disp32] => 8B 1D <disp32>
            code[wPos++] = 0x8B;
            code[wPos++] = 0x1D;
            {
                uintptr_t intAddr = base + 0x400;
                uintptr_t nextInstr = base + wPos + 4;
                int64_t diff = static_cast<int64_t>(intAddr) - static_cast<int64_t>(nextInstr);
                int32_t disp32 = static_cast<int32_t>(diff);
                std::memcpy(&code[wPos], &disp32, 4);
                wPos += 4;
            }

            // mov [rsi+20],ebx => 89 5E 20
            code[wPos++] = 0x89;
            code[wPos++] = 0x5E;
            code[wPos++] = 0x20;

            // mov rax,[rdi+20] => 48 8B 47 20
            code[wPos++] = 0x48;
            code[wPos++] = 0x8B;
            code[wPos++] = 0x47;
            code[wPos++] = 0x20;

            // jmp hookAddr + overwriteLen => E9 <rel32>
            code[wPos++] = 0xE9;
            uintptr_t retAddr = hookAddr + overwriteLen;
            uintptr_t nextInstr = base + wPos + 4;
            int32_t rel = static_cast<int32_t>(retAddr - nextInstr);
            std::memcpy(&code[wPos], &rel, 4);
            wPos += 4;

            return code;
        };

        return createNamedHook(
            moduleName,
            "SetXP",
            bigPat,
            patternOffset,
            overwriteLen,
            codeSize,
            buildFunc);
    }
};
