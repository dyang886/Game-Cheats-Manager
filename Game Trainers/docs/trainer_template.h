#pragma once

#include "TrainerBase.h" // Your base class providing the helper functions
#include "MonoBase.h"    // For Unity games (if needed)

/**
 * @file trainer_template.h
 * @brief A template for creating a new game-specific trainer.
 *
 * This file serves as a guide, showing examples of different types of
 * trainer functions (cheats) you can implement using the TrainerBase.
 *
 * --- How to use this template ---
 * 1. Copy this file and rename it (e.g., "MyGameTrainer.h").
 * 2. Change the game's executable name in the constructor (e.g., "WOL2-Win64-Shipping.exe").
 * 3. Change the 'moduleName' constant to match the .exe or a .dll (e.g., "GameAssembly.dll").
 * 4. Find your offsets/AOBs/methods and fill in the function you need.
 */
class Trainer : public TrainerBase // Or MonoBase if it's a Unity game
{
public:
    /**
     * @brief Constructor.
     * @param gameProcessName The executable name of the game (e.g., "MyGame-Win64-Shipping.exe").
     */
    Trainer() : TrainerBase(L"MyGame-Win64-Shipping.exe") // <-- TODO: Change this
    {
    }

    ~Trainer() override = default;

    // --- Constants ---
    // TODO: Change this to your game's module to be used (can be .exe or a .dll like GameAssembly.dll)
    static inline const wchar_t *moduleName = L"MyGame-Win64-Shipping.exe";

    //=====================================================================
    // SECTION 1: POINTER TOGGLES (Freezing Values)
    //=====================================================================
    //
    // Use this type for cheats that need to "freeze" a value, like
    // "God Mode" (infinite health) or "Infinite Ammo."
    // This works by repeatedly writing 'newVal' to the address found
    // by resolving the pointer chain.
    //
    // Base Function: createPointerToggle(moduleName, "CheatName", offsets, newVal)
    //

    /**
     * @brief Sets and freezes the player's health.
     * @param newVal The value to freeze health at (e.g., 100.0f).
     */
    inline bool setInfiniteHealth(float newVal)
    {
        // Base address is from 'moduleName'
        std::vector<unsigned int> offsets = {0x01ABCDEF, 0x10, 0x20, 0x30}; // <-- TODO: Find your offsets
        return createPointerToggle(moduleName, "SetHealth", offsets, newVal);
    }

    //=====================================================================
    // SECTION 2: POINTER WRITES (One-Time Set)
    //=====================================================================
    //
    // Use this type for cheats that set a value *once*, like
    // "Give 9999 Coins" or "Set Level."
    // This resolves the pointer chain and writes the value just one time.
    //
    // Base Function: WriteToDynamicAddress(moduleName, offsets, newVal)
    //

    /**
     * @brief Sets the player's coins to a specific amount.
     * @param amount The amount to set (e.g., 9999).
     */
    inline bool setCoins(int amount)
    {
        // Base address is from 'moduleName'
        std::vector<unsigned int> offsets = {0x03DEF456, 0x18, 0x28, 0x38}; // <-- TODO: Find your offsets
        return WriteToDynamicAddress(moduleName, offsets, amount);
    }

    //=====================================================================
    // SECTION 3: AOB SCAN & INJECTION (Code Cave Hooking)
    //=====================================================================
    //
    // Use this type for complex cheats that modify game logic, such as
    // "Multiply Coin Gain," or for replacing instructions with
    // more complex logic (e.g., replacing 'inc' with a 'mov').
    // This finds a unique array of bytes (AOB) and "hooks" it to
    // run your custom assembly code in an allocated "code cave".
    //
    // Base Function: createNamedHook(moduleName, "HookName", pat, offset, len, size, buildFunc)
    //

