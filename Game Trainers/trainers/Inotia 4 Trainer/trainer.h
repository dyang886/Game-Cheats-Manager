// trainer.h
#pragma once

#include "TrainerBase.h"

class Trainer : public TrainerBase
{
public:
    Trainer() : TrainerBase(L"Inotia4.exe") {} // x64
    ~Trainer() override = default;

    // Constants
    static inline const wchar_t *moduleName = L"Inotia4_port-Native.dll";

    inline bool freezeHealth()
    {
        std::vector<unsigned int> offsets = {0x003F9720, 0x1F0};
        return createPointerToggle(moduleName, "FreezeHealth", offsets, nullptr, (int *)nullptr);
    }

    inline bool freezeMana()
    {
        std::vector<unsigned int> offsets = {0x003F9720, 0x1F4};
        return createPointerToggle(moduleName, "FreezeMana", offsets, nullptr, (int *)nullptr);
    }

    inline bool setExp(int newVal)
    {
        std::vector<unsigned int> offsets = {0x003F9720, 0x318};
        return WriteToDynamicAddress(moduleName, offsets, newVal);
    }

    inline bool setCoins(int newVal)
    {
        std::vector<unsigned int> offsets = {0x40F2F8};
        return WriteToDynamicAddress(moduleName, offsets, newVal);
    }

    inline bool setStatPoints(int newVal)
    {
        std::vector<unsigned int> offsets = {0x3c7a08};
        return WriteToDynamicAddress(moduleName, offsets, newVal);
    }

    inline bool setSkillPoints(int newVal)
    {
        std::vector<unsigned int> offsets = {0x003F9720, 0x330};
        return WriteToDynamicAddress(moduleName, offsets, newVal);
    }

    inline unsigned int getCurMouseItem()
    {
        std::vector<unsigned int> offsets = {0x388200};
        return ReadFromDynamicAddress<unsigned int>(moduleName, offsets);
    }

    inline unsigned int getSlot1Item()
    {
        std::vector<unsigned int> offsets = {0x40F300};
        return ReadFromDynamicAddress<unsigned int>(moduleName, offsets);
    }

    inline bool setSlot1Item(int itemAddress)
    {
        std::vector<unsigned int> offsets = {0x40F300};
        return WriteToDynamicAddress(moduleName, offsets, itemAddress);
    }

    inline bool unlockAllMercenarySlots()
    {
        std::vector<unsigned int> offsets = {0x003E2D18, 0x97};
        uintptr_t startAddr = resolveModuleDynamicAddress(moduleName, offsets);

        if (startAddr == 0)
        {
            std::cerr << "[!] Failed to resolve mercenary slots base pointer.\n";
            return false;
        }

        BYTE val = 92;
        bool allSuccess = true;
        for (int i = 0; i < 14; i++)
        {
            uintptr_t currentAddr = startAddr + (i * 20);
            if (!WriteProcessMemory(hProcess, (LPVOID)currentAddr, &val, sizeof(BYTE), nullptr))
            {
                std::cerr << "[!] Failed to unlock slot " << i << "\n";
                allSuccess = false;
            }
        }

        return allSuccess;
    }
};
