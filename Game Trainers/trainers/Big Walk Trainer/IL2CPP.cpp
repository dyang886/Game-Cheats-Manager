// Big Walk trainer payload (IL2CPP / Unity 6000.3.17f1).
//
// The trainer executable injects this DLL and calls the exported functions below on
// remote threads. Those threads only record the requested state; every game-side write
// happens inside a detour, which runs on the game's own runtime-attached thread.
//
// Always Day
//   The world clock is Enviro 3. Enviro.EnviroTimeModule.UpdateModule() advances the
//   clock and then rebuilds the date (and the sun/moon position) from the serial fields
//   on Enviro.EnviroTime - so EnviroTime.timeOfDay is an output, not the driver. Pinning
//   hourSerial/minSerial/secSerial to noon *before* the original runs makes it recompute
//   noon every frame. That also beats LobbyEnviroNetworking, which pushes the host's
//   clock onto clients and would fight a one-shot SkyManager.SetFixedTime().
//
// Movement Speed Multiplier
//   PlayerMover.GetForwardSpeed() returns the speed for the current movement mode (walk,
//   sprint, crouch, swim, ghost) after reading PlayerTunings, and is called from exactly
//   one place: PlayerMover.LocalFixedUpdate(), which FixedUpdate() only reaches behind an
//   isLocalPlayer check. Scaling the return value therefore covers every movement mode
//   for the local player only, and needs no state to restore when switched off.
//
// No Tripping
//   Landing from too high makes PlayerFaller sit you down, drop what you were holding and
//   daze you for sitPauseDuration. That whole reaction is inlined into PlayerFaller.Update()
//   behind the game's own `ignoreFalling` debug flag, and it is the only path into it -
//   the standalone TriggerFall() has no callers. So the detour just raises ignoreFalling
//   for the duration of the tick and puts it back afterwards.
//   The fall *measurement* (heightLastGrounded, isInDanger) happens before that flag is
//   read, and remote players' trips arrive through PlayerNetworking -> ProcessRemoteFall(),
//   which is untouched, so applying this to every faller costs other players nothing.
//
// Jump Height Multiplier
//   PlayerJumper.LocalFixedUpdate() is inlined into PlayerMover.LocalFixedUpdate() (it has
//   no call sites of its own), so the jumper itself is not hookable. Instead the detour
//   sits on PlayerMover.LocalFixedUpdate(out float velY) and scales the returned velY,
//   which is the vertical velocity the caller applies to the rigidbody. It only scales on
//   ticks where PlayerJumper.jumpInQueue was set going in - sampled before the original,
//   which consumes it - so falling and normal vertical motion are left alone.
//   That impulse alone is not enough: the original reads rigidbody.linearVelocity.y back in
//   on *every* tick and re-clamps it to PlayerTunings.maxUpwardsVelocity, so a boosted jump
//   is clawed back to the ceiling on the very next tick and the height plateaus once the
//   multiplier passes maxUpwardsVelocity/jumpForce. So the detour also lifts that ceiling by
//   the same multiplier for the duration of each call and restores it afterwards. (Only when
//   scaling up - multipliers below 1 want a shorter jump, not a lower ceiling on everything
//   else.) The other clamp in there is a constant Mathf.Clamp(velY, -500, 500) sanity guard,
//   which only bites at extreme settings and is left alone.
//   Note this multiplies jump *velocity*; height rises with its square.

#pragma warning(disable : 4703)
#pragma warning(disable : 4996)

#include <iostream>
#include "il2cpp/il2cpp.h"

using namespace IL2CPP;

// =============================================================
// IL2CPP names
// =============================================================

const char *GAME_ASSEMBLY = "Assembly-CSharp";
const char *GAME_NAMESPACE = ""; // PlayerMover lives in the global namespace
const char *ENVIRO_ASSEMBLY = "Enviro3.Runtime";
const char *ENVIRO_NAMESPACE = "Enviro";