    /**
     * @brief Modifies coin gain (e.g., multiplies it) by patching game code.
     * @param multiplier The value to use (e.g., 10).
     */
    inline bool patchCoinGain(int multiplier)
    {
        // Example: Patches code that looks like "add [rax+50], ebx"
        const std::vector<std::string> pat = {"01", "58", "50", "48", "83", "C4", "20"}; // <-- TODO: Find AOB
        size_t patternOffset = 0;   // 0 = hook at the start of the pattern
        size_t overwriteLen = 7;    // 7 = length of the bytes in 'pat'
        size_t codeSize = 0x100;    // 256 bytes for our code cave

        // The buildFunc writes the custom assembly
        auto buildFunc = [multiplier, overwriteLen, codeSize](uintptr_t codeCaveAddr, uintptr_t hookAddr, const std::vector<BYTE> &originalBytes) -> std::vector<BYTE>
        {
            const size_t dataOffset = 0x80;
            std::vector<BYTE> code(codeSize, 0x90);

            // --- DATA SECTION ---
            std::memcpy(&code[dataOffset], &multiplier, sizeof(int));
            uintptr_t dataAbsoluteAddr = codeCaveAddr + dataOffset;

            // --- CODE SECTION ---
            size_t wPos = 0;

            // --- Custom Assembly (32/64-bit safe) ---
            // TODO: Customize this section. This example loads value into 'ebx'.
            // Note that x64 can't mov absolute addresses into registers directly,
            // so we use RIP-relative addressing there.

            if (sizeof(uintptr_t) == 8)
            {
                // --- 64-BIT (x64) PATH ---
                // We MUST use RIP-relative addressing.
                // mov ebx, [rip + rel32] => 8B 1D <rel32>
                code[wPos++] = 0x8B;
                code[wPos++] = 0x1D; // 0x1D=ebx, 0x05=eax, 0x0D=ecx, 0x15=edx

                // Calculate the relative offset from the *next* instruction
                uintptr_t nextInstrAddr = codeCaveAddr + wPos + 4;
                int32_t rel32 = static_cast<int32_t>(dataAbsoluteAddr - nextInstrAddr);

                std::memcpy(&code[wPos], &rel32, 4);
                wPos += 4;
            }
            else if (sizeof(uintptr_t) == 4)
            {
                // --- 32-BIT (x86) PATH ---
                // We can use an absolute 32-bit address.
                // mov ebx, [absolute_addr32] => 8B 1D <addr32>
                code[wPos++] = 0x8B;
                code[wPos++] = 0x1D; // 0x1D=ebx, 0x05=eax, 0x0D=ecx, 0x15=edx

                // Cast the address to 32-bit (which is safe in 32-bit mode)
                uint32_t absAddr32 = static_cast<uint32_t>(dataAbsoluteAddr);

                std::memcpy(&code[wPos], &absAddr32, 4);
                wPos += 4;
            }
            // --- End Custom Assembly ---

            // Copy original instructions (the ones we overwrote)
            std::memcpy(&code[wPos], originalBytes.data(), originalBytes.size());
            wPos += originalBytes.size();

            // jmp back to the original code
            code[wPos++] = 0xE9;
            uintptr_t returnAddr = hookAddr + overwriteLen;
            uintptr_t nextInstrAddr = codeCaveAddr + wPos + 4;
            int32_t relativeJump = static_cast<int32_t>(returnAddr - nextInstrAddr);
            std::memcpy(&code[wPos], &relativeJump, 4);
            wPos += 4;

            return code;
        };

        return createNamedHook(moduleName, "PatchCoinGain", pat, patternOffset, overwriteLen, codeSize, buildFunc);
    }

    //=====================================================================
    // SECTION 4: MONO DLL INJECTION (Unity Games)
    //=====================================================================
    //
    // Use this type for Unity games to call C# methods directly.
    // This requires a separate C# DLL (e.g., "GCMInjection.dll")
    // to be injected into the game's Mono runtime.
    //
    // Base Functions:
    // initializeDllInjection() - Call this once.
    // invokeMethod(namespace, "ClassName", "MethodName", {args})
    // invokeMethodReturn(namespace, "ClassName", "MethodName", {args})
    //

    /**
     * @brief Spawns an item using a C# method from your injected DLL.
     * @param itemIndex The ID of the item to spawn.
     */
    inline bool spawnItem(int itemIndex)
    {
        if (initializeDllInjection())
        {
            // Calls: GCMInjection.SpawnItem(itemIndex)
            return invokeMethod(
                "",                 // C# namespace (empty for global)
                "GCMInjection", // <-- TODO: Your C# class name
                "SpawnItem",    // <-- TODO: Your C# method name
                {itemIndex}         // Arguments
            );
        }
        return false;
    }

    //=====================================================================
    // SECTION 5: BYTE PATCHING (Simple In-Place Writes)
    //=====================================================================
    //
    // Use this type for simple, direct memory patches where you are not
    // injecting a code cave. This is perfect for changing one instruction
    // into another (e.g., JNE -> JE) or NOP-ing out an instruction.
    //
    // (This requires the 'createBytePatch' function in your TrainerBase.h)
    //
    // Base Function: createBytePatch(moduleName, "CheatName", pat, patchOffset, newBytes)
    //

    /**
     * @brief Disables plant cooldown by patching a JNE instruction to a JE.
     */
    inline bool setNoPlantCooldown()
    {
        // AOB to find the unique instruction
        const std::vector<std::string> pat = {"0F", "85", "25", "04", "00", "00", "48", "8B", "43"}; // <-- TODO: Find your AOB
        
        // Patch at an offset of 1 byte from the start of the pattern.
        size_t patchOffset = 1;

        // The new byte(s) to write. We are changing 0F 85 (JNE) to 0F 84 (JE).
        // Since the 0F is at offset 0, we only need to write the 84 at offset 1.
        const std::vector<BYTE> newBytes = { 0x84 }; // 0x84 = JE

        // Call the byte patch function
        return createBytePatch(
            moduleName,
            "NoPlantCooldown",  // Hook name
            pat,                // AOB pattern
            patchOffset,        // Offset from pattern start
            newBytes            // Bytes to write
        );
    }
};