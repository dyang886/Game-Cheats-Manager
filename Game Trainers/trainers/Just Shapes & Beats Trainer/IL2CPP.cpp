#include <iostream>
#include "il2cpp/il2cpp.h"

using namespace IL2CPP;

// =============================================================
// Constants & Config
// =============================================================

const char *ASSEMBLY_GAME = "Assembly-CSharp";
const char *NAMESPACE_GLOBAL = "";

// Class Names
const char *CLASS_METAGAME = "MetaGameProgress";
const char *CLASS_SAVE_MANAGER = "SaveGameManager";
const char *CLASS_SCENE_MANAGER = "GameSceneManager";
const char *CLASS_LIFECOMPONENT = "LifeComponent";
const char *CLASS_HERO_COLLISION = "HeroCollisionWithEnemy";
const char *CLASS_LOGIC_CHECKPOINT = "LogicCheckPointCheck";

// Method Names
const char *METHOD_ADD_BP_AND_BANK = "addBPToPlayerAndBank";
const char *METHOD_SAVE = "Save";
const char *METHOD_UPDATE = "update";
const char *METHOD_DAMAGE = "damage";
const char *METHOD_ON_COLLISION = "onCollision";
const char *METHOD_ON_COMPLETE_LAST_CP = "onCompleteLastCheckpoint";

// Field Names
const char *FIELD_INSTANCE = "instance";

// Offsets
const uintptr_t OFFSET_ACTOR_COMPONENT_TO_ACTOR = 0x10;  // ActorComponent -> Actor
const uintptr_t OFFSET_MANAGER_TO_SCENE = 0x10;          // GameSceneManager -> GameScene
const uintptr_t OFFSET_SCENE_TO_LEVEL_LOGIC = 0x60;      // NormalLevelScene -> ActorNormalLevelLogic
const uintptr_t OFFSET_LEVEL_LOGIC_TO_CHECKPOINT = 0x78; // ActorNormalLevelLogic -> LogicCheckPointCheck

// Typedefs
typedef void (*UpdateGame_t)(void *instance);
typedef void (*AddBpAndBank_t)(void *instance, uint32_t amount);
typedef void (*SaveGame_t)(void *instance);
typedef void (*LifeComponent_damage_t)(void *instance, float amount);
typedef void (*HeroCollision_t)(void *instance);
typedef void (*OnCompleteLastCheckpoint_t)(void *instance);

// Global State
struct BeatPointsCommand
{
    bool active = false;
    uint32_t amount = 0;
};

BeatPointsCommand g_BeatPointsCommand;
bool g_GodModeEnabled = false;
bool g_FinishLevelRequested = false;
bool g_IsInitialized = false;
void *g_GameSceneManager = nullptr;

// Hooks
HOOK *update_hook = nullptr;
HOOK *damage_hook = nullptr;
HOOK *collision_hook = nullptr;

// =============================================================
void HandleAddBeatPoints()
{
    auto assembly = IL2CPP::Assembly(ASSEMBLY_GAME);
    if (!assembly)
        return;
    auto ns = assembly->Namespace(NAMESPACE_GLOBAL);

    // 1. Add Points
    auto metaGameClass = ns->Class(CLASS_METAGAME);
    if (!metaGameClass)
        return;

    auto metaInstanceField = metaGameClass->Field(FIELD_INSTANCE);
    if (!metaInstanceField)
        return;

    OBJECT *metaGameInstance = metaInstanceField->GetStaticObject();
    if (!metaGameInstance)
        return;

    auto addMethod = metaGameClass->Method(METHOD_ADD_BP_AND_BANK, 1);
    if (!addMethod)
        return;

    AddBpAndBank_t AddBpFunc = addMethod->FindFunction<AddBpAndBank_t>();
    if (AddBpFunc)
    {
        AddBpFunc(metaGameInstance, g_BeatPointsCommand.amount);
        std::cout << "[+] Added " << g_BeatPointsCommand.amount << " BP.\n";
    }

    // 2. Force Save
    auto saveClass = ns->Class(CLASS_SAVE_MANAGER);
    if (saveClass)
    {
        auto saveInstanceField = saveClass->Field(FIELD_INSTANCE);
        if (saveInstanceField)
        {
            OBJECT *saveManagerInstance = saveInstanceField->GetStaticObject();
            if (saveManagerInstance)
            {
                auto saveMethod = saveClass->Method(METHOD_SAVE, 0);
                if (saveMethod)
                {
                    saveMethod->FindFunction<SaveGame_t>()(saveManagerInstance);
                    std::cout << "[+] Game Saved.\n";
                }
            }
        }
    }
}

// =============================================================
void HandleFinishLevel()
{
    if (!g_GameSceneManager)
        return;
    auto assembly = IL2CPP::Assembly(ASSEMBLY_GAME);
    if (!assembly)
        return;
    auto ns = assembly->Namespace(NAMESPACE_GLOBAL);

    void *gameScenePtr = *(void **)((uintptr_t)g_GameSceneManager + OFFSET_MANAGER_TO_SCENE);
    if (!gameScenePtr)
        return;

    void *levelLogicPtr = *(void **)((uintptr_t)gameScenePtr + OFFSET_SCENE_TO_LEVEL_LOGIC);
    if (!levelLogicPtr)
        return;

    void *checkpointLogicPtr = *(void **)((uintptr_t)levelLogicPtr + OFFSET_LEVEL_LOGIC_TO_CHECKPOINT);
    if (checkpointLogicPtr)
    {
        auto cpClass = ns->Class(CLASS_LOGIC_CHECKPOINT);
        auto cpMethod = cpClass->Method(METHOD_ON_COMPLETE_LAST_CP, 0);
        if (cpMethod)
        {
            auto CPFunc = cpMethod->FindFunction<OnCompleteLastCheckpoint_t>();
            if (CPFunc)
            {
                CPFunc(checkpointLogicPtr);
                std::cout << "[+] Triggered Last Checkpoint Event.\n";
            }
        }
    }
}