// Classes
const char *PLAYER_MOVER_CLASS = "PlayerMover";
const char *PLAYER_CHARACTER_CLASS = "PlayerCharacter";
const char *PLAYER_JUMPER_CLASS = "PlayerJumper";
const char *PLAYER_TUNINGS_CLASS = "PlayerTunings";
const char *PLAYER_FALLER_CLASS = "PlayerFaller";
const char *ENVIRO_TIME_MODULE_CLASS = "EnviroTimeModule";
const char *ENVIRO_TIME_CLASS = "EnviroTime";

// Methods
const char *GET_FORWARD_SPEED_METHOD = "GetForwardSpeed";
const char *LOCAL_FIXED_UPDATE_METHOD = "LocalFixedUpdate";
const char *UPDATE_METHOD = "Update";
const char *UPDATE_MODULE_METHOD = "UpdateModule";

// Fields
const char *SETTINGS_FIELD = "Settings";            // EnviroTimeModule -> EnviroTime
const char *SEC_SERIAL_FIELD = "secSerial";         // EnviroTime
const char *MIN_SERIAL_FIELD = "minSerial";         // EnviroTime
const char *HOUR_SERIAL_FIELD = "hourSerial";       // EnviroTime
const char *IGNORE_FALLING_FIELD = "ignoreFalling"; // PlayerFaller
const char *PC_FIELD = "pc";                        // PlayerMover -> PlayerCharacter
const char *JUMPER_FIELD = "jumper";                // PlayerCharacter -> PlayerJumper
const char *JUMP_IN_QUEUE_FIELD = "jumpInQueue";    // PlayerJumper
const char *TUNINGS_FIELD = "tunings";              // PlayerCharacter -> PlayerTunings
const char *MAX_UPWARDS_VELOCITY_FIELD = "maxUpwardsVelocity"; // PlayerTunings

// Typedefs (il2cpp passes a hidden trailing const MethodInfo*)
typedef float (*GetForwardSpeed_t)(void *instance, const void *method);
typedef void (*UpdateModule_t)(void *instance, const void *method);
typedef void (*FallerUpdate_t)(void *instance, const void *method);
typedef void (*LocalFixedUpdate_t)(void *instance, float *velY, const void *method);

// The hour Always Day pins the world clock to.
const int NOON_HOUR = 12;

// =============================================================
// Cheat state (written by the exports, read by the detours)
// =============================================================

bool g_AlwaysDay = false;

bool g_MovementSpeedEnabled = false;
float g_MovementSpeedMultiplier = 1.0f;

bool g_NoTripping = false;

bool g_JumpHeightEnabled = false;
float g_JumpHeightMultiplier = 1.0f;

HOOK *get_forward_speed_hook = nullptr;
HOOK *update_module_hook = nullptr;
HOOK *faller_update_hook = nullptr;
HOOK *local_fixed_update_hook = nullptr;

size_t g_SettingsOffset = 0;
size_t g_SecSerialOffset = 0;
size_t g_MinSerialOffset = 0;
size_t g_HourSerialOffset = 0;
bool g_TimeOffsetsResolved = false;

size_t g_IgnoreFallingOffset = 0;
bool g_IgnoreFallingResolved = false;

size_t g_PcOffset = 0;
size_t g_JumperOffset = 0;
size_t g_JumpInQueueOffset = 0;
size_t g_TuningsOffset = 0;
size_t g_MaxUpwardsVelocityOffset = 0;
bool g_JumpOffsetsResolved = false;

bool g_IsInitialized = false;

// =============================================================
// Detours
// =============================================================

/** Enviro's per-frame clock tick - rewind it to noon before it rebuilds the date. */
void Detour_UpdateModule(void *instance, const void *method)
{
    if (g_AlwaysDay && instance && g_TimeOffsetsResolved)
    {
        char *timeModule = static_cast<char *>(instance);
        char *settings = *reinterpret_cast<char **>(timeModule + g_SettingsOffset);
        if (settings)
        {
            *reinterpret_cast<int32_t *>(settings + g_SecSerialOffset) = 0;
            *reinterpret_cast<int32_t *>(settings + g_MinSerialOffset) = 0;
            *reinterpret_cast<int32_t *>(settings + g_HourSerialOffset) = NOON_HOUR;
        }
    }

    if (update_module_hook && update_module_hook->get_original<UpdateModule_t>())
    {
        update_module_hook->get_original<UpdateModule_t>()(instance, method);
    }
}

