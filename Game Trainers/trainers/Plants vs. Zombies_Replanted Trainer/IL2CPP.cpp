#pragma warning(disable : 4703)
#pragma warning(disable : 4996)

#include <iostream>
#include "il2cpp/il2cpp.h"

using namespace IL2CPP;

const char *GAME_ASSEMBLY = "Assembly-CSharp";
const char *GAME_NAMESPACE = "Reloaded.Gameplay";

// Classes
const char *BOARD_CLASS = "Board";
const char *ZENGARDEN_CLASS = "ZenGarden";
const char *USERPROFILE_CLASS = "UserProfile";

// Methods
const char *UPDATE_METHOD = "UpdateGame";
const char *GIVE_PLANT_METHOD = "AttemptNewPlantUserdata";
const char *PLACE_PLANT_METHOD = "PlacePottedPlant";
const char *SET_PLANT_AGE_METHOD = "set_PlantAge";
const char *GET_ACTIVE_PROFILE_METHOD = "get_ActiveUserProfile";

// Fields
const char *FACING_FIELD = "mFacing";

struct Command
{
    bool active = false;
    int plantID = 0;
    int direction = 0;
};

Command g_PlantCommand;

typedef bool (*AttemptNewPlantUserdata_t)(int plantId);
typedef void (*UpdateGame_t)(void *instance);
typedef void *(*GetActiveUserProfile_t)(void *userService);
typedef void (*SetPlantAge_t)(void *pottedPlant, int age);
typedef void *(*PlacePottedPlant_t)(void *zenGardenInstance, int pottedPlantIndex);

AttemptNewPlantUserdata_t AttemptNewPlantUserdata = nullptr;
PlacePottedPlant_t PlacePottedPlant = nullptr;
HOOK *update_hook = nullptr;
bool g_IsInitialized = false;

/** Enhances the last added plant with age and direction settings */
void SpawnAndEnhanceLastPlant(void *boardInstance, int facingDirection)
{
    if (!boardInstance)
        return;

    OBJECT *board = (OBJECT *)boardInstance;
    OBJECT *app = board->GetValue<OBJECT *>("mApp");
    if (!app)
        return;

    OBJECT *userService = app->GetValue<OBJECT *>("m_userService");
    OBJECT *zenGarden = app->GetValue<OBJECT *>("m_zenGarden");

    if (userService && zenGarden)
    {
        auto getProfileMethod = userService->Class()->Method(GET_ACTIVE_PROFILE_METHOD, 0);
        if (getProfileMethod)
        {
            auto GetActiveUserProfile = getProfileMethod->FindFunction<GetActiveUserProfile_t>();
            OBJECT *userProfile = (OBJECT *)GetActiveUserProfile(userService);

            if (userProfile)
            {
                int count = userProfile->GetValue<int>("mNumPottedPlants");
                ARRAY *plantsArray = userProfile->GetArray("mPottedPlant");

                if (plantsArray && count > 0)
                {
                    int newPlantIndex = count - 1;
                    OBJECT *newPlantData = plantsArray->GetObject(newPlantIndex);

                    if (newPlantData)
                    {
                        auto setAgeMethod = newPlantData->Class()->Method(SET_PLANT_AGE_METHOD, 1);
                        if (setAgeMethod)
                        {
                            auto SetPlantAge = setAgeMethod->FindFunction<SetPlantAge_t>();
                            SetPlantAge(newPlantData, 3);
                        }
                        newPlantData->SetValue(FACING_FIELD, &facingDirection);
                    }

                    if (PlacePottedPlant)
                        PlacePottedPlant(zenGarden, newPlantIndex);
                }
            }
        }
    }
}

