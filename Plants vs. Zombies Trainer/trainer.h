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
        std::vector<unsigned int> offsets = {0x00073A64, 0x44, 0x0, 0x10, 0x2C, 0x4, 0x18, 0x254};
        return createPointerToggle(moduleName, "SetFertilizer", offsets, newVal);
    }

    inline bool setBugSpray(int newVal)
    {
        std::vector<unsigned int> offsets = {0x00073A64, 0x44, 0x0, 0x10, 0x28, 0x4, 0x18, 0x258};
        return createPointerToggle(moduleName, "SetBugSpray", offsets, newVal);
    }

    inline bool setChocolate(int newVal)
    {
        std::vector<unsigned int> offsets = {0x00073A64, 0x44, 0x0, 0x10, 0x28, 0x0, 0x14, 0x284};
        return createPointerToggle(moduleName, "SetChocolate", offsets, newVal);
    }

    inline bool setTreeFood(int newVal)
    {
        std::vector<unsigned int> offsets = {0x0032E77C, 0x10, 0x28, 0x0, 0x4, 0x4, 0x18, 0x28C};
        return createPointerToggle(moduleName, "SetTreeFood", offsets, newVal);
    }

    inline bool addCoin(int newVal)
    {
        // Target instruction bytes: 09 00 00 01 50 54 8B 48 54 81
        std::vector<std::string> bigPat = {"09", "00", "00", "01", "50", "54", "8B", "48", "54", "81"};

        size_t patternOffset = 3; // start position of target instruction
        size_t overwriteLen = 6;  // length of target instruction to overwrite
        size_t codeSize = 0x600;  // Allocate sufficient space

        // Build the hook
        auto buildFunc = [newVal, overwriteLen](uintptr_t base, uintptr_t hookAddr) -> std::vector<BYTE>
        {
            std::vector<BYTE> code(0x600, 0x90); // NOP padding

            int *pInt = reinterpret_cast<int *>(&code[0x400]);
            *pInt = newVal;

            size_t wPos = 0;

            // 2) mov edx, [absolute 32-bit address] => 8B 15 <disp32>
            code[wPos++] = 0x8B;
            code[wPos++] = 0x15;
            {
                uintptr_t absoluteAddr = base + 0x400;
                int32_t disp32 = static_cast<int32_t>(absoluteAddr);
                std::memcpy(&code[wPos], &disp32, 4);
                wPos += 4;
            }

            // add [eax+54],edx => 01 50 54
            code[wPos++] = 0x01;
            code[wPos++] = 0x50;
            code[wPos++] = 0x54;

            // mov ecx,[eax+54] => 8B 48 54
            code[wPos++] = 0x8B;
            code[wPos++] = 0x48;
            code[wPos++] = 0x54;

            // jmp hookAddr + overwriteLen => E9 <rel32>
            code[wPos++] = 0xE9;
            uintptr_t retAddr = hookAddr + overwriteLen;
            uintptr_t nextInstr = base + wPos + 4;
            int32_t rel = static_cast<int32_t>(retAddr - nextInstr);
            std::memcpy(&code[wPos], &rel, 4);
            wPos += 4;

            return code;
        };

        // Create the hook
        return createNamedHook(
            moduleName,
            "AddCoin",
            bigPat,
            patternOffset,
            overwriteLen,
            codeSize,
            buildFunc);
    }

    inline bool addSun(int newVal)
    {
        // Target instruction bytes: 01 88 78 55 00 00
        std::vector<std::string> bigPat = {"01", "88", "78", "55", "00", "00"};

        size_t patternOffset = 0; // start position of target instruction
        size_t overwriteLen = 6;  // length of target instruction to overwrite
        size_t codeSize = 0x600;  // Allocate sufficient space

        // Build the hook
        auto buildFunc = [newVal, overwriteLen](uintptr_t base, uintptr_t hookAddr) -> std::vector<BYTE>
        {
            std::vector<BYTE> code(0x600, 0x90); // NOP padding

            int *pInt = reinterpret_cast<int *>(&code[0x400]);
            *pInt = newVal;

            size_t wPos = 0;

            // 2) mov ecx, [absolute 32-bit address] => 8B 0D <disp32>
            code[wPos++] = 0x8B;
            code[wPos++] = 0x0D;
            {
                uintptr_t absoluteAddr = base + 0x400;
                int32_t disp32 = static_cast<int32_t>(absoluteAddr);
                std::memcpy(&code[wPos], &disp32, 4);
                wPos += 4;
            }

            // add [eax+00005578], ecx => 01 88 78 55 00 00
            code[wPos++] = 0x01;
            code[wPos++] = 0x88;
            code[wPos++] = 0x78;
            code[wPos++] = 0x55;
            code[wPos++] = 0x00;
            code[wPos++] = 0x00;

            // jmp hookAddr + overwriteLen => E9 <rel32>
            code[wPos++] = 0xE9;
            uintptr_t retAddr = hookAddr + overwriteLen;
            uintptr_t nextInstr = base + wPos + 4;
            int32_t rel = static_cast<int32_t>(retAddr - nextInstr);
            std::memcpy(&code[wPos], &rel, 4);
            wPos += 4;

            return code;
        };

        // Create the hook
        return createNamedHook(
            moduleName,
            "AddSun",
            bigPat,
            patternOffset,
            overwriteLen,
            codeSize,
            buildFunc);
    }
};