/** Local player's movement speed for the current movement mode. */
float Detour_GetForwardSpeed(void *instance, const void *method)
{
    float speed = 0.0f;

    if (get_forward_speed_hook && get_forward_speed_hook->get_original<GetForwardSpeed_t>())
    {
        speed = get_forward_speed_hook->get_original<GetForwardSpeed_t>()(instance, method);
    }

    if (g_MovementSpeedEnabled)
    {
        speed *= g_MovementSpeedMultiplier;
    }

    return speed;
}

/** Local player's per-tick movement step, which hands back the vertical velocity. */
void Detour_LocalFixedUpdate(void *instance, float *velY, const void *method)
{
    // The original consumes jumpInQueue, so sample it before handing over.
    bool jumping = false;
    // Raised for the duration of the call so the original does not clamp the ascent back
    // down to the stock ceiling on the ticks after the jump.
    float *maxUpwardsVelocity = nullptr;
    float previousMaxUpwardsVelocity = 0.0f;

    if (g_JumpHeightEnabled && instance && g_JumpOffsetsResolved)
    {
        char *playerCharacter = *reinterpret_cast<char **>(static_cast<char *>(instance) + g_PcOffset);
        if (playerCharacter)
        {
            char *jumper = *reinterpret_cast<char **>(playerCharacter + g_JumperOffset);
            if (jumper)
                jumping = *reinterpret_cast<bool *>(jumper + g_JumpInQueueOffset);

            char *tunings = *reinterpret_cast<char **>(playerCharacter + g_TuningsOffset);
            if (tunings && g_JumpHeightMultiplier > 1.0f)
            {
                maxUpwardsVelocity = reinterpret_cast<float *>(tunings + g_MaxUpwardsVelocityOffset);
                previousMaxUpwardsVelocity = *maxUpwardsVelocity;
                *maxUpwardsVelocity = previousMaxUpwardsVelocity * g_JumpHeightMultiplier;
            }
        }
    }

    if (local_fixed_update_hook && local_fixed_update_hook->get_original<LocalFixedUpdate_t>())
    {
        local_fixed_update_hook->get_original<LocalFixedUpdate_t>()(instance, velY, method);
    }

    if (maxUpwardsVelocity)
    {
        *maxUpwardsVelocity = previousMaxUpwardsVelocity;
    }

    if (jumping && velY)
    {
        *velY *= g_JumpHeightMultiplier;
    }
}

/** Per-player fall bookkeeping - raise the game's own ignoreFalling flag for this tick. */
void Detour_FallerUpdate(void *instance, const void *method)
{
    bool *ignoreFalling = nullptr;
    bool previous = false;

    if (g_NoTripping && instance && g_IgnoreFallingResolved)
    {
        ignoreFalling = reinterpret_cast<bool *>(static_cast<char *>(instance) + g_IgnoreFallingOffset);
        previous = *ignoreFalling;
        *ignoreFalling = true;
    }

    if (faller_update_hook && faller_update_hook->get_original<FallerUpdate_t>())
    {
        faller_update_hook->get_original<FallerUpdate_t>()(instance, method);
    }

    // Restore through the pointer rather than re-reading g_NoTripping, which the trainer
    // can flip on its own thread while the original is running.
    if (ignoreFalling)
    {
        *ignoreFalling = previous;
    }
}

// =============================================================
// Initialization
// =============================================================