/** Main game thread hook - processes command queue */
void Detour_UpdateGame(void *instance)
{
    if (update_hook && update_hook->get_original<UpdateGame_t>())
    {
        update_hook->get_original<UpdateGame_t>()(instance);
    }

    if (g_PlantCommand.active)
    {
        g_PlantCommand.active = false;

        if (AttemptNewPlantUserdata)
        {
            if (AttemptNewPlantUserdata(g_PlantCommand.plantID))
            {
                SpawnAndEnhanceLastPlant(instance, g_PlantCommand.direction);
            }
        }
    }
}

struct PlantArgs
{
    int plantID;
    int direction;
};

/** Initialize IL2CPP hooks and methods */
bool InitializeIL2CPP()
{
    if (g_IsInitialized)
        return true;

    std::cout << "[+] Initializing IL2CPP on first function call...\n";
    MEMORY memory;
    if (!IL2CPP::Initialize(memory))
    {
        std::cout << "[!] Failed to initialize IL2CPP.\n";
        return false;
    }
    std::cout << "[+] IL2CPP initialized successfully.\n";

    IL2CPP::Attach();
    std::cout << "[+] Thread attached to IL2CPP domain.\n";

    auto assembly = IL2CPP::Assembly(GAME_ASSEMBLY);
    if (!assembly)
    {
        std::cout << "[!] Failed to find assembly: " << GAME_ASSEMBLY << "\n";
        return false;
    }
    std::cout << "[+] Found assembly: " << GAME_ASSEMBLY << "\n";

    auto ns = assembly->Namespace(GAME_NAMESPACE);

    auto zenClass = ns->Class(ZENGARDEN_CLASS);
    if (zenClass)
    {
        std::cout << "[+] Found ZenGarden class.\n";
        auto method = zenClass->Method(GIVE_PLANT_METHOD, 1);
        if (method)
        {
            AttemptNewPlantUserdata = method->FindFunction<AttemptNewPlantUserdata_t>();
            std::cout << "[+] Found AttemptNewPlantUserdata method.\n";
        }

        auto placeMethod = zenClass->Method(PLACE_PLANT_METHOD, 1);
        if (placeMethod)
        {
            PlacePottedPlant = placeMethod->FindFunction<PlacePottedPlant_t>();
            std::cout << "[+] Found PlacePottedPlant method.\n";
        }
    }

    auto boardClass = ns->Class(BOARD_CLASS);
    if (boardClass)
    {
        std::cout << "[+] Found Board class.\n";
        auto updateMethod = boardClass->Method(UPDATE_METHOD, 0);
        if (updateMethod)
        {
            update_hook = updateMethod->Hook<UpdateGame_t>(memory, Detour_UpdateGame);
            std::cout << "[+] Hooked UpdateGame method.\n";
        }
    }

    g_IsInitialized = true;
    std::cout << "[+] IL2CPP initialization complete!\n";
    return true;
}

/** Verify all required hooks and methods are ready */
bool ValidateIL2CPP()
{
    if (!update_hook || !AttemptNewPlantUserdata)
    {
        std::cout << "[!] Hooks or functions not initialized properly.\n";
        return false;
    }
    return true;
}

/** Exported function called by trainer to queue a plant addition */
extern "C" __declspec(dllexport) DWORD WINAPI AddPlantToGarden(LPVOID lpParam)
{
    PlantArgs *args = (PlantArgs *)lpParam;
    if (!args || !InitializeIL2CPP() || !ValidateIL2CPP())
        return 0;

    g_PlantCommand.plantID = args->plantID;
    g_PlantCommand.direction = args->direction;
    g_PlantCommand.active = true;

    std::cout << "[+] Plant command queued: ID=" << args->plantID << " Direction=" << args->direction << "\n";
    return 1;
}

/** DLL initialization on load - minimal setup only */
DWORD WINAPI mainThread(HMODULE hModule)
{
#ifdef _DEBUG
    // Only allocate console in debug builds
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
        CreateThread(nullptr, NULL, (LPTHREAD_START_ROUTINE)mainThread, hModule, NULL, nullptr);
        break;
    }
    return TRUE;
}
