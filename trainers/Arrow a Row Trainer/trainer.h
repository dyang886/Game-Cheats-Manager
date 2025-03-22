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

    // Pointer-toggle approach
    inline bool setCoin(int newVal)
    {
        std::vector<unsigned int> offsets = {0x018E43C8, 0x440, 0x230, 0x230, 0x268, 0xEB8, 0x70, 0xE38};
        return createPointerToggle(moduleName, "Coin", offsets, newVal);
    }

    inline bool setHealth(float newVal)
    {
        std::vector<unsigned int> offsets = {0x01918328, 0xC0, 0x90, 0x40, 0xB8, 0x20, 0x18, 0x28};
        return createPointerToggle(moduleName, "Health", offsets, newVal);
    }

    // AOB-toggle approach
    inline bool setHorizontalSpeed(float newVal)
    {
        // Target instruction bytes: F3 0F 11 40 20
        std::vector<std::string> bigPat = {"F3", "0F", "11", "40", "20", "E9", "DA"};

        size_t patternOffset = 0;
        size_t overwriteLen = 5;
        size_t codeSize = 0x600;

        auto buildFunc = [newVal, overwriteLen](uintptr_t base, uintptr_t hookAddr) -> std::vector<BYTE>
        {
            std::vector<BYTE> code(0x600, 0x90);

            float *pFloat = reinterpret_cast<float *>(&code[0x400]);
            *pFloat = newVal;

            size_t wPos = 0;
            // movss xmm0,[RIP+disp32] => F3 0F 10 05 <disp32>
            code[wPos++] = 0xF3;
            code[wPos++] = 0x0F;
            code[wPos++] = 0x10;
            code[wPos++] = 0x05;
            {
                uintptr_t intAddr = base + 0x400;
                uintptr_t nextInstr = base + wPos + 4;
                int64_t diff = static_cast<int64_t>(intAddr) - static_cast<int64_t>(nextInstr);
                int32_t disp32 = static_cast<int32_t>(diff);
                std::memcpy(&code[wPos], &disp32, 4);
                wPos += 4;
            }

            // movss [rax+20],xmm0 => F3 0F 11 40 20
            code[wPos++] = 0xF3;
            code[wPos++] = 0x0F;
            code[wPos++] = 0x11;
            code[wPos++] = 0x40;
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
            "HorizontalSpeed",
            bigPat,
            patternOffset,
            overwriteLen,
            codeSize,
            buildFunc);
    }

    inline bool setArrowDamage(float newVal)
    {
        // Target instruction bytes: F3 0F 11 43 44
        std::vector<std::string> bigPat = {"F3", "0F", "11", "43", "44", "0F"};

        size_t patternOffset = 0; // start position of target instruction
        size_t overwriteLen = 5;  // length of target instruction to overwrite
        size_t codeSize = 0x600;  // Allocate sufficient space

        // Build the hook
        auto buildFunc = [newVal, overwriteLen](uintptr_t base, uintptr_t hookAddr) -> std::vector<BYTE>
        {
            std::vector<BYTE> code(0x600, 0x90); // NOP padding

            // Embed the new float value at a known offset (e.g., 0x400)
            float *pFloat = reinterpret_cast<float *>(&code[0x400]);
            *pFloat = newVal;

            size_t wPos = 0;

            // movss xmm0, [RIP+disp32] => F3 0F 10 05 <disp32>
            code[wPos++] = 0xF3;
            code[wPos++] = 0x0F;
            code[wPos++] = 0x10;
            code[wPos++] = 0x05;
            {
                uintptr_t floatAddr = base + 0x400;
                uintptr_t nextInstr = base + wPos + 4;
                int64_t diff = static_cast<int64_t>(floatAddr) - static_cast<int64_t>(nextInstr);
                int32_t disp32 = static_cast<int32_t>(diff);
                std::memcpy(&code[wPos], &disp32, 4);
                wPos += 4;
            }

            // movss [rbx+44], xmm0 => F3 0F 11 43 44
            code[wPos++] = 0xF3;
            code[wPos++] = 0x0F;
            code[wPos++] = 0x11;
            code[wPos++] = 0x43;
            code[wPos++] = 0x44;

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
            "ArrowDamage",
            bigPat,
            patternOffset,
            overwriteLen,
            codeSize,
            buildFunc);
    }

    inline bool setArrowFrequency(float newVal)
    {
        // Target instruction bytes: F3 0F 11 73 4C
        std::vector<std::string> bigPat = {"F3", "0F", "11", "73", "4C"};

        size_t patternOffset = 0;
        size_t overwriteLen = 5;
        size_t codeSize = 0x600;

        auto buildFunc = [newVal, overwriteLen](uintptr_t base, uintptr_t hookAddr) -> std::vector<BYTE>
        {
            std::vector<BYTE> code(0x600, 0x90);

            float *pFloat = reinterpret_cast<float *>(&code[0x400]);
            *pFloat = newVal;

            size_t wPos = 0;

            // sub rsp,0x10 => 48 83 EC 10
            code[wPos++] = 0x48;
            code[wPos++] = 0x83;
            code[wPos++] = 0xEC;
            code[wPos++] = 0x10;

            // movss xmm6, [RIP+disp32] => F3 0F 10 35 <disp32>
            code[wPos++] = 0xF3;
            code[wPos++] = 0x0F;
            code[wPos++] = 0x10;
            code[wPos++] = 0x35;
            {
                uintptr_t floatAddr = base + 0x400;
                uintptr_t nextInstr = base + wPos + 4;
                int64_t diff = static_cast<int64_t>(floatAddr) - static_cast<int64_t>(nextInstr);
                int32_t disp32 = static_cast<int32_t>(diff);
                std::memcpy(&code[wPos], &disp32, 4);
                wPos += 4;
            }

            // movss [rbx+4C], xmm6 => F3 0F 11 73 4C
            code[wPos++] = 0xF3;
            code[wPos++] = 0x0F;
            code[wPos++] = 0x11;
            code[wPos++] = 0x73;
            code[wPos++] = 0x4C;

            // add rsp,0x10 => 48 83 C4 10
            code[wPos++] = 0x48;
            code[wPos++] = 0x83;
            code[wPos++] = 0xC4;
            code[wPos++] = 0x10;

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
            "ArrowFrequency",
            bigPat,
            patternOffset,
            overwriteLen,
            codeSize,
            buildFunc);
    }

    inline bool setArrowSpeed(float newVal)
    {
        // Target instruction bytes: F3 0F 11 7B 38
        std::vector<std::string> bigPat = {"F3", "0F", "11", "7B", "38", "83"};

        size_t patternOffset = 0;
        size_t overwriteLen = 5;
        size_t codeSize = 0x600;

        auto buildFunc = [newVal, overwriteLen](uintptr_t base, uintptr_t hookAddr) -> std::vector<BYTE>
        {
            std::vector<BYTE> code(0x600, 0x90);

            float *pFloat = reinterpret_cast<float *>(&code[0x400]);
            *pFloat = newVal;

            size_t wPos = 0;

            // movss xmm7, [RIP+disp32] => F3 0F 10 3D <disp32>
            code[wPos++] = 0xF3;
            code[wPos++] = 0x0F;
            code[wPos++] = 0x10;
            code[wPos++] = 0x3D;
            {
                uintptr_t floatAddr = base + 0x400;
                uintptr_t nextInstr = base + wPos + 4;
                int64_t diff = static_cast<int64_t>(floatAddr) - static_cast<int64_t>(nextInstr);
                int32_t disp32 = static_cast<int32_t>(diff);
                std::memcpy(&code[wPos], &disp32, 4);
                wPos += 4;
            }

            // movss [rbx+38], xmm7 => F3 0F 11 7B 38
            code[wPos++] = 0xF3;
            code[wPos++] = 0x0F;
            code[wPos++] = 0x11;
            code[wPos++] = 0x7B;
            code[wPos++] = 0x38;

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
            "ArrowSpeed",
            bigPat,
            patternOffset,
            overwriteLen,
            codeSize,
            buildFunc);
    }

    inline bool setArrowDistance(float newVal)
    {
        // Target instruction bytes: F3 0F 11 73 40
        std::vector<std::string> bigPat = {"F3", "0F", "11", "73", "40"};

        size_t patternOffset = 0;
        size_t overwriteLen = 5;
        size_t codeSize = 0x600;

        auto buildFunc = [newVal, overwriteLen](uintptr_t base, uintptr_t hookAddr) -> std::vector<BYTE>
        {
            std::vector<BYTE> code(0x600, 0x90);

            float *pFloat = reinterpret_cast<float *>(&code[0x400]);
            *pFloat = newVal;

            size_t wPos = 0;

            // movss xmm6, [RIP+disp32] => F3 0F 10 35 <disp32>
            code[wPos++] = 0xF3;
            code[wPos++] = 0x0F;
            code[wPos++] = 0x10;
            code[wPos++] = 0x35;
            {
                uintptr_t floatAddr = base + 0x400;
                uintptr_t nextInstr = base + wPos + 4;
                int64_t diff = static_cast<int64_t>(floatAddr) - static_cast<int64_t>(nextInstr);
                int32_t disp32 = static_cast<int32_t>(diff);
                std::memcpy(&code[wPos], &disp32, 4);
                wPos += 4;
            }

            // movss [rbx+40], xmm6 => F3 0F 11 73 40
            code[wPos++] = 0xF3;
            code[wPos++] = 0x0F;
            code[wPos++] = 0x11;
            code[wPos++] = 0x73;
            code[wPos++] = 0x40;

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
            "ArrowDistance",
            bigPat,
            patternOffset,
            overwriteLen,
            codeSize,
            buildFunc);
    }

    inline bool setArrowCount(int newVal)
    {
        // Target instruction bytes: 8B 43 48 83 F8 0F
        std::vector<std::string> bigPat = {"8B", "43", "48", "83", "F8", "0F"};

        size_t patternOffset = 0;
        size_t overwriteLen = 6;
        size_t codeSize = 0x600;

        auto buildFunc = [newVal, overwriteLen](uintptr_t base, uintptr_t hookAddr) -> std::vector<BYTE>
        {
            std::vector<BYTE> code(0x600, 0x90);

            int *pInt = reinterpret_cast<int *>(&code[0x400]);
            *pInt = newVal;

            size_t wPos = 0;

            // mov eax,[RIP+disp32] => 8B 05 <disp32>
            code[wPos++] = 0x8B;
            code[wPos++] = 0x05;
            {
                uintptr_t intAddr = base + 0x400;
                uintptr_t nextInstr = base + wPos + 4;
                int64_t diff = static_cast<int64_t>(intAddr) - static_cast<int64_t>(nextInstr);
                int32_t disp32 = static_cast<int32_t>(diff);
                std::memcpy(&code[wPos], &disp32, 4);
                wPos += 4;
            }

            // mov [rbx+48], eax => 89 43 48
            code[wPos++] = 0x89;
            code[wPos++] = 0x43;
            code[wPos++] = 0x48;

            // cmp eax,0F => 83 F8 0F
            code[wPos++] = 0x83;
            code[wPos++] = 0xF8;
            code[wPos++] = 0x0F;

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
            "ArrowCount",
            bigPat,
            patternOffset,
            overwriteLen,
            codeSize,
            buildFunc);
    }

    inline bool setSwordDamage(float newVal)
    {
        // Target instruction bytes: F3 44 0F 11 00
        std::vector<std::string> bigPat = {"F3", "44", "0F", "11", "00"};

        size_t patternOffset = 0;
        size_t overwriteLen = 5;
        size_t codeSize = 0x600;

        auto buildFunc = [newVal, overwriteLen](uintptr_t base, uintptr_t hookAddr) -> std::vector<BYTE>
        {
            std::vector<BYTE> code(0x600, 0x90);

            float *pFloat = reinterpret_cast<float *>(&code[0x400]);
            *pFloat = newVal;

            size_t wPos = 0;

            // movss xmm8,[RIP+disp32] => F3 44 0F 10 05 <disp32>
            code[wPos++] = 0xF3;
            code[wPos++] = 0x44;
            code[wPos++] = 0x0F;
            code[wPos++] = 0x10;
            code[wPos++] = 0x05;
            {
                uintptr_t intAddr = base + 0x400;
                uintptr_t nextInstr = base + wPos + 4;
                int64_t diff = static_cast<int64_t>(intAddr) - static_cast<int64_t>(nextInstr);
                int32_t disp32 = static_cast<int32_t>(diff);
                std::memcpy(&code[wPos], &disp32, 4);
                wPos += 4;
            }

            // movss [rax],xmm8 => F3 44 0F 11 00
            code[wPos++] = 0xF3;
            code[wPos++] = 0x44;
            code[wPos++] = 0x0F;
            code[wPos++] = 0x11;
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

        return createNamedHook(
            moduleName,
            "SwordDamage",
            bigPat,
            patternOffset,
            overwriteLen,
            codeSize,
            buildFunc);
    }

    inline bool setSwordCoolDown(float newVal)
    {
        // Target instruction bytes: F3 0F 11 79 04
        std::vector<std::string> bigPat = {"F3", "0F", "11", "79", "04"};

        size_t patternOffset = 0;
        size_t overwriteLen = 5;
        size_t codeSize = 0x600;

        auto buildFunc = [newVal, overwriteLen](uintptr_t base, uintptr_t hookAddr) -> std::vector<BYTE>
        {
            std::vector<BYTE> code(0x600, 0x90);

            float *pFloat = reinterpret_cast<float *>(&code[0x400]);
            *pFloat = newVal;

            size_t wPos = 0;

            // movss xmm7,[RIP+disp32] => F3 0F 10 3D <disp32>
            code[wPos++] = 0xF3;
            code[wPos++] = 0x0F;
            code[wPos++] = 0x10;
            code[wPos++] = 0x3D;
            {
                uintptr_t intAddr = base + 0x400;
                uintptr_t nextInstr = base + wPos + 4;
                int64_t diff = static_cast<int64_t>(intAddr) - static_cast<int64_t>(nextInstr);
                int32_t disp32 = static_cast<int32_t>(diff);
                std::memcpy(&code[wPos], &disp32, 4);
                wPos += 4;
            }

            // movss [rcx+04],xmm7 => F3 0F 11 79 04
            code[wPos++] = 0xF3;
            code[wPos++] = 0x0F;
            code[wPos++] = 0x11;
            code[wPos++] = 0x79;
            code[wPos++] = 0x04;

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
            "SwordCoolDown",
            bigPat,
            patternOffset,
            overwriteLen,
            codeSize,
            buildFunc);
    }

    inline bool setSwordSpeed(float newVal)
    {
        // Target instruction bytes: F3 0F 11 41 14
        std::vector<std::string> bigPat = {"F3", "0F", "11", "41", "14", "48", "8B", "05"};

        size_t patternOffset = 0;
        size_t overwriteLen = 5;
        size_t codeSize = 0x600;

        auto buildFunc = [newVal, overwriteLen](uintptr_t base, uintptr_t hookAddr) -> std::vector<BYTE>
        {
            std::vector<BYTE> code(0x600, 0x90);

            float *pFloat = reinterpret_cast<float *>(&code[0x400]);
            *pFloat = newVal;

            size_t wPos = 0;
            // movss xmm0,[RIP+disp32] => F3 0F 10 05 <disp32>
            code[wPos++] = 0xF3;
            code[wPos++] = 0x0F;
            code[wPos++] = 0x10;
            code[wPos++] = 0x05;
            {
                uintptr_t intAddr = base + 0x400;
                uintptr_t nextInstr = base + wPos + 4;
                int64_t diff = static_cast<int64_t>(intAddr) - static_cast<int64_t>(nextInstr);
                int32_t disp32 = static_cast<int32_t>(diff);
                std::memcpy(&code[wPos], &disp32, 4);
                wPos += 4;
            }

            // movss [rcx+14],xmm0 => F3 0F 11 41 14
            code[wPos++] = 0xF3;
            code[wPos++] = 0x0F;
            code[wPos++] = 0x11;
            code[wPos++] = 0x41;
            code[wPos++] = 0x14;

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
            "SwordSpeed",
            bigPat,
            patternOffset,
            overwriteLen,
            codeSize,
            buildFunc);
    }

    inline bool setSwordDistance(float newVal)
    {
        // Target instruction bytes: F3 0F 11 71 08
        std::vector<std::string> bigPat = {"F3", "0F", "11", "71", "08"};

        size_t patternOffset = 0;
        size_t overwriteLen = 5;
        size_t codeSize = 0x600;

        auto buildFunc = [newVal, overwriteLen](uintptr_t base, uintptr_t hookAddr) -> std::vector<BYTE>
        {
            std::vector<BYTE> code(0x600, 0x90);

            float *pFloat = reinterpret_cast<float *>(&code[0x400]);
            *pFloat = newVal;

            size_t wPos = 0;

            // movss xmm6,[RIP+disp32] => F3 0F 10 35 <disp32>
            code[wPos++] = 0xF3;
            code[wPos++] = 0x0F;
            code[wPos++] = 0x10;
            code[wPos++] = 0x35;
            {
                uintptr_t intAddr = base + 0x400;
                uintptr_t nextInstr = base + wPos + 4;
                int64_t diff = static_cast<int64_t>(intAddr) - static_cast<int64_t>(nextInstr);
                int32_t disp32 = static_cast<int32_t>(diff);
                std::memcpy(&code[wPos], &disp32, 4);
                wPos += 4;
            }

            // movss [rcx+08],xmm6 => F3 0F 11 71 08
            code[wPos++] = 0xF3;
            code[wPos++] = 0x0F;
            code[wPos++] = 0x11;
            code[wPos++] = 0x71;
            code[wPos++] = 0x08;

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
            "SwordDistance",
            bigPat,
            patternOffset,
            overwriteLen,
            codeSize,
            buildFunc);
    }
};
