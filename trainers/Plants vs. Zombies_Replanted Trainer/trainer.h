// trainer.h
#pragma once

#include "TrainerBase.h"

class Trainer : public TrainerBase
{
public:
    Trainer() : TrainerBase(L"Replanted.exe") {} // x64
    ~Trainer() override = default;

    // Constants
    static inline const wchar_t *moduleName = L"GameAssembly.dll";

    inline bool noPlantCooldown()
    {
        const std::vector<std::string> pat = {"0F", "85", "25", "04", "00", "00", "48", "8B", "43"};
        size_t patchOffset = 1;

        // JNE -> JE
        const std::vector<BYTE> newBytes = { 0x84 };

        return createBytePatch(moduleName, "NoPlantCooldown", pat, patchOffset, newBytes);
    }

    inline bool noPlantCost()
    {
        bool hook1_success = false;
        {
            // Changes 'jle' (7E) to 'jmp' (EB) to make the cost check always pass.
            const std::vector<std::string> pat = {"7E", "43", "48", "8B", "87", "08", "03", "00", "00"};
            size_t patchOffset = 0;
            const std::vector<BYTE> newBytes = { 0xEB }; // EB = jmp

            hook1_success = createBytePatch(moduleName, "NoPlantCost1", pat, patchOffset, newBytes);
        }

        bool hook2_success = false;
        {
            // Changes 'setle al' (0F 9E C0) to 'mov al, 1; nop' (B0 01 90).
            // This forces the "can afford" check to always return 'true'.
            const std::vector<std::string> pat = {"0F", "9E", "C0", "48", "83", "C4", "20", "5F", "C3", "E8", "C8"};
            size_t patchOffset = 0;
            const std::vector<BYTE> newBytes = { 0xB0, 0x01, 0x90 }; // mov al, 1; nop

            hook2_success = createBytePatch(moduleName, "NoPlantCost2", pat, patchOffset, newBytes);
        }

        bool hook3_success = false;
        {
            // Changes 'sub [rax+10],r13d' (44 29 68 10) to 'nop's.
            // This prevents the sun cost from being subtracted from your total.
            const std::vector<std::string> pat = {"44", "29", "68", "10", "44", "8B", "84", "24", "E0", "00", "00", "00"};
            size_t patchOffset = 0;
            const std::vector<BYTE> newBytes = { 0x90, 0x90, 0x90, 0x90 }; // 4 NOPs

            hook3_success = createBytePatch(moduleName, "NoPlantCost3", pat, patchOffset, newBytes);
        }

        return hook1_success && hook2_success && hook3_success;
    }

