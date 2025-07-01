// Trainer.h
#pragma once

#include "MonoBase.h"

class Trainer : public MonoBase
{
public:
    Trainer() : MonoBase(L"DREDGE.exe") {} // x86
    virtual ~Trainer() {}

    static inline const wchar_t *moduleName = L"mono-2.0-bdwgc.dll";

    bool godMode(BYTE enable)
    {
        std::vector<unsigned int> offsets1 = {0x00561CCC, 0xC, 0x4, 0x6C, 0x18, 0x0, 0xAC, 0x94}; // No death
        std::vector<unsigned int> offsets2 = {0x00561CCC, 0xC, 0x4, 0x6C, 0x18, 0x0, 0xAC, 0x95}; // No damage
        bool result1 = WriteToDynamicAddress(moduleName, offsets1, enable);
        bool result2 = WriteToDynamicAddress(moduleName, offsets2, enable);
        return result1 && result2;
    }

    inline bool setSanity(float newVal)
    {
        // Allocate shared memory for float value
        if (m_pSharedSanity == 0)
        {
            m_pSharedSanity = reinterpret_cast<uintptr_t>(VirtualAllocEx(hProcess, nullptr, sizeof(float), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
            if (!m_pSharedSanity)
            {
                std::cerr << "[!] Failed to allocate shared memory for sanity value." << std::endl;
                return false;
            }
        }
        WriteProcessMemory(hProcess, (LPVOID)m_pSharedSanity, &newVal, sizeof(float), nullptr);

        // First hook
        bool hook1_success = false;
        {
            const std::vector<std::string> pat1 = {"D9", "5F", "18", "D9", "47", "18", "D9", "1C", "24", "8D"};
            auto buildFunc1 = [this](uintptr_t codeCaveAddr, uintptr_t hookAddr, const std::vector<BYTE> &originalBytes) -> std::vector<BYTE>
            {
                std::vector<BYTE> code;

                // fstp st(0)
                code.insert(code.end(), {0xDD, 0xD8});

                // fld dword ptr [codeCaveAddr]
                uint32_t addr32 = static_cast<uint32_t>(this->m_pSharedSanity);
                code.insert(code.end(), {0xD9, 0x05});
                code.insert(code.end(), (BYTE *)&addr32, (BYTE *)&addr32 + 4);

                // fstp dword ptr [edi+18]
                code.insert(code.end(), {0xD9, 0x5F, 0x18});

                // fld dword ptr [edi+18]
                code.insert(code.end(), {0xD9, 0x47, 0x18});

                // jmp [rel_addr]
                uintptr_t retAddr = hookAddr + originalBytes.size();     // address to jump back to
                uintptr_t nextInstr = codeCaveAddr + code.size() + 5;    // next instruction address after jmp in code cave
                int32_t rel = static_cast<int32_t>(retAddr - nextInstr); // address that jmp uses
                code.push_back(0xE9);
                code.insert(code.end(), (BYTE *)&rel, (BYTE *)&rel + 4);

                return code;
            };
            hook1_success = createNamedHook(nullptr, "SetSanity_1", pat1, 0, 6, 0x100, buildFunc1);
        }

        // Second hook
        bool hook2_success = false;
        {
            // This is a case to replace dynamic instruction bytes
            const std::vector<std::string> pat2 = {"E8", "??", "??", "??", "??", "D9", "5F", "18", "8B", "05"};
            auto buildFunc2 = [this](uintptr_t codeCaveAddr, uintptr_t hookAddr, const std::vector<BYTE> &originalBytes) -> std::vector<BYTE>
            {
                std::vector<BYTE> code;

                // call [rel_addr]
                int32_t original_relative_offset;
                std::memcpy(&original_relative_offset, &originalBytes[1], 4);
                uintptr_t absolute_target_address = hookAddr + 5 + original_relative_offset;
                uintptr_t next_instr_in_cave = codeCaveAddr + code.size() + 5;
                int32_t new_relative_offset = static_cast<int32_t>(absolute_target_address - next_instr_in_cave);
                code.push_back(0xE8);
                code.insert(code.end(), (BYTE *)&new_relative_offset, (BYTE *)&new_relative_offset + 4);

                // fstp st(0)
                code.insert(code.end(), {0xDD, 0xD8});

                // fld dword ptr [codeCaveAddr]
                uint32_t addr32 = static_cast<uint32_t>(this->m_pSharedSanity);
                code.insert(code.end(), {0xD9, 0x05});
                code.insert(code.end(), (BYTE *)&addr32, (BYTE *)&addr32 + 4);

                // jmp [rel_addr]
                uintptr_t retAddr = hookAddr + originalBytes.size();
                uintptr_t nextInstr = codeCaveAddr + code.size() + 5;
                int32_t rel = static_cast<int32_t>(retAddr - nextInstr);
                code.push_back(0xE9);
                code.insert(code.end(), (BYTE *)&rel, (BYTE *)&rel + 4);

                return code;
            };
            hook2_success = createNamedHook(nullptr, "SetSanity_2", pat2, 0, 5, 0x100, buildFunc2);
        }

        return hook1_success && hook2_success;
    }

    inline bool setMovementSpeed(float newVal)
    {
        const std::vector<std::string> pat = {"D9", "47", "48", "DE", "C9", "D9", "9D"};
        size_t patternOffset = 0;
        size_t overwriteLen = 5;
        size_t codeSize = 0x100;

        auto buildFunc = [newVal, overwriteLen, codeSize](uintptr_t codeCaveAddr, uintptr_t hookAddr, const std::vector<BYTE> &originalBytes) -> std::vector<BYTE>
        {
            const size_t dataOffset = 0x80;
            std::vector<BYTE> code(codeSize, 0x90);

            // --- DATA SECTION ---
            std::memcpy(&code[dataOffset], &newVal, sizeof(float));
            uint32_t dataAddr = static_cast<uint32_t>(codeCaveAddr + dataOffset);

            // --- CODE SECTION ---
            size_t wPos = 0;

            // push eax
            code[wPos++] = 0x50;

            // mov eax, [codeCaveAddr]
            code[wPos++] = 0xA1;
            std::memcpy(&code[wPos], &dataAddr, 4);
            wPos += 4;

            // mov [edi+48], eax
            code[wPos++] = 0x89;
            code[wPos++] = 0x47;
            code[wPos++] = 0x48;

            // pop eax
            code[wPos++] = 0x58;

            // fld dword ptr [edi+48] and fmulp st(1),st(0)
            std::memcpy(&code[wPos], originalBytes.data(), originalBytes.size());
            wPos += originalBytes.size();

            // jmp <rel32>
            code[wPos++] = 0xE9;
            uintptr_t returnAddress = hookAddr + overwriteLen;
            uintptr_t nextInstruction = codeCaveAddr + wPos + 4;
            int32_t relativeJump = static_cast<int32_t>(returnAddress - nextInstruction);
            std::memcpy(&code[wPos], &relativeJump, 4);
            wPos += 4;

            return code;
        };

        return createNamedHook(nullptr, "SetMovementSpeed", pat, patternOffset, overwriteLen, codeSize, buildFunc);
    }

    // DLL injection methods
    std::string getItemList()
    {
        if (initializeDllInjection())
        {
            return invokeMethodReturn("", "GCMInjection", "GetItemList", {});
        }
        return "";
    }

    bool spawnItem(int itemIndex)
    {
        if (initializeDllInjection())
        {
            return invokeMethod("", "GCMInjection", "SpawnItem", {itemIndex});
        }
        return false;
    }

    bool addFunds(float amount)
    {
        if (initializeDllInjection())
        {
            return invokeMethod("", "GCMInjection", "AddFunds", {amount});
        }
        return false;
    }

    bool repairAll()
    {
        if (initializeDllInjection())
        {
            return invokeMethod("", "GCMInjection", "RepairAll", {});
        }
        return false;
    }

    bool restockAll()
    {
        if (initializeDllInjection())
        {
            return invokeMethod("", "GCMInjection", "RestockAll", {});
        }
        return false;
    }

    bool clearWeather()
    {
        if (initializeDllInjection())
        {
            return invokeMethod("", "GCMInjection", "ClearWeather", {});
        }
        return false;
    }

    bool freezeTime(bool freeze)
    {
        if (initializeDllInjection())
        {
            return invokeMethod("", "GCMInjection", "FreezeTime", {freeze});
        }
        return false;
    }

private:
    uintptr_t m_pSharedSanity = 0;
};