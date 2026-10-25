// trainer.h
#pragma once

#include "Il2CppBase.h"

class Trainer : public Il2CppBase
{
public:
    Trainer() : Il2CppBase(L"Replanted.exe") {} // x64
    ~Trainer() override = default;

    // Constants
    static inline const wchar_t *moduleName = L"GameAssembly.dll";

    // The cooldown check's jne becomes je.
    inline bool noPlantCooldown()
    {
        return patchCode("NoPlantCooldown", CodeSite::aob(moduleName, "0F 85 25 04 00 00 48 8B 43", 1), {0x84});
    }

    inline bool noPlantCost()
    {
        // 'jle' (7E) -> 'jmp' (EB): the cost check always passes.
        if (!patchCode("NoPlantCost1",
                       CodeSite::aob(moduleName, "7E ?? 48 ?? ?? ?? ?? ?? ?? 48 ?? ?? 0F ?? ?? ?? ?? ?? 4C ?? ?? ?? 4D ?? ?? 0F ?? ?? ?? ?? ?? 48 ?? ?? ?? ?? ?? ?? B9 ?? ?? ?? ?? 41 ?? ?? ?? ?? ??"),
                       {0xEB}))
            return false;

        // 'setle al' (0F 9E C0) -> 'mov al,1 ; nop' (B0 01 90).
        if (!patchCode("NoPlantCost2",
                       CodeSite::aob(moduleName, "0F ?? ?? 48 ?? ?? ?? 5F C3 E8 ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? 48 ?? ?? ?? ?? 57 48 ?? ?? ?? 80 ?? ?? ?? ?? ?? ?? 8B ??"),
                       {0xB0, 0x01, 0x90}))
        {
            disable("NoPlantCost1");
            return false;
        }

        // 'sub [rax+10],r13d' (44 29 68 10) -> NOPs: the sun cost is never subtracted.
        if (!patchCode("NoPlantCost3",
                       CodeSite::aob(moduleName, "44 ?? ?? ?? 44 ?? ?? ?? ?? ?? ?? ?? 41 ?? ?? ?? ?? ?? 41 ?? ?? 48 ?? ?? ?? ??"),
                       {0x90, 0x90, 0x90, 0x90}))
        {
            disable("NoPlantCost1");
            disable("NoPlantCost2");
            return false;
        }
        return true;
    }

    // The sun gain is `add [rax+10],esi`; the cave loads esi with the amount to add first.
    inline bool addSun(int newVal)
    {
        return hookCode("AddSun", CodeSite::aob(nullptr, "01 70 10 48 8B 8B E0 01 00 00"), 10,
                        [newVal](CaveBuilder &cave)
                        {
                            cave.ripRelative({0x8B, 0x35}, "amount") // mov esi,[amount]
                                .original()                          // add [rax+10],esi ; mov rcx,[rbx+1E0]
                                .jmpBack()
                                .label("amount")
                                .value(newVal);
                        });
    }

    // Every change to fertilizer or bug spray stores the cheat's value instead.
    inline bool setFertilizerAndBugSpray(int newVal)
    {
        // 'add dword ptr [rcx+rdi*4+20],05' - a purchase
        if (!hookCode("SetFertilizerAndBugSprayInc", CodeSite::aob(nullptr, "83 44 B9 20 05"), 5,
                      [newVal](CaveBuilder &cave)
                      {
                          cave.bytes({0xC7, 0x44, 0xB9, 0x20}).value(newVal) // mov dword ptr [rcx+rdi*4+20],newVal
                              .jmpBack();
                      }))
            return false;

        // 'dec [rax+58] ; xor edx,edx' - fertilizer used
        if (!hookCode("SetFertilizerDec", CodeSite::aob(nullptr, "FF 48 58 33 D2"), 5,
                      [newVal](CaveBuilder &cave)
                      {
                          cave.bytes({0xC7, 0x40, 0x58}).value(newVal) // mov dword ptr [rax+58],newVal
                              .original(3, 2)                          // xor edx,edx
                              .jmpBack();
                      }))
        {
            disable("SetFertilizerAndBugSprayInc");
            return false;
        }

        // 'dec [rax+5C] ; jmp <xor edx,edx>' - bug spray used. The cave runs the xor itself and
        // continues after it.
        if (!hookCode("SetBugSprayDec", CodeSite::aob(nullptr, "FF 48 5C E9 06 01 00 00"), 8,
                      [newVal](CaveBuilder &cave)
                      {
                          cave.bytes({0xC7, 0x40, 0x5C}).value(newVal) // mov dword ptr [rax+5C],newVal
                              .bytes({0x33, 0xD2})                     // xor edx,edx
                              .jmp(cave.originalBranchTarget(4) + 2);  // past the xor the jmp went to
                      }))
        {
            disable("SetFertilizerAndBugSprayInc");
            disable("SetFertilizerDec");
            return false;
        }
        return true;
    }