    inline bool setFertilizerAndBugSpray(int newVal)
    {
        bool hook1_success = false;
        {
            // Hook 1: Replaces 'add dword ptr [rcx+rdi*4+20],05'
            const std::vector<std::string> pat = {"83", "44", "B9", "20", "05"};
            size_t patternOffset = 0;
            size_t overwriteLen = 5;
            size_t codeSize = 0x100;

            auto buildFunc = [newVal, overwriteLen, codeSize](uintptr_t codeCaveAddr, uintptr_t hookAddr, const std::vector<BYTE> &originalBytes) -> std::vector<BYTE>
            {
                std::vector<BYTE> code(codeSize, 0x90);
                size_t wPos = 0;

                // 1. mov dword ptr [rcx+rdi*4+20], newVal => C7 44 B9 20 <imm32>
                code[wPos++] = 0xC7;
                code[wPos++] = 0x44;
                code[wPos++] = 0xB9;
                code[wPos++] = 0x20;
                std::memcpy(&code[wPos], &newVal, 4);
                wPos += 4;

                // 2. jmp <rel32>
                code[wPos++] = 0xE9;
                uintptr_t returnAddr = hookAddr + overwriteLen;
                uintptr_t nextInstrAddr = codeCaveAddr + wPos + 4;
                int32_t relativeJump = static_cast<int32_t>(returnAddr - nextInstrAddr);
                std::memcpy(&code[wPos], &relativeJump, 4);
                wPos += 4;
                return code;
            };
            // Use one, shared hook name for the 'increase' function
            hook1_success = createNamedHook(nullptr, "SetFertilizerAndBugSprayInc", pat, patternOffset, overwriteLen, codeSize, buildFunc);
        }

        bool hook2_success = false;
        {
            // Hook 2: Replaces 'dec [rax+58]' and 'xor edx,edx' (Fertilizer Decrease)
            const std::vector<std::string> pat = {"FF", "48", "58", "33", "D2"};
            size_t patternOffset = 0;
            size_t overwriteLen = 5;
            size_t codeSize = 0x100;

            auto buildFunc = [newVal, overwriteLen, codeSize](uintptr_t codeCaveAddr, uintptr_t hookAddr, const std::vector<BYTE> &originalBytes) -> std::vector<BYTE>
            {
                std::vector<BYTE> code(codeSize, 0x90);
                size_t wPos = 0;

                // 1. mov dword ptr [rax+58], newVal => C7 40 58 <imm32>
                code[wPos++] = 0xC7;
                code[wPos++] = 0x40;
                code[wPos++] = 0x58;
                std::memcpy(&code[wPos], &newVal, 4);
                wPos += 4;

                // 2. Add back the 'xor edx,edx' (33 D2) we overwrote
                code[wPos++] = 0x33;
                code[wPos++] = 0xD2;

                // 3. jmp <rel32> back to the *next* instruction after the xor
                code[wPos++] = 0xE9;
                uintptr_t returnAddr = hookAddr + overwriteLen; // Jumps to ...6F36AF
                uintptr_t nextInstrAddr = codeCaveAddr + wPos + 4;
                int32_t relativeJump = static_cast<int32_t>(returnAddr - nextInstrAddr);
                std::memcpy(&code[wPos], &relativeJump, 4);
                wPos += 4;
                return code;
            };
            hook2_success = createNamedHook(nullptr, "SetFertilizerDec", pat, patternOffset, overwriteLen, codeSize, buildFunc);
        }

        bool hook3_success = false;
        {
            // Hook 3: Replaces 'dec [rax+5C]' and 'jmp ...' (BugSpray Decrease)
            const std::vector<std::string> pat = {"FF", "48", "5C", "E9", "06", "01", "00", "00"};
            size_t patternOffset = 0;
            size_t overwriteLen = 8;
            size_t codeSize = 0x100;

            auto buildFunc = [newVal, overwriteLen, codeSize](uintptr_t codeCaveAddr, uintptr_t hookAddr, const std::vector<BYTE> &originalBytes) -> std::vector<BYTE>
            {
                std::vector<BYTE> code(codeSize, 0x90);
                size_t wPos = 0;

                // 1. mov dword ptr [rax+5C], newVal => C7 40 5C <imm32>
                code[wPos++] = 0xC7;
                code[wPos++] = 0x40;
                code[wPos++] = 0x5C;
                std::memcpy(&code[wPos], &newVal, 4);
                wPos += 4;

                // 2. The original jmp went to 'xor edx,edx'. We add that back.
                code[wPos++] = 0x33;
                code[wPos++] = 0xD2;

                // 3. We now need to find where the 'xor' would have continued to.
                //    The 'xor' was at ...6F36AD. The instruction after it is at ...6F36AF.
                //    We must jump to ...6F36AF.

                // 4. Get the original jmp's target (...6F36AD)
                int32_t original_rel32;
                std::memcpy(&original_rel32, &originalBytes[4], 4);
                uintptr_t originalJmpNextInstr = hookAddr + overwriteLen;
                uintptr_t jmpTargetAddr = originalJmpNextInstr + original_rel32; // ...6F36AD

                // 5. Calculate the *real* target, which is 2 bytes after the jmp's target
                uintptr_t absoluteTargetAddr = jmpTargetAddr + 2; // ...6F36AF

                // 6. jmp <new_rel32>
                code[wPos++] = 0xE9;
                uintptr_t newJmpNextInstr = codeCaveAddr + wPos + 4;
                int32_t new_rel32 = static_cast<int32_t>(absoluteTargetAddr - newJmpNextInstr);
                std::memcpy(&code[wPos], &new_rel32, 4);
                wPos += 4;
                return code;
            };
            hook3_success = createNamedHook(nullptr, "SetBugSprayDec", pat, patternOffset, overwriteLen, codeSize, buildFunc);
        }

        return hook1_success && hook2_success && hook3_success;
    }

