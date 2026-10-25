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
        return freezeValue<int>("FreezeHealth", {moduleName, {0x003E4D38, 0x1F0}});
    }

    inline bool freezeMana()
    {
        return freezeValue<int>("FreezeMana", {moduleName, {0x003E4D38, 0x1F4}});
    }

    inline bool setExp(int newVal)
    {
        return hashedFieldHook("SetExp", {moduleName, {0x003E4D38, 0x318}}, newVal);
    }

    inline bool setCoins(int newVal)
    {
        return writeValue(PointerChain{moduleName, {0x411618}}, newVal);
    }

    inline bool setStatPoints(int newVal)
    {
        return hashedFieldHook("SetStatPoints", {moduleName, {0x003E4D38, 0x340}}, newVal);
    }

    inline bool setSkillPoints(int newVal)
    {
        return hashedFieldHook("SetSkillPoints", {moduleName, {0x003E4D38, 0x330}}, newVal);
    }

    inline unsigned int getCurMouseItem()
    {
        unsigned int item = 0;
        readValue(PointerChain{moduleName, {0x38A470}}, item);
        return item;
    }

    inline unsigned int getSlot1Item()
    {
        unsigned int item = 0;
        readValue(PointerChain{moduleName, {0x411620}}, item);
        return item;
    }

    inline bool setSlot1Item(int itemAddress)
    {
        return writeValue(PointerChain{moduleName, {0x411620}}, itemAddress);
    }

    inline bool unlockAllMercenarySlots()
    {
        uintptr_t startAddr = resolve({moduleName, {0x003E4F40, 0x8}});
        if (startAddr == 0)
            return false;

        // A locked slot reads FF FF FF FF 00 00 00 00 after its first byte; an unlocked one starts 92 (0x5C).
        const BYTE locked[] = {0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00};
        std::vector<BYTE> buffer(4096);
        if (!readBytes(startAddr, buffer.data(), buffer.size()))
        {
            std::cerr << "[!] Could not read the mercenary slots at 0x" << std::hex << startAddr << std::dec << ".\n";
            return false;
        }

        bool allPatched = true;
        for (size_t i = 1; i + sizeof(locked) <= buffer.size(); i++)
        {
            if (std::memcmp(&buffer[i], locked, sizeof(locked)) == 0)
                allPatched = writeMemory<BYTE>(startAddr + i - 1, 92) && allPatched;
        }
        return allPatched;
    }

private:
    // The game hashes stat writes through one `imul eax,edx,9E3779B9`, with rcx the field being
    // written and edx its value. The cave swaps in the cheat's value when rcx is this stat's field.
    // Every such cheat hooks the same instruction, so only one can be active at a time - main.cpp
    // keeps them exclusive.
    inline bool hashedFieldHook(const std::string &name, const PointerChain &field, int value)
    {
        uintptr_t fieldAddress = resolve(field);
        if (fieldAddress == 0)
            return false;

        return hookCode(name, CodeSite::aob(moduleName, "69 C2 B9 79 37 9E"), 6,
                        [fieldAddress, value](CaveBuilder &cave)
                        {
                            cave.bytes({0x48, 0xB8}).value<uint64_t>(fieldAddress) // mov rax,field
                                .bytes({0x48, 0x39, 0xC1})                         // cmp rcx,rax
                                .jumpTo(CaveBuilder::Jcc::NE, "original")
                                .bytes({0xBA}).value(value)                        // mov edx,value
                                .label("original")
                                .original()                                        // imul eax,edx,9E3779B9
                                .jmpBack();
                        });
    }
};