/** Reads the EnviroTime field offsets and hooks the clock tick */
void InitializeAlwaysDay(MEMORY &memory)
{
    auto assembly = IL2CPP::Assembly(ENVIRO_ASSEMBLY);
    if (!assembly)
    {
        std::cout << "[!] Failed to find assembly: " << ENVIRO_ASSEMBLY << "\n";
        return;
    }

    auto ns = assembly->Namespace(ENVIRO_NAMESPACE);
    if (!ns)
    {
        std::cout << "[!] Failed to find namespace: " << ENVIRO_NAMESPACE << "\n";
        return;
    }

    auto timeModuleClass = ns->Class(ENVIRO_TIME_MODULE_CLASS);
    if (!timeModuleClass)
    {
        std::cout << "[!] Failed to find class: " << ENVIRO_TIME_MODULE_CLASS << "\n";
        return;
    }

    auto timeClass = ns->Class(ENVIRO_TIME_CLASS);
    if (!timeClass)
    {
        std::cout << "[!] Failed to find class: " << ENVIRO_TIME_CLASS << "\n";
        return;
    }

    auto settingsField = timeModuleClass->Field(SETTINGS_FIELD);
    auto secField = timeClass->Field(SEC_SERIAL_FIELD);
    auto minField = timeClass->Field(MIN_SERIAL_FIELD);
    auto hourField = timeClass->Field(HOUR_SERIAL_FIELD);
    if (!settingsField || !secField || !minField || !hourField)
    {
        std::cout << "[!] Failed to resolve the Enviro clock fields.\n";
        return;
    }

    g_SettingsOffset = settingsField->Offset();
    g_SecSerialOffset = secField->Offset();
    g_MinSerialOffset = minField->Offset();
    g_HourSerialOffset = hourField->Offset();
    g_TimeOffsetsResolved = true;

    auto updateModule = timeModuleClass->Method(UPDATE_MODULE_METHOD, 0);
    if (!updateModule)
    {
        std::cout << "[!] Failed to find method: " << UPDATE_MODULE_METHOD << "\n";
        return;
    }

    update_module_hook = updateModule->Hook<UpdateModule_t>(memory, Detour_UpdateModule);
    if (!update_module_hook || !update_module_hook->active)
    {
        std::cout << "[!] Failed to hook " << UPDATE_MODULE_METHOD << ".\n";
        update_module_hook = nullptr;
        return;
    }

    std::cout << "[+] Always Day ready.\n";
}

/** Hooks the local player's forward speed getter */
void InitializeMovementSpeed(MEMORY &memory)
{
    auto assembly = IL2CPP::Assembly(GAME_ASSEMBLY);
    if (!assembly)
    {
        std::cout << "[!] Failed to find assembly: " << GAME_ASSEMBLY << "\n";
        return;
    }

    auto ns = assembly->Namespace(GAME_NAMESPACE);
    if (!ns)
    {
        std::cout << "[!] Failed to find the global namespace.\n";
        return;
    }

    auto moverClass = ns->Class(PLAYER_MOVER_CLASS);
    if (!moverClass)
    {
        std::cout << "[!] Failed to find class: " << PLAYER_MOVER_CLASS << "\n";
        return;
    }

    auto getForwardSpeed = moverClass->Method(GET_FORWARD_SPEED_METHOD, 0);
    if (!getForwardSpeed)
    {
        std::cout << "[!] Failed to find method: " << GET_FORWARD_SPEED_METHOD << "\n";
        return;
    }

    get_forward_speed_hook = getForwardSpeed->Hook<GetForwardSpeed_t>(memory, Detour_GetForwardSpeed);
    if (!get_forward_speed_hook || !get_forward_speed_hook->active)
    {
        std::cout << "[!] Failed to hook " << GET_FORWARD_SPEED_METHOD << ".\n";
        get_forward_speed_hook = nullptr;
        return;
    }

    std::cout << "[+] Movement Speed Multiplier ready.\n";
}

