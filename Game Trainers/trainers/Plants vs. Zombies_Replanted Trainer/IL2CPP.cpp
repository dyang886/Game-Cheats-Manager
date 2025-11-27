#pragma warning(disable : 4703)
#pragma warning(disable : 4996)

#include <iostream>
#include <sstream>
#include "il2cpp/il2cpp.h"

using namespace IL2CPP;

const char *GAME_ASSEMBLY = "Assembly-CSharp";
const char *GAME_NAMESPACE = "Reloaded.Gameplay";

// Classes
const char *BOARD_CLASS = "Board";
const char *ZENGARDEN_CLASS = "ZenGarden";
const char *USERPROFILE_CLASS = "UserProfile";
const char *SEEDTYPE_ENUM = "SeedType";
const char *ZOMBIETYPE_ENUM = "ZombieType";

// Methods
const char *UPDATE_METHOD = "UpdateGame";
const char *GIVE_PLANT_METHOD = "AttemptNewPlantUserdata";
const char *PLACE_PLANT_METHOD = "PlacePottedPlant";
const char *SET_PLANT_AGE_METHOD = "set_PlantAge";
const char *GET_ACTIVE_PROFILE_METHOD = "get_ActiveUserProfile";
const char *NEW_PLANT_METHOD = "NewPlant";
const char *ADD_ZOMBIE_ROW_METHOD = "AddZombieInRow";
const char *GET_NUM_ROWS_METHOD = "GetNumRows";

// Fields
const char *FACING_FIELD = "mFacing";
const char *LEVEL_COMPLETE_FIELD = "mLevelComplete";

// Typedefs
typedef bool (*AttemptNewPlantUserdata_t)(int plantId);
typedef void (*UpdateGame_t)(void *instance);
typedef void *(*GetActiveUserProfile_t)(void *userService);
typedef void (*SetPlantAge_t)(void *pottedPlant, int age);
typedef void *(*PlacePottedPlant_t)(void *zenGardenInstance, int pottedPlantIndex);
typedef void *(*NewPlant_t)(void *instance, int x, int y, int seedType, int imitaterType);
typedef void *(*AddZombieInRow_t)(void *instance, int zombieType, int row, int fromWave, bool shakeBrush);
typedef int (*GetNumRows_t)(void *instance);

// Command Structures
struct ZenGardenPlantCommand
{
    bool active = false;
    int plantID = 0;
    int direction = 0;
};

struct PlantArgs
{
    int plantID;
    int direction;
};

struct SpawnZombieArgs
{
    int type;
    int row;
    bool isEveryRow;
};

struct ZombieCommand
{
    bool active = false;
    int type = 0;
    int row = 0;
    bool isEveryRow = false;
};

ZenGardenPlantCommand g_PlantCommand;
ZombieCommand g_ZombieCommand;
bool g_InstantWinCommand = false;
bool g_JalapenoCommand = false;

HOOK *update_hook = nullptr;
bool g_IsInitialized = false;

/** Enhances the last added plant with age and direction settings */
void HandlePlantSpawn(void *boardInstance)
{
    auto assembly = IL2CPP::Assembly(GAME_ASSEMBLY);
    auto ns = assembly->Namespace(GAME_NAMESPACE);
    auto zenClass = ns->Class(ZENGARDEN_CLASS);

    auto method = zenClass->Method(GIVE_PLANT_METHOD, 1);
    AttemptNewPlantUserdata_t AttemptNewPlantUserdata = method->FindFunction<AttemptNewPlantUserdata_t>();

    auto placeMethod = zenClass->Method(PLACE_PLANT_METHOD, 1);
    PlacePottedPlant_t PlacePottedPlant = placeMethod->FindFunction<PlacePottedPlant_t>();

    if (!AttemptNewPlantUserdata(g_PlantCommand.plantID) || !PlacePottedPlant)
        return;

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
                        newPlantData->SetValue(FACING_FIELD, &g_PlantCommand.direction);
                    }

                    if (PlacePottedPlant)
                        PlacePottedPlant(zenGarden, newPlantIndex);
                }
            }
        }
    }
}

/** Handle instant level completion */
void HandleInstantWin(void *boardInstance)
{
    OBJECT *board = (OBJECT *)boardInstance;
    if (!board)
        return;

    auto assembly = IL2CPP::Assembly(GAME_ASSEMBLY);
    if (assembly)
    {
        auto ns = assembly->Namespace(GAME_NAMESPACE);
        auto boardClass = ns->Class(BOARD_CLASS);
        if (boardClass)
        {
            auto completeField = boardClass->Field(LEVEL_COMPLETE_FIELD);
            if (completeField)
            {
                bool won = true;
                completeField->SetValue(board, &won);
                std::cout << "[+] Level Complete set to TRUE\n";
            }
            else
            {
                std::cout << "[!] Failed to find field: " << LEVEL_COMPLETE_FIELD << "\n";
            }
        }
    }
}

/** Handle zombie spawning */
void HandleZombieSpawn(void *boardInstance)
{
    if (!boardInstance)
        return;

    auto assembly = IL2CPP::Assembly(GAME_ASSEMBLY);
    if (assembly)
    {
        auto ns = assembly->Namespace(GAME_NAMESPACE);
        auto boardClass = ns->Class(BOARD_CLASS);

        // AddZombieInRow takes 4 arguments in C# (Type, Row, FromWave, ShakeBrush)
        auto addZombieMethod = boardClass->Method(ADD_ZOMBIE_ROW_METHOD, 4);

        if (addZombieMethod)
        {
            auto AddZombieInRow = addZombieMethod->FindFunction<AddZombieInRow_t>();
            if (AddZombieInRow)
            {
                int zombieType = g_ZombieCommand.type;
                int row = g_ZombieCommand.row;
                bool isEveryRow = g_ZombieCommand.isEveryRow;
                int fromWave = 0;
                bool shakeBrush = true;

                if (isEveryRow)
                {
                    auto getRowsMethod = boardClass->Method(GET_NUM_ROWS_METHOD, 0);
                    auto GetNumRows = getRowsMethod->FindFunction<GetNumRows_t>();
                    for (int r = 0; r < GetNumRows(boardInstance); r++)
                    {
                        AddZombieInRow(boardInstance, zombieType, r, fromWave, shakeBrush);
                        std::cout << "[+] Zombie Spawned: ID=" << zombieType << " Row=" << r << "\n";
                    }
                }
                else
                {
                    AddZombieInRow(boardInstance, zombieType, row, fromWave, shakeBrush);
                    std::cout << "[+] Zombie Spawned: ID=" << zombieType << " Row=" << row << "\n";
                }
            }
            else
            {
                std::cout << "[!] Failed to find function pointer for: " << ADD_ZOMBIE_ROW_METHOD << "\n";
            }
        }
        else
        {
            std::cout << "[!] Failed to find method: " << ADD_ZOMBIE_ROW_METHOD << "\n";
        }
    }
}

/** Handle full screen jalapeno planting */
void HandleFullScreenJalapeno(void *boardInstance)
{
    auto assembly = IL2CPP::Assembly(GAME_ASSEMBLY);
    if (assembly)
    {
        auto ns = assembly->Namespace(GAME_NAMESPACE);
        auto boardClass = ns->Class(BOARD_CLASS);
        auto newPlantMethod = boardClass->Method(NEW_PLANT_METHOD, 4);

        if (newPlantMethod)
        {
            auto NewPlant = newPlantMethod->FindFunction<NewPlant_t>();
            if (NewPlant)
            {
                int jalapenoType = 20;
                int imitaterType = -1;

                for (int x = 0; x < 9; x++)
                {
                    for (int y = 0; y < 6; y++)
                    {
                        NewPlant(boardInstance, x, y, jalapenoType, imitaterType);
                    }
                }
                std::cout << "[+] Full screen Jalapeno triggered.\n";
            }
            else
            {
                std::cout << "[!] Failed to find NewPlant function pointer.\n";
            }
        }
        else
        {
            std::cout << "[!] Failed to find method: " << NEW_PLANT_METHOD << "\n";
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
        HandlePlantSpawn(instance);
    }

    if (g_InstantWinCommand)
    {
        g_InstantWinCommand = false;
        HandleInstantWin(instance);
    }

    if (g_ZombieCommand.active)
    {
        g_ZombieCommand.active = false;
        HandleZombieSpawn(instance);
    }

    if (g_JalapenoCommand)
    {
        g_JalapenoCommand = false;
        HandleFullScreenJalapeno(instance);
    }
}

/** Initialize IL2CPP hooks and methods */
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

    auto assembly = IL2CPP::Assembly(GAME_ASSEMBLY);
    if (!assembly)
    {
        std::cout << "[!] Failed to find assembly: " << GAME_ASSEMBLY << "\n";
        return false;
    }

    auto ns = assembly->Namespace(GAME_NAMESPACE);
    auto boardClass = ns->Class(BOARD_CLASS);
    if (boardClass)
    {
        auto updateMethod = boardClass->Method(UPDATE_METHOD, 0);
        if (updateMethod)
            update_hook = updateMethod->Hook<UpdateGame_t>(memory, Detour_UpdateGame);
        else
        {
            std::cout << "[!] Failed to find method for update hook: " << UPDATE_METHOD << "\n";
            return false;
        }
    }
    else
    {
        std::cout << "[!] Failed to find class for update hook: " << BOARD_CLASS << "\n";
        return false;
    }

    g_IsInitialized = true;
    std::cout << "[+] IL2CPP initialization complete!\n";
    return true;
}

/** Send a string response back to the trainer through shared memory */
extern "C" __declspec(dllexport) void SendResponse(const char *message)
{
    if (!message)
        return;

    DWORD procId = GetCurrentProcessId();
    char mappingName[256];
    char eventName[256];

    snprintf(mappingName, sizeof(mappingName), "IL2CppResponseSharedMemory_%lu", procId);
    snprintf(eventName, sizeof(eventName), "IL2CppResponseEvent_%lu", procId);

    // Open the shared memory file mapping created by trainer
    HANDLE hMapFile = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, mappingName);
    if (!hMapFile)
    {
        std::cerr << "[!] Failed to open shared memory: " << mappingName << "\n";
        return;
    }

    // Map the file view
    LPVOID pBuf = MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, 1024 * 1024);
    if (!pBuf)
    {
        std::cerr << "[!] Failed to map view of file.\n";
        CloseHandle(hMapFile);
        return;
    }

    // Write the response: [length][data...]
    size_t msgLen = strlen(message);
    if (msgLen > 1024 * 1024 - sizeof(size_t))
    {
        std::cerr << "[!] Message too large for response buffer.\n";
        UnmapViewOfFile(pBuf);
        CloseHandle(hMapFile);
        return;
    }

    memcpy(pBuf, &msgLen, sizeof(size_t));
    memcpy(static_cast<char *>(pBuf) + sizeof(size_t), message, msgLen);

    // Open the event and signal it
    HANDLE hEvent = OpenEventA(EVENT_MODIFY_STATE, FALSE, eventName);
    if (hEvent)
    {
        SetEvent(hEvent);
        CloseHandle(hEvent);
        std::cout << "[+] Response sent: " << message << "\n";
    }
    else
    {
        std::cerr << "[!] Failed to open response event: " << eventName << "\n";
    }

    // Cleanup
    UnmapViewOfFile(pBuf);
    CloseHandle(hMapFile);
}

/** Test function that returns a random string response */
extern "C" __declspec(dllexport) void GetPlantList()
{
    if (!InitializeIL2CPP())
        return;

    std::stringstream ss;

    auto assembly = IL2CPP::Assembly(GAME_ASSEMBLY);
    auto ns = assembly->Namespace(GAME_NAMESPACE);
    auto enumClass = ns->Class(SEEDTYPE_ENUM);

    if (enumClass)
    {
        void *iter = nullptr;
        FieldInfo *field = nullptr;

        while ((field = IL2CPP::API::il2cpp_class_get_fields(enumClass, &iter)))
        {
            const char *fieldName = IL2CPP::API::il2cpp_field_get_name(field);

            // Skip internal value field
            if (strcmp(fieldName, "value__") == 0)
                continue;

            int32_t value = 0;
            IL2CPP::API::il2cpp_field_static_get_value(field, &value);

            // Plants are 0-52 (40+ are not plantable)
            if (value >= 0 && value <= 39)
            {
                ss << value << ">" << fieldName << "\n";
            }
        }
    }
    else
    {
        ss << "";
    }

    SendResponse(ss.str().c_str());
}

/** Exported: Returns a list of all Zombie IDs and Names */
extern "C" __declspec(dllexport) void GetZombieList()
{
    if (!InitializeIL2CPP())
        return;

    std::stringstream ss;

    auto assembly = IL2CPP::Assembly(GAME_ASSEMBLY);
    auto ns = assembly->Namespace(GAME_NAMESPACE);
    auto enumClass = ns->Class(ZOMBIETYPE_ENUM);

    if (enumClass)
    {
        void *iter = nullptr;
        FieldInfo *field = nullptr;

        while ((field = IL2CPP::API::il2cpp_class_get_fields(enumClass, &iter)))
        {
            const char *fieldName = IL2CPP::API::il2cpp_field_get_name(field);

            // Skip the internal value field generated by IL2CPP
            if (strcmp(fieldName, "value__") == 0)
                continue;

            int32_t value = 0;
            IL2CPP::API::il2cpp_field_static_get_value(field, &value);

            // Zombies are 0-35
            if (value >= 0 && value <= 35)
            {
                ss << value << ">" << fieldName << "\n";
            }
        }
    }
    else
    {
        ss << "";
    }

    SendResponse(ss.str().c_str());
}

/** Exported: Instantly complete the current level */
extern "C" __declspec(dllexport) DWORD WINAPI InstantCompleteLevel(LPVOID lpParam)
{
    if (!InitializeIL2CPP())
        return 0;

    g_InstantWinCommand = true;
    std::cout << "[+] Command queued: Instant Win\n";
    return 1;
}

/** Exported: Spawn a specific zombie on a specific row */
extern "C" __declspec(dllexport) DWORD WINAPI SpawnZombie(LPVOID lpParam)
{
    SpawnZombieArgs *args = (SpawnZombieArgs *)lpParam;
    if (!args || !InitializeIL2CPP())
        return 0;

    g_ZombieCommand.type = args->type;
    g_ZombieCommand.row = args->row;
    g_ZombieCommand.isEveryRow = args->isEveryRow;
    g_ZombieCommand.active = true;

    std::cout << "[+] Command queued: Spawn Zombie ID=" << args->type << " Row=" << args->row << "\n";
    return 1;
}

/** Exported: Plant Jalapenos on every tile */
extern "C" __declspec(dllexport) DWORD WINAPI FullScreenJalapeno(LPVOID lpParam)
{
    if (!InitializeIL2CPP())
        return 0;

    g_JalapenoCommand = true;
    std::cout << "[+] Command queued: Full Screen Jalapeno\n";
    return 1;
}

/** Exported function called by trainer to queue a plant addition */
extern "C" __declspec(dllexport) DWORD WINAPI AddPlantToGarden(LPVOID lpParam)
{
    PlantArgs *args = (PlantArgs *)lpParam;
    if (!args || !InitializeIL2CPP())
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