    // 'inc / dec [rax+00000088]' both store the cheat's chocolate count instead.
    inline bool setChocolate(int newVal)
    {
        auto store = [newVal](CaveBuilder &cave)
        {
            cave.bytes({0xC7, 0x80, 0x88, 0x00, 0x00, 0x00}).value(newVal) // mov dword ptr [rax+88],newVal
                .jmpBack();
        };
        if (!hookCode("SetChocolateInc", CodeSite::aob(nullptr, "FF 80 88 00 00 00 C7"), 6, store))
            return false;
        if (!hookCode("SetChocolateDec", CodeSite::aob(nullptr, "FF 88 88 00 00 00 33"), 6, store))
        {
            disable("SetChocolateInc");
            return false;
        }
        return true;
    }

    // 'inc / dec [rax+00000090]' both store the cheat's tree food count instead.
    inline bool setTreeFood(int newVal)
    {
        auto store = [newVal](CaveBuilder &cave)
        {
            cave.bytes({0xC7, 0x80, 0x90, 0x00, 0x00, 0x00}).value(newVal) // mov dword ptr [rax+90],newVal
                .jmpBack();
        };
        if (!hookCode("TreeFoodInc", CodeSite::aob(nullptr, "FF 80 90 00 00 00"), 6, store))
            return false;
        if (!hookCode("TreeFoodDec", CodeSite::aob(nullptr, "FF 88 90 00 00 00 48 8B 4B"), 6, store))
        {
            disable("TreeFoodInc");
            return false;
        }
        return true;
    }

    // 'add [rax+50],ebx ; add rsp,20' - the coin gain stores the cheat's count instead.
    inline bool setCoin(int newVal)
    {
        return hookCode("SetCoin", CodeSite::aob(nullptr, "01 58 50 48 83 C4 20"), 7,
                        [newVal](CaveBuilder &cave)
                        {
                            cave.bytes({0xC7, 0x40, 0x50}).value(newVal) // mov dword ptr [rax+50],newVal
                                .original(3, 4)                          // add rsp,20
                                .jmpBack();
                        });
    }

    std::string getPlantList()
    {
        if (initializeDllInjection())
        {
            return invokeMethodReturn("GetPlantList");
        }
        return "";
    }

    bool addPlantToGarden(int plantID, int direction = 0)
    {
        if (initializeDllInjection())
        {
            struct PlantArgs
            {
                int plantID;
                int direction; // 0 = Right, 1 = Left
            };
            PlantArgs args = {plantID, direction};
            return invokeMethod<PlantArgs>("AddPlantToGarden", args);
        }
        return false;
    }

    bool instantCompleteLevel()
    {
        if (initializeDllInjection())
        {
            return invokeMethod("InstantCompleteLevel");
        }
        return false;
    }

    std::string getZombieList()
    {
        if (initializeDllInjection())
        {
            return invokeMethodReturn("GetZombieList");
        }
        return "";
    }

    bool spawnZombie(int zombieType, int row, bool isEveryRow = false)
    {
        if (initializeDllInjection())
        {
            struct ZombieArgs
            {
                int type;
                int row;
                bool isEveryRow;
            };
            ZombieArgs args = {zombieType, row, isEveryRow};
            return invokeMethod<ZombieArgs>("SpawnZombie", args);
        }
        return false;
    }

    bool fullScreenJalapeno()
    {
        if (initializeDllInjection())
        {
            return invokeMethod("FullScreenJalapeno");
        }
        return false;
    }
};
