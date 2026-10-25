// trainer.h
#pragma once

#include "TrainerBase.h"

class Trainer : public TrainerBase
{
public:
    Trainer() : TrainerBase(L"OilRush.exe") {} // x86
    ~Trainer() override = default;

    // Constants
    static inline const wchar_t *moduleName = L"Unigine_x86.dll";

    // Internal state trackers: the value each resource is forced to, -1 when it is not
    int currentOilTarget = -1;
    int currentAbilityTarget = -1;

    // --- CLEAN UI WRAPPERS ---
    inline bool toggleOil(bool active, int value)
    {
        currentOilTarget = active ? value : -1;
        return setMasterResources(currentOilTarget, currentAbilityTarget);
    }

    inline bool toggleAbility(bool active, int value)
    {
        currentAbilityTarget = active ? value : -1;
        return setMasterResources(currentOilTarget, currentAbilityTarget);
    }

private:
    static inline const std::string HOOK = "MasterResourceHook";

    // Hook Overview: Hijacks the engine's globally shared Sync_Component_Value read instruction (Unigine_x86.dll+90336) to intercept all resource updates.
    // Pointer Traversal: Dynamically traverses static pointer chains inside the code cave (7 levels for Oil, 3 for Upgrades) to find the player's exact HUD values.
    // Filtering: Identifies player resources by matching the current object's value against the HUD value and verifying the Team ID is 0.
    // State Management (-1 Safeguard): Compares the user's target cheat value to -1; if it matches, it safely skips execution to preserve original memory and prevent register corruption.
    // Memory Hijack: Forcefully overwrites the entity's real resource base with the custom target value and updates eax to instantly sync the visual UI mirror.
    inline bool setMasterResources(int targetOil, int targetAbilityPoints)
    {
        // 1. Neither resource forced: remove the hook.
        if (targetOil == -1 && targetAbilityPoints == -1)
            return !isActive(HOOK) || disable(HOOK);

        // 2. Hook already in place: change the targets it reads.
        if (isActive(HOOK))
            return writeCaveData(HOOK, "oilTarget", targetOil) && writeCaveData(HOOK, "upgradeTarget", targetAbilityPoints);

        // 3. Create the hook.
        const uint32_t playerRoot = static_cast<uint32_t>(moduleAddress(moduleName, 0x00671B28));
        if (playerRoot == 0)
            return false;

        return hookCode(HOOK, CodeSite::rva(moduleName, 0x90336), 6,
                        [playerRoot, targetOil, targetAbilityPoints](CaveBuilder &cave)
                        {
                            // Walks [playerRoot] + offsets with ebx and bails unless the value being
                            // synced (eax) is that HUD value and belongs to team 0.
                            auto matchesHud = [&](std::initializer_list<uint32_t> offsets, uint32_t hudOffset, const std::string &bail)
                            {
                                cave.bytes({0x8B, 0x1D}).value(playerRoot) // mov ebx,[playerRoot]
                                    .bytes({0x85, 0xDB}).jumpTo(CaveBuilder::Jcc::E, bail);
                                for (uint32_t offset : offsets)
                                {
                                    if (offset < 0x80)
                                        cave.bytes({0x8B, 0x5B, static_cast<BYTE>(offset)}); // mov ebx,[ebx+disp8]
                                    else
                                        cave.bytes({0x8B, 0x9B}).value(offset); // mov ebx,[ebx+disp32]
                                    cave.bytes({0x85, 0xDB}).jumpTo(CaveBuilder::Jcc::E, bail); // test ebx,ebx
                                }
                                cave.bytes({0x8B, 0x9B}).value(hudOffset)                     // mov ebx,[ebx+hud]
                                    .bytes({0x39, 0xD8}).jumpTo(CaveBuilder::Jcc::NE, bail)   // cmp eax,ebx
                                    .bytes({0x83, 0x7E, 0xE0, 0x00}).jumpTo(CaveBuilder::Jcc::NE, bail); // cmp dword ptr [esi-20],0
                            };
                            // Forces the synced value to the target, unless the target is -1.
                            auto force = [&](const std::string &target, const std::string &bail)
                            {
                                cave.bytes({0x8B, 0x1D}).address(target)                   // mov ebx,[target]
                                    .bytes({0x83, 0xFB, 0xFF}).jumpTo(CaveBuilder::Jcc::E, bail) // cmp ebx,-1
                                    .bytes({0x89, 0x5E, 0x10})                             // mov [esi+10],ebx
                                    .bytes({0x8B, 0xC3});                                  // mov eax,ebx
                            };

                            cave.bytes({0x8B, 0x46, 0x10, 0x53}); // mov eax,[esi+10] ; push ebx

                            matchesHud({0x0D8, 0x00C, 0x120, 0x77C, 0x6A0, 0x064}, 0xCCC, "upgrades");
                            force("oilTarget", "upgrades");
                            cave.jumpTo(CaveBuilder::Jcc::Always, "cleanup");

                            cave.label("upgrades");
                            matchesHud({0x844, 0x0A4}, 0x620, "cleanup");
                            force("upgradeTarget", "cleanup");

                            cave.label("cleanup")
                                .bytes({0x5B, 0x89, 0x47, 0x10}) // pop ebx ; mov [edi+10],eax
                                .jmpBack()
                                .label("oilTarget")
                                .value(targetOil)
                                .label("upgradeTarget")
                                .value(targetAbilityPoints);
                        });
    }
};