/** Reads the jump-queue field chain and hooks the local player's movement step */
void InitializeJumpHeight(MEMORY &memory)
{
    auto assembly = IL2CPP::Assembly(GAME_ASSEMBLY);
    if (!assembly)
    {
        std::cout << "[!] Failed to find assembly: " << GAME_ASSEMBLY << "\n";
        return;
    }

    auto ns = assembly->Namespace(GAME_NAMESPACE);
    if (!ns)
    {
        std::cout << "[!] Failed to find the global namespace.\n";
        return;
    }

    auto moverClass = ns->Class(PLAYER_MOVER_CLASS);
    auto characterClass = ns->Class(PLAYER_CHARACTER_CLASS);
    auto jumperClass = ns->Class(PLAYER_JUMPER_CLASS);
    auto tuningsClass = ns->Class(PLAYER_TUNINGS_CLASS);
    if (!moverClass || !characterClass || !jumperClass || !tuningsClass)
    {
        std::cout << "[!] Failed to resolve the player movement classes.\n";
        return;
    }

    auto pcField = moverClass->Field(PC_FIELD);
    auto jumperField = characterClass->Field(JUMPER_FIELD);
    auto jumpInQueueField = jumperClass->Field(JUMP_IN_QUEUE_FIELD);
    auto tuningsField = characterClass->Field(TUNINGS_FIELD);
    auto maxUpwardsVelocityField = tuningsClass->Field(MAX_UPWARDS_VELOCITY_FIELD);
    if (!pcField || !jumperField || !jumpInQueueField || !tuningsField || !maxUpwardsVelocityField)
    {
        std::cout << "[!] Failed to resolve the jump fields.\n";
        return;
    }

    g_PcOffset = pcField->Offset();
    g_JumperOffset = jumperField->Offset();
    g_JumpInQueueOffset = jumpInQueueField->Offset();
    g_TuningsOffset = tuningsField->Offset();
    g_MaxUpwardsVelocityOffset = maxUpwardsVelocityField->Offset();
    g_JumpOffsetsResolved = true;

    auto localFixedUpdate = moverClass->Method(LOCAL_FIXED_UPDATE_METHOD, 1);
    if (!localFixedUpdate)
    {
        std::cout << "[!] Failed to find method: " << PLAYER_MOVER_CLASS << "." << LOCAL_FIXED_UPDATE_METHOD << "\n";
        return;
    }

    local_fixed_update_hook = localFixedUpdate->Hook<LocalFixedUpdate_t>(memory, Detour_LocalFixedUpdate);
    if (!local_fixed_update_hook || !local_fixed_update_hook->active)
    {
        std::cout << "[!] Failed to hook " << PLAYER_MOVER_CLASS << "." << LOCAL_FIXED_UPDATE_METHOD << ".\n";
        local_fixed_update_hook = nullptr;
        return;
    }

    std::cout << "[+] Jump Height Multiplier ready.\n";
}

/** Reads the PlayerFaller debug flag offset and hooks its per-frame tick */
void InitializeNoTripping(MEMORY &memory)
{
    auto assembly = IL2CPP::Assembly(GAME_ASSEMBLY);
    if (!assembly)
    {
        std::cout << "[!] Failed to find assembly: " << GAME_ASSEMBLY << "\n";
        return;
    }

    auto ns = assembly->Namespace(GAME_NAMESPACE);
    if (!ns)
    {
        std::cout << "[!] Failed to find the global namespace.\n";
        return;
    }

    auto fallerClass = ns->Class(PLAYER_FALLER_CLASS);
    if (!fallerClass)
    {
        std::cout << "[!] Failed to find class: " << PLAYER_FALLER_CLASS << "\n";
        return;
    }

    auto ignoreFallingField = fallerClass->Field(IGNORE_FALLING_FIELD);
    if (!ignoreFallingField)
    {
        std::cout << "[!] Failed to find field: " << IGNORE_FALLING_FIELD << "\n";
        return;
    }

    g_IgnoreFallingOffset = ignoreFallingField->Offset();
    g_IgnoreFallingResolved = true;

    auto fallerUpdate = fallerClass->Method(UPDATE_METHOD, 0);
    if (!fallerUpdate)
    {
        std::cout << "[!] Failed to find method: " << PLAYER_FALLER_CLASS << "." << UPDATE_METHOD << "\n";
        return;
    }

    faller_update_hook = fallerUpdate->Hook<FallerUpdate_t>(memory, Detour_FallerUpdate);
    if (!faller_update_hook || !faller_update_hook->active)
    {
        std::cout << "[!] Failed to hook " << PLAYER_FALLER_CLASS << "." << UPDATE_METHOD << ".\n";
        faller_update_hook = nullptr;
        return;
    }

    std::cout << "[+] No Tripping ready.\n";
}