    inline bool setChocolate(int newVal)
    {
        bool hook1_success = false;
        {
            // Hook 1: Replaces 'inc [rax+00000088]'
            const std::vector<std::string> pat = {"FF", "80", "88", "00", "00", "00", "C7"};
            size_t patternOffset = 0;
            size_t overwriteLen = 6; // Length of the 'inc' instruction
            size_t codeSize = 0x100;

            auto buildFunc = [newVal, overwriteLen, codeSize](uintptr_t codeCaveAddr, uintptr_t hookAddr, const std::vector<BYTE> &originalBytes) -> std::vector<BYTE>
            {
                // No data section needed
                std::vector<BYTE> code(codeSize, 0x90);
                size_t wPos = 0;

                // --- CODE SECTION ---
                // 1. mov dword ptr [rax+00000088], newVal => C7 80 88 00 00 00 <imm32>
                code[wPos++] = 0xC7;
                code[wPos++] = 0x80; // ModR/M for [rax + disp32]
                code[wPos++] = 0x88; // The offset +88
                code[wPos++] = 0x00;
                code[wPos++] = 0x00;
                code[wPos++] = 0x00;

                // 2. Copy newVal (our 4-byte immediate value)
                std::memcpy(&code[wPos], &newVal, 4);
                wPos += 4;

                // 3. jmp <rel32>
                code[wPos++] = 0xE9;
                uintptr_t returnAddr = hookAddr + overwriteLen;
                uintptr_t nextInstrAddr = codeCaveAddr + wPos + 4;
                int32_t relativeJump = static_cast<int32_t>(returnAddr - nextInstrAddr);
                std::memcpy(&code[wPos], &relativeJump, 4);
                wPos += 4;

                return code;
            };
            hook1_success = createNamedHook(nullptr, "SetChocolateInc", pat, patternOffset, overwriteLen, codeSize, buildFunc);
        }

        bool hook2_success = false;
        {
            // Hook 2: Replaces 'dec [rax+00000088]'
            const std::vector<std::string> pat = {"FF", "88", "88", "00", "00", "00", "33"};
            size_t patternOffset = 0;
            size_t overwriteLen = 6; // Length of the 'dec' instruction
            size_t codeSize = 0x100;

            auto buildFunc = [newVal, overwriteLen, codeSize](uintptr_t codeCaveAddr, uintptr_t hookAddr, const std::vector<BYTE> &originalBytes) -> std::vector<BYTE>
            {
                // No data section needed
                std::vector<BYTE> code(codeSize, 0x90);
                size_t wPos = 0;

                // --- CODE SECTION ---
                // 1. mov dword ptr [rax+00000088], newVal => C7 80 88 00 00 00 <imm32>
                code[wPos++] = 0xC7;
                code[wPos++] = 0x80; // ModR/M for [rax + disp32]
                code[wPos++] = 0x88; // The offset +88
                code[wPos++] = 0x00;
                code[wPos++] = 0x00;
                code[wPos++] = 0x00;

                // 2. Copy newVal (our 4-byte immediate value)
                std::memcpy(&code[wPos], &newVal, 4);
                wPos += 4;

                // 3. jmp <rel32>
                code[wPos++] = 0xE9;
                uintptr_t returnAddr = hookAddr + overwriteLen;
                uintptr_t nextInstrAddr = codeCaveAddr + wPos + 4;
                int32_t relativeJump = static_cast<int32_t>(returnAddr - nextInstrAddr);
                std::memcpy(&code[wPos], &relativeJump, 4);
                wPos += 4;

                return code;
            };
            hook2_success = createNamedHook(nullptr, "SetChocolateDec", pat, patternOffset, overwriteLen, codeSize, buildFunc);
        }

        return hook1_success && hook2_success;
    }

