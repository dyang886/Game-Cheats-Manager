// Trainer.h
#pragma once

#include "MonoBase.h"

class Trainer : public MonoBase
{
public:
    Trainer() : MonoBase(L"DREDGE.exe") {} // x86
    virtual ~Trainer() {}

    static inline const wchar_t *moduleName = L"mono-2.0-bdwgc.dll";

    bool godMode(bool enable)
    {
        if (initializeDllInjection())
        {
            return invokeMethod("", "GCMInjection", "SetGodMode", {enable});
        }
        return false;
    }

    // Sanity is computed on the x87 stack and stored to [edi+18] in two JIT-compiled places. Both
    // caves drop the computed value and load the cheat's instead.
    inline bool setSanity(float newVal)
    {
        // fstp dword ptr [edi+18] ; fld dword ptr [edi+18]
        if (!hookCode("SetSanity_1", CodeSite::aob(nullptr, "D9 5F 18 D9 47 18 D9 1C 24 8D"), 6,
                      [newVal](CaveBuilder &cave)
                      {
                          cave.bytes({0xDD, 0xD8})                   // fstp st(0)
                              .bytes({0xD9, 0x05}).address("sanity") // fld dword ptr [sanity]
                              .original()                            // fstp dword ptr [edi+18] ; fld dword ptr [edi+18]
                              .jmpBack()
                              .label("sanity")
                              .value(newVal);
                      }))
            return false;

        // call <compute sanity> - the call is relocated into the cave
        if (!hookCode("SetSanity_2", CodeSite::aob(nullptr, "E8 ?? ?? ?? ?? D9 5F 18 8B 05"), 5,
                      [newVal](CaveBuilder &cave)
                      {
                          cave.call(cave.originalBranchTarget(1))
                              .bytes({0xDD, 0xD8})                   // fstp st(0)
                              .bytes({0xD9, 0x05}).address("sanity") // fld dword ptr [sanity]
                              .jmpBack()
                              .label("sanity")
                              .value(newVal);
                      }))
        {
            disable("SetSanity_1");
            return false;
        }
        return true;
    }

    // Movement multiplies by [edi+48] (fld dword ptr [edi+48] ; fmulp); the cave sets it first.
    inline bool setMovementSpeed(float newVal)
    {
        return hookCode("SetMovementSpeed", CodeSite::aob(nullptr, "D9 47 48 DE C9 D9 9D"), 5,
                        [newVal](CaveBuilder &cave)
                        {
                            cave.bytes({0x50})                        // push eax
                                .bytes({0xA1}).address("speed")       // mov eax,[speed]
                                .bytes({0x89, 0x47, 0x48})            // mov [edi+48],eax
                                .bytes({0x58})                        // pop eax
                                .original()                           // fld dword ptr [edi+48] ; fmulp st(1),st(0)
                                .jmpBack()
                                .label("speed")
                                .value(newVal);
                        });
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
};