/** Initialize IL2CPP and install the hooks. Each cheat reports its own hook separately,
 *  so one missing target does not take the other option down with it. */
bool InitializeIL2CPP()
{
    if (g_IsInitialized)
        return true;

    MEMORY memory;
    if (!IL2CPP::Initialize(memory))
    {
        std::cout << "[!] Failed to initialize IL2CPP.\n";
        return false;
    }

    IL2CPP::Attach();

    InitializeAlwaysDay(memory);
    InitializeMovementSpeed(memory);
    InitializeJumpHeight(memory);
    InitializeNoTripping(memory);

    g_IsInitialized = true;
    std::cout << "[+] IL2CPP initialization complete!\n";
    return true;
}

// =============================================================
// Exports (called by the trainer on remote threads)
// =============================================================

extern "C" __declspec(dllexport) DWORD WINAPI ToggleAlwaysDay(LPVOID lpParam)
{
    if (!lpParam || !InitializeIL2CPP())
        return 0;

    if (!update_module_hook)
    {
        std::cout << "[!] Always Day is unavailable: the clock hook is not installed.\n";
        return 0;
    }

    g_AlwaysDay = *reinterpret_cast<bool *>(lpParam);
    std::cout << "[+] Always Day: " << (g_AlwaysDay ? "ON" : "OFF") << "\n";
    return 1;
}

/** Layout must match Trainer::MultiplierArgs in trainer.h */
struct MultiplierArgs
{
    bool enable;
    float multiplier;
};

extern "C" __declspec(dllexport) DWORD WINAPI SetMovementSpeedMultiplier(LPVOID lpParam)
{
    MultiplierArgs *args = (MultiplierArgs *)lpParam;
    if (!args || !InitializeIL2CPP())
        return 0;

    if (!get_forward_speed_hook)
    {
        std::cout << "[!] Movement Speed Multiplier is unavailable: the speed hook is not installed.\n";
        return 0;
    }

    g_MovementSpeedMultiplier = args->multiplier;
    g_MovementSpeedEnabled = args->enable;
    std::cout << "[+] Movement Speed Multiplier: " << (g_MovementSpeedEnabled ? "ON" : "OFF")
              << " x" << g_MovementSpeedMultiplier << "\n";
    return 1;
}

extern "C" __declspec(dllexport) DWORD WINAPI SetJumpHeightMultiplier(LPVOID lpParam)
{
    MultiplierArgs *args = (MultiplierArgs *)lpParam;
    if (!args || !InitializeIL2CPP())
        return 0;

    if (!local_fixed_update_hook)
    {
        std::cout << "[!] Jump Height Multiplier is unavailable: the movement hook is not installed.\n";
        return 0;
    }

    g_JumpHeightMultiplier = args->multiplier;
    g_JumpHeightEnabled = args->enable;
    std::cout << "[+] Jump Height Multiplier: " << (g_JumpHeightEnabled ? "ON" : "OFF")
              << " x" << g_JumpHeightMultiplier << "\n";
    return 1;
}

extern "C" __declspec(dllexport) DWORD WINAPI ToggleNoTripping(LPVOID lpParam)
{
    if (!lpParam || !InitializeIL2CPP())
        return 0;

    if (!faller_update_hook)
    {
        std::cout << "[!] No Tripping is unavailable: the faller hook is not installed.\n";
        return 0;
    }

    g_NoTripping = *reinterpret_cast<bool *>(lpParam);
    std::cout << "[+] No Tripping: " << (g_NoTripping ? "ON" : "OFF") << "\n";
    return 1;
}

// =============================================================
// DllMain
// =============================================================

DWORD WINAPI MainThread(HMODULE hModule)
{
#ifdef _DEBUG
    AllocConsole();
    freopen_s((FILE **)(stdout), "CONOUT$", "w", stdout);
#endif
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        CreateThread(nullptr, NULL, (LPTHREAD_START_ROUTINE)MainThread, hModule, NULL, nullptr);
        break;
    }
    return TRUE;
}