    inline bool setTreeFood(int newVal)
    {
        bool hook1_success = false;
        {
            const std::vector<std::string> pat = {"FF", "80", "90", "00", "00", "00"};
            size_t patternOffset = 0;
            size_t overwriteLen = 6;
            size_t codeSize = 0x100;

            auto buildFunc = [newVal, overwriteLen, codeSize](uintptr_t codeCaveAddr, uintptr_t hookAddr, const std::vector<BYTE> &originalBytes) -> std::vector<BYTE>
            {
                // No data section needed
                std::vector<BYTE> code(codeSize, 0x90);
                size_t wPos = 0;

                // --- CODE SECTION ---
                // 1. mov dword ptr [rax+00000090], newVal => C7 80 90 00 00 00 <imm32>
                code[wPos++] = 0xC7;
                code[wPos++] = 0x80;
                code[wPos++] = 0x90;
                code[wPos++] = 0x00;
                code[wPos++] = 0x00;
                code[wPos++] = 0x00;

                // 2. Copy newVal (our 4-byte immediate value)
                std::memcpy(&code[wPos], &newVal, 4);
                wPos += 4;

                // 3. jmp <rel32>
                code[wPos++] = 0xE9;
                uintptr_t returnAddr = hookAddr + overwriteLen;
                uintptr_t nextInstrAddr = codeCaveAddr + wPos + 4;
                int32_t relativeJump = static_cast<int32_t>(returnAddr - nextInstrAddr);
                std::memcpy(&code[wPos], &relativeJump, 4);
                wPos += 4;

                return code;
            };
            hook1_success = createNamedHook(nullptr, "TreeFoodInc", pat, patternOffset, overwriteLen, codeSize, buildFunc);
        }

        bool hook2_success = false;
        {
            const std::vector<std::string> pat = {"FF", "88", "90", "00", "00", "00", "48", "8B", "4B"};
            size_t patternOffset = 0;
            size_t overwriteLen = 6;
            size_t codeSize = 0x100;

            auto buildFunc = [newVal, overwriteLen, codeSize](uintptr_t codeCaveAddr, uintptr_t hookAddr, const std::vector<BYTE> &originalBytes) -> std::vector<BYTE>
            {
                // No data section needed
                std::vector<BYTE> code(codeSize, 0x90);
                size_t wPos = 0;

                // --- CODE SECTION ---
                // 1. mov dword ptr [rax+00000090], newVal => C7 80 90 00 00 00 <imm32>
                code[wPos++] = 0xC7;
                code[wPos++] = 0x80;
                code[wPos++] = 0x90;
                code[wPos++] = 0x00;
                code[wPos++] = 0x00;
                code[wPos++] = 0x00;

                // 2. Copy newVal (our 4-byte immediate value)
                std::memcpy(&code[wPos], &newVal, 4);
                wPos += 4;

                // 3. jmp <rel32>
                code[wPos++] = 0xE9;
                uintptr_t returnAddr = hookAddr + overwriteLen;
                uintptr_t nextInstrAddr = codeCaveAddr + wPos + 4;
                int32_t relativeJump = static_cast<int32_t>(returnAddr - nextInstrAddr);
                std::memcpy(&code[wPos], &relativeJump, 4);
                wPos += 4;

                return code;
            };

            hook2_success = createNamedHook(nullptr, "TreeFoodDec", pat, patternOffset, overwriteLen, codeSize, buildFunc);
        }

        return hook1_success && hook2_success;
    }

    inline bool setCoin(int newVal)
    {
        // Target instruction bytes: 01 58 50 48 83 C4 20
        const std::vector<std::string> pat = {"01", "58", "50", "48", "83", "C4", "20"};
        size_t patternOffset = 0;
        size_t overwriteLen = 7;
        size_t codeSize = 0x100;

        auto buildFunc = [newVal, overwriteLen, codeSize](uintptr_t codeCaveAddr, uintptr_t hookAddr, const std::vector<BYTE> &originalBytes) -> std::vector<BYTE>
        {
            // No data section needed
            std::vector<BYTE> code(codeSize, 0x90);
            size_t wPos = 0;

            // --- CODE SECTION ---
            // 1. mov dword ptr [rax+50], newVal => C7 40 50 <imm32>
            code[wPos++] = 0xC7;
            code[wPos++] = 0x40;
            code[wPos++] = 0x50;

            // 2. Copy newVal
            std::memcpy(&code[wPos], &newVal, 4);
            wPos += 4;

            // 3. Copy the second original instruction (add rsp,20)
            std::memcpy(&code[wPos], &originalBytes[3], 4);
            wPos += 4;

            // 4. jmp <rel32>
            code[wPos++] = 0xE9;
            uintptr_t returnAddr = hookAddr + overwriteLen;
            uintptr_t nextInstrAddr = codeCaveAddr + wPos + 4;
            int32_t relativeJump = static_cast<int32_t>(returnAddr - nextInstrAddr);
            std::memcpy(&code[wPos], &relativeJump, 4);
            wPos += 4;

            return code;
        };

        return createNamedHook(nullptr, "SetCoin", pat, patternOffset, overwriteLen, codeSize, buildFunc);
    }

    inline bool setSun(int newVal)
    {
        bool hook1_success = false;
        {
            // Hook 1: Replaces 'add [rax+10], esi'
            const std::vector<std::string> pat = {"01", "70", "10", "48", "8B", "8B", "E0", "01", "00", "00"};
            size_t patternOffset = 0;
            size_t overwriteLen = 10; // 3 bytes (add) + 7 bytes (mov)
            size_t codeSize = 0x100;

            auto buildFunc = [newVal, overwriteLen, codeSize](uintptr_t codeCaveAddr, uintptr_t hookAddr, const std::vector<BYTE> &originalBytes) -> std::vector<BYTE>
            {
                // No data section needed
                std::vector<BYTE> code(codeSize, 0x90);
                size_t wPos = 0;

                // --- CODE SECTION ---
                // 1. mov dword ptr [rax+10], newVal => C7 40 10 <imm32>
                code[wPos++] = 0xC7;
                code[wPos++] = 0x40;
                code[wPos++] = 0x10;

                // 2. Copy newVal (our 4-byte immediate value)
                std::memcpy(&code[wPos], &newVal, 4);
                wPos += 4;

                // 3. Copy the second original instruction (mov rcx,[rbx+...])
                std::memcpy(&code[wPos], &originalBytes[3], 7);
                wPos += 7;

                // 4. jmp <rel32>
                code[wPos++] = 0xE9;
                uintptr_t returnAddr = hookAddr + overwriteLen;
                uintptr_t nextInstrAddr = codeCaveAddr + wPos + 4;
                int32_t relativeJump = static_cast<int32_t>(returnAddr - nextInstrAddr);
                std::memcpy(&code[wPos], &relativeJump, 4);
                wPos += 4;

                return code;
            };
            hook1_success = createNamedHook(nullptr, "SetSunInc", pat, patternOffset, overwriteLen, codeSize, buildFunc);
        }

        bool hook2_success = false;
        {
            // Hook 2: Replaces 'sub [rax+10], r13d'
            const std::vector<std::string> pat = {"44", "29", "68", "10", "44", "8B", "84", "24", "E0", "00", "00", "00"};
            size_t patternOffset = 0;
            size_t overwriteLen = 12; // 4 bytes (sub) + 8 bytes (mov)
            size_t codeSize = 0x100;

            auto buildFunc = [newVal, overwriteLen, codeSize](uintptr_t codeCaveAddr, uintptr_t hookAddr, const std::vector<BYTE> &originalBytes) -> std::vector<BYTE>
            {
                // No data section needed
                std::vector<BYTE> code(codeSize, 0x90);
                size_t wPos = 0;

                // --- CODE SECTION ---
                // 1. mov dword ptr [rax+10], newVal => C7 40 10 <imm32>
                code[wPos++] = 0xC7;
                code[wPos++] = 0x40;
                code[wPos++] = 0x10;

                // 2. Copy newVal (our 4-byte immediate value)
                std::memcpy(&code[wPos], &newVal, 4);
                wPos += 4;

                // 3. Copy the second original instruction (mov r8d,[rsp+...])
                std::memcpy(&code[wPos], &originalBytes[4], 8);
                wPos += 8;

                // 4. jmp <rel32>
                code[wPos++] = 0xE9;
                uintptr_t returnAddr = hookAddr + overwriteLen;
                uintptr_t nextInstrAddr = codeCaveAddr + wPos + 4;
                int32_t relativeJump = static_cast<int32_t>(returnAddr - nextInstrAddr);
                std::memcpy(&code[wPos], &relativeJump, 4);
                wPos += 4;

                return code;
            };
            hook2_success = createNamedHook(nullptr, "SetSunDec", pat, patternOffset, overwriteLen, codeSize, buildFunc);
        }

        return hook1_success && hook2_success;
    }
};