// =============================================================
bool IsPlayerActor(void *actorPtr)
{
    if (!actorPtr)
        return false;

    OBJECT *actorObj = (OBJECT *)actorPtr;

    if (!actorObj)
        return false;

    CLASS *klass = actorObj->Class();
    if (!klass)
        return false;

    const char *className = klass->Name();
    if (!className)
        return false;

    return (strcmp(className, "Hero") == 0);
}

void Detour_LifeComponent_Damage(void *instance, float amount)
{
    if (g_GodModeEnabled && instance != nullptr)
    {
        void *actorPtr = *(void **)((uintptr_t)instance + OFFSET_ACTOR_COMPONENT_TO_ACTOR);

        if (IsPlayerActor(actorPtr))
        {
            // Prevent damage to player
            return;
        }
    }

    if (damage_hook && damage_hook->get_original<LifeComponent_damage_t>())
    {
        damage_hook->get_original<LifeComponent_damage_t>()(instance, amount);
    }
}

void Detour_HeroCollision_OnCollision(void *instance)
{
    if (g_GodModeEnabled)
    {
        // Prevent knockback/hit animation
        return;
    }

    if (collision_hook && collision_hook->get_original<HeroCollision_t>())
    {
        collision_hook->get_original<HeroCollision_t>()(instance);
    }
}

bool HookGodMode(NAMESPACE *ns, MEMORY &memory)
{
    if (!damage_hook)
    {
        auto lifeClass = ns->Class(CLASS_LIFECOMPONENT);
        if (lifeClass)
        {
            auto dmgMethod = lifeClass->Method(METHOD_DAMAGE, 1);
            if (dmgMethod)
            {
                damage_hook = dmgMethod->Hook<LifeComponent_damage_t>(memory, Detour_LifeComponent_Damage);
            }
        }
    }

    if (!collision_hook)
    {
        auto collisionClass = ns->Class(CLASS_HERO_COLLISION);
        if (collisionClass)
        {
            auto colMethod = collisionClass->Method(METHOD_ON_COLLISION, 0);
            if (colMethod)
            {
                collision_hook = colMethod->Hook<HeroCollision_t>(memory, Detour_HeroCollision_OnCollision);
            }
        }
    }

    return true;
}

// =============================================================
// Initialization
// =============================================================

void Detour_Update(void *instance)
{
    if (instance != nullptr)
    {
        g_GameSceneManager = instance;
    }

    if (update_hook && update_hook->get_original<UpdateGame_t>())
    {
        update_hook->get_original<UpdateGame_t>()(instance);
    }

    if (g_BeatPointsCommand.active)
    {
        g_BeatPointsCommand.active = false;
        HandleAddBeatPoints();
    }

    if (g_FinishLevelRequested)
    {
        g_FinishLevelRequested = false;
        HandleFinishLevel();
    }
}

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

    auto assembly = IL2CPP::Assembly(ASSEMBLY_GAME);
    if (!assembly)
    {
        std::cout << "[!] Failed to find assembly: " << ASSEMBLY_GAME << "\n";
        return false;
    }

    auto ns = assembly->Namespace(NAMESPACE_GLOBAL);
    if (!ns)
    {
        std::cout << "[!] Failed to find namespace: " << NAMESPACE_GLOBAL << "\n";
        return false;
    }

    // Update hook
    auto updateClass = ns->Class(CLASS_SCENE_MANAGER);
    if (updateClass)
    {
        auto updateMethod = updateClass->Method(METHOD_UPDATE, 0);
        if (updateMethod)
        {
            update_hook = updateMethod->Hook<UpdateGame_t>(memory, Detour_Update);
        }
        else
        {
            std::cout << "[!] Failed to find method for update hook: " << METHOD_UPDATE << "\n";
            return false;
        }
    }
    else
    {
        std::cout << "[!] Failed to find class for update hook: " << CLASS_SCENE_MANAGER << "\n";
        return false;
    }

    // Other hooks
    if (!HookGodMode(ns, memory))
    {
        std::cout << "[!] Failed to install God Mode hooks.\n";
        return false;
    }

    g_IsInitialized = true;
    std::cout << "[+] IL2CPP initialization complete!\n";
    return true;
}

// =============================================================
// Exported Functions
// =============================================================

extern "C" __declspec(dllexport) DWORD WINAPI GodMode(LPVOID lpParam)
{
    bool enabled = *(bool *)lpParam;
    if (!InitializeIL2CPP())
        return 0;

    g_GodModeEnabled = enabled;
    std::cout << "[+] God Mode set to: " << (enabled ? "ON" : "OFF") << "\n";
    return 1;
}

extern "C" __declspec(dllexport) DWORD WINAPI AddBeatPoints(LPVOID lpParam)
{
    int amount = *(int *)lpParam;
    if (!amount || !InitializeIL2CPP())
        return 0;

    g_BeatPointsCommand.amount = (uint32_t)amount;
    g_BeatPointsCommand.active = true;
    std::cout << "[+] Queueing BP Add: " << amount << "\n";
    return 1;
}

extern "C" __declspec(dllexport) DWORD WINAPI FinishLevel(LPVOID lpParam)
{
    if (!InitializeIL2CPP())
        return 0;

    g_FinishLevelRequested = true;
    std::cout << "[+] Queueing Instant Finish...\n";
    return 1;
}

// =============================================================
// DLL Main
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