// trainer.h
#pragma once

#include "TrainerBase.h"

class Trainer : public TrainerBase
{
public:
    Trainer() : TrainerBase(L"Sackboy.exe") {} // x64
    ~Trainer() override = default;

    // Constants
    static inline const wchar_t *moduleName = L"VCRUNTIME140.dll";

    inline bool setBell(int newVal)
    {
        // Target instruction bytes: 89 08 C3 90 49 83 F8 20 77 17 F3 0F 6F 0A F3 42 0F 6F 54 02 F0 F3 0F 7F 09 F3 42 0F 7F 54 01 F0 C3 4E 8D 0C 02 48 3B CA 4C 0F 46 C9 49 3B C9 0F 82 3F 04 00 00 83 3D 48 7B 00 00 03 0F 82 E2 02 00 00 49 81 F8 00 20 00 00 76 16 49 81 F8 00 00 18 00 77 0D F6 05 B9 7E 00 00 02 0F 85 73 FE FF FF C5 FE 6F 02 C4 A1 7E 6F 6C 02 E0 49 81 F8 00 01 00 00 0F 86 C3 00 00 00 4C 8B C9 49 83 E1 1F 49 83 E9 20 49 2B C9 49 2B D1 4D 03 C1 49 81 F8 00 01 00 00 0F 86 A2 00 00 00 49 81 F8 00 00 18 00 0F 87 3D 01 00 00 66 66 66 66 66 0F 1F 84 00 00 00 00 00 C5 FE 6F 0A C5 FE 6F 52 20 C5 FE 6F 5A 40 C5 FE 6F 62 60 C5 FD 7F 09 C5 FD 7F 51 20 C5 FD 7F 59 40 C5 FD 7F 61 60 C5 FE 6F 8A 80 00 00 00 C5 FE 6F 92 A0 00 00 00 C5 FE 6F 9A C0 00 00 00 C5 FE 6F A2 E0 00 00 00 C5 FD 7F 89 80 00 00 00 C5 FD 7F 91 A0 00 00 00 C5 FD 7F 99 C0 00 00 00 C5 FD 7F A1 E0 00 00 00 48 81 C1 00 01 00 00 48 81 C2 00 01 00 00 49 81 E8 00 01 00 00 49 81 F8 00 01 00 00 0F 83 78 FF FF FF 4D 8D 48 1F 49 83 E1 E0 4D 8B D9 49 C1 EB 05 47 8B 9C 9A C0 5E 01 00 4D 03 DA 41 FF E3 C4 A1 7E 6F 8C 0A 00 FF FF FF
        std::vector<std::string> bigPat = {
            "89", "08", "C3", "90", "49", "83", "F8", "20", "77", "17", "F3", "0F", "6F", "0A", "F3", "42",
            "0F", "6F", "54", "02", "F0", "F3", "0F", "7F", "09", "F3", "42", "0F", "7F", "54", "01", "F0",
            "C3", "4E", "8D", "0C", "02", "48", "3B", "CA", "4C", "0F", "46", "C9", "49", "3B", "C9", "0F",
            "82", "3F", "04", "00", "00", "83", "3D", "48", "7B", "00", "00", "03", "0F", "82", "E2", "02",
            "00", "00", "49", "81", "F8", "00", "20", "00", "00", "76", "16", "49", "81", "F8", "00", "00",
            "18", "00", "77", "0D", "F6", "05", "B9", "7E", "00", "00", "02", "0F", "85", "73", "FE", "FF",
            "FF", "C5", "FE", "6F", "02", "C4", "A1", "7E", "6F", "6C", "02", "E0", "49", "81", "F8", "00",
            "01", "00", "00", "0F", "86", "C3", "00", "00", "00", "4C", "8B", "C9", "49", "83", "E1", "1F",
            "49", "83", "E9", "20", "49", "2B", "C9", "49", "2B", "D1", "4D", "03", "C1", "49", "81", "F8",
            "00", "01", "00", "00", "0F", "86", "A2", "00", "00", "00", "49", "81", "F8", "00", "00", "18",
            "00", "0F", "87", "3D", "01", "00", "00", "66", "66", "66", "66", "66", "0F", "1F", "84", "00",
            "00", "00", "00", "00", "C5", "FE", "6F", "0A", "C5", "FE", "6F", "52", "20", "C5", "FE", "6F",
            "5A", "40", "C5", "FE", "6F", "62", "60", "C5", "FD", "7F", "09", "C5", "FD", "7F", "51", "20",
            "C5", "FD", "7F", "59", "40", "C5", "FD", "7F", "61", "60", "C5", "FE", "6F", "8A", "80", "00",
            "00", "00", "C5", "FE", "6F", "92", "A0", "00", "00", "00", "C5", "FE", "6F", "9A", "C0", "00",
            "00", "00", "C5", "FE", "6F", "A2", "E0", "00", "00", "00", "C5", "FD", "7F", "89", "80", "00",
            "00", "00", "C5", "FD", "7F", "91", "A0", "00", "00", "00", "C5", "FD", "7F", "99", "C0", "00",
            "00", "00", "C5", "FD", "7F", "A1", "E0", "00", "00", "00", "48", "81", "C1", "00", "01", "00",
            "00", "48", "81", "C2", "00", "01", "00", "00", "49", "81", "E8", "00", "01", "00", "00", "49",
            "81", "F8", "00", "01", "00", "00", "0F", "83", "78", "FF", "FF", "FF", "4D", "8D", "48", "1F",
            "49", "83", "E1", "E0", "4D", "8B", "D9", "49", "C1", "EB", "05", "47", "8B", "9C", "9A", "C0",
            "5E", "01", "00", "4D", "03", "DA", "41", "FF", "E3", "C4", "A1", "7E", "6F", "8C", "0A", "00",
            "FF", "FF", "FF"};

        size_t patternOffset = 0; // start position of target instruction
        size_t overwriteLen = 8;  // length of target instruction to overwrite
        size_t codeSize = 0x1000;  // Allocate sufficient space

        // Build the hook
        auto buildFunc = [newVal, overwriteLen](uintptr_t base, uintptr_t hookAddr) -> std::vector<BYTE>
        {
            std::vector<BYTE> code(0x1000, 0x90); // NOP padding

            int *pInt = reinterpret_cast<int *>(&code[0x400]);
            *pInt = newVal;

            size_t wPos = 0;

            // mov ecx, [RIP+disp32] => 8B 0D <disp32>
            code[wPos++] = 0x8B;
            code[wPos++] = 0x0D;
            {
                uintptr_t floatAddr = base + 0x400;
                uintptr_t nextInstr = base + wPos + 4;
                int64_t diff = static_cast<int64_t>(floatAddr) - static_cast<int64_t>(nextInstr);
                int32_t disp32 = static_cast<int32_t>(diff);
                std::memcpy(&code[wPos], &disp32, 4);
                wPos += 4;
            }

            // mov [rax],ecx => 89 08
            code[wPos++] = 0x89;
            code[wPos++] = 0x08;

            // ret => C3
            code[wPos++] = 0xC3;

            // nop => 90
            code[wPos++] = 0x90;

            // cmp r8,20 => 49 83 F8 20
            code[wPos++] = 0x49;
            code[wPos++] = 0x83;
            code[wPos++] = 0xF8;
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

        // Create the hook
        return createNamedHook(
            moduleName,
            "Bell",
            bigPat,
            patternOffset,
            overwriteLen,
            codeSize,
            buildFunc);
    }
};
