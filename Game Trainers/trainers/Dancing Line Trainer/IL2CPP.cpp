#include <cstring>
#include <iostream>
#include <string>
#include "il2cpp/il2cpp.h"

using namespace IL2CPP;

// =============================================================
// Constants & Config
// =============================================================

const char *ASSEMBLY_GAME = "Assembly-CSharp";
const char *NAMESPACE_GLOBAL = "";
const char *NAMESPACE_CHARACTER = "DancingLine.Character";
const char *NAMESPACE_DANCING_LINE = "DancingLine";
const char *NAMESPACE_GAME_LOOP = "DancingLine.GameLoop";
const char *NAMESPACE_LEVEL = "DancingLine.Level";
const char *NAMESPACE_LEVEL_TRIGGERS = "DancingLine.Level.Triggers";
const char *NAMESPACE_MONETIZATION = "DancingLine.Monetyzation";
const char *NAMESPACE_MUSICIAL = "DancingLine.Musicial";
const char *NAMESPACE_SECURITY = "DancingLine.Security";
const char *NAMESPACE_CUSTOMIZATION = "DancingLine.Character.Customization";
const char *NAMESPACE_STORE = "DancingLine.UI.Store";

const char *CLASS_COINS_CONTROLLER = "CoinsController";
const char *CLASS_EINT = "eint";
const char *CLASS_GAME_CHARACTER = "GameCharacter";
const char *CLASS_HERO = "Hero";
const char *CLASS_HERO_CLASSIC = "HeroClassic";
const char *CLASS_KILL_HARACTER = "KillHaracter";
const char *CLASS_LEVEL_BORDER = "LevelBorder";
const char *CLASS_LEVEL_BASE = "LevelBase";
const char *CLASS_LEVELS_CONTROLLER = "LevelsController";
const char *CLASS_FAKE_GAME_CHARACTER = "FakeGameCharacter";
const char *CLASS_MUSICIAL_MANAGER = "MusicialManager";
const char *CLASS_USER_DATA_MANAGER = "UserDataManager";
const char *CLASS_UPDATE_LOOP_HANDLER = "UpdateLoopHandler";
const char *CLASS_LINE_CUSTOMIZATION_CONTROLLER = "LineCustomizationController";
const char *CLASS_LINE_CUSTOMIZATION_SETTINGS = "LineCustomizationSettings";
const char *CLASS_ORNAMENT_CONTROLLER = "OrnamentController";
const char *CLASS_STORE_PANEL_TAB_HATS = "StorePanelTabHats";
const char *CLASS_STORE_PANEL_TAB_SKIN = "StorePanelTabSkin";

const char *METHOD_COMPLETE_LEVEL = "CompleteLevel";
const char *METHOD_HERO_KILLED = "HeroKilled";
const char *METHOD_KILL = "Kill";
const char *METHOD_ON_TRIGGER_ENTER = "OnTriggerEnter";
const char *METHOD_ON_UPDATE = "OnUpdate";
const char *METHOD_ON_COLLISION_ENTER = "OnCollisionEnter";
const char *METHOD_REFRESH_COUNTER = "RefreshCounter";
const char *METHOD_REACHED_100 = "Reached100";
const char *METHOD_SAVE = "Save";
const char *METHOD_SAVE_DATA = "SaveData";
const char *METHOD_SET_DIAMONTS_COLLECTED = "set_DiamontsCollected";
const char *METHOD_SET_IS_PERFECT_REACHED_BEFORE = "set_IsPerfectReachedBefore";
const char *METHOD_SET_STARS_COLLECTED = "set_StarsCollected";
const char *METHOD_SHOW_LEVEL_AFTER_COMPLETED = "ShowLevelAfterCompleted";
const char *METHOD_STAR_REACHED = "StarReached";
const char *METHOD_START = "Start";
const char *METHOD_UPDATE = "Update";
const char *METHOD_GAME_CHARACTER_KILLED = "Killed";
const char *METHOD_GET_MUSICIAL_NOTE_AMOUNT = "get_MusicialNoteAmount";
const char *METHOD_GET_CURRENT_DIAMONTS_COLLECTED = "get_CurrentDiamontsCollected";
const char *METHOD_GET_CURRENT_RUN_STARS = "get_CurrentRunStars";
const char *METHOD_GET_CURRENT_RUN_STARS_COLLECTED = "get_CurrentRunStarsCollected";
const char *METHOD_GET_CURRENT_RUN_STARS_WHEN_DIE = "get_CurrentRunStarsCollectedWhenDie";
const char *METHOD_GET_IS_PERFECT_ON_CURRENT_RUN = "get_IsPerfectOnCurrentRun";
const char *METHOD_ADD_MUSIC_NOTE = "addMusicNote";
const char *METHOD_REDUCE_MUSIC_NOTE = "reduceMusicNote";
const char *METHOD_GET_SETTINGS = "get_Settings";
const char *METHOD_GET_IS_UNLOCKED = "get_IsUnlocked";
const char *METHOD_UNLOCK = "Unlock";
const char *METHOD_SET_HAVE_ACT_REWARD = "setHaveActReward";
const char *METHOD_REFRESH = "Refresh";

const char *COINS_DATA_KEY = "currentCoins";
const char *FIELD_INSTANCE = "instance";
const char *FIELD_COINS = "coins";
const char *FIELD_CURRENT_LEVEL = "<CurrentLevel>k__BackingField";
const char *FIELD_LIST_ITEMS = "_items";
const char *FIELD_LIST_SIZE = "_size";
const char *FIELD_GAME_CHARACTER_CURRENT_LEVEL = "currentLevel";
const char *FIELD_STARTING_FROM_CHECKPOINT = "startingFromCheckpoint";
const char *FIELD_STARTING_FROM_CHECKPOINT_COUNTER = "startingFromCheckpointCounter";
const char *FIELD_CURRENT_DIAMONTS_COLLECTED = "_currentDiamontsCollected";
const char *FIELD_STARTED = "<started>k__BackingField";

// =============================================================
// Types & State
// =============================================================

struct EInt
{
    int v;
    int k;
    int o;
};

struct IntCommand
{
    bool active = false;
    int value = 0;
};

typedef void (*VoidMethod_t)(void *instance, const MethodInfo *method);
typedef bool (*StaticBoolMethod_t)(const MethodInfo *method);
typedef void (*StaticVoidMethod_t)(const MethodInfo *method);
typedef int (*StaticGetInt_t)(const MethodInfo *method);
typedef void (*StaticSetInt_t)(int value, const MethodInfo *method);
typedef bool (*StaticReduceInt_t)(int value, const MethodInfo *method);
typedef ARRAY *(*GetArray_t)(void *instance, const MethodInfo *method);
typedef EInt (*IntToEInt_t)(int value, const MethodInfo *method);
typedef int (*GetInt_t)(void *instance, const MethodInfo *method);
typedef bool (*GetBool_t)(void *instance, const MethodInfo *method);
typedef int (*ListCount_t)(void *instance, const MethodInfo *method);
typedef bool (*ListContainsObject_t)(void *instance, void *item, const MethodInfo *method);
typedef void (*SaveInt_t)(STRING *name, int value, bool localOnly, const MethodInfo *method);
typedef void (*SetBool_t)(void *instance, bool value, const MethodInfo *method);
typedef void (*SetInt_t)(void *instance, int value, const MethodInfo *method);
typedef void (*HeroKill_t)(void *instance, void *obstacle, const MethodInfo *method);
typedef void (*TriggerEnter_t)(void *instance, void *collider, const MethodInfo *method);
typedef void (*CollisionEnter_t)(void *instance, void *collision, const MethodInfo *method);
typedef void (*LevelBaseHeroKilled_t)(void *instance, void *hero, const MethodInfo *method);
typedef void (*LevelBaseOnUpdate_t)(void *instance, float deltaTime, float smoothDeltaTime, float unscaledDeltaTime, const MethodInfo *method);

bool g_IsInitialized = false;
bool g_NoCollisionEnabled = false;
bool g_FinishLevelPerfectlyRequested = false;
bool g_ApplyingCommands = false;
bool g_WaitingForLevelStartLogged = false;
bool g_StarsUiRefreshQueued = false;
bool g_StoreTabsRefreshQueued = false;
bool g_ForcePerfectResult = false;

IntCommand g_StarsCommand;
IntCommand g_NotesCommand;
void *g_CurrentLevel = nullptr;
void *g_PerfectResultLevel = nullptr;
void *g_CoinsController = nullptr;
void *g_GameCharacter = nullptr;
CLASS *g_EIntClass = nullptr;
CLASS *g_UpdateLoopClass = nullptr;

HOOK *g_LevelUpdateHook = nullptr;
HOOK *g_UpdateLoopHook = nullptr;
HOOK *g_LevelCurrentRunStarsHook = nullptr;
HOOK *g_LevelCurrentRunStarsCollectedHook = nullptr;
HOOK *g_LevelCurrentRunStarsWhenDieHook = nullptr;
HOOK *g_LevelCurrentDiamondsHook = nullptr;
HOOK *g_LevelIsPerfectHook = nullptr;
HOOK *g_GameCharacterUpdateHook = nullptr;
HOOK *g_CoinsStartHook = nullptr;
HOOK *g_CoinsRefreshHook = nullptr;
HOOK *g_HeroKillHook = nullptr;
HOOK *g_HeroClassicKillHook = nullptr;
HOOK *g_HeroCollisionHook = nullptr;
HOOK *g_GameCharacterKilledHook = nullptr;
HOOK *g_LevelHeroKilledHook = nullptr;
HOOK *g_KillHaracterHook = nullptr;
HOOK *g_LevelBorderHook = nullptr;
HOOK *g_FakeGameCharacterKilledHook = nullptr;

// =============================================================
// Helpers
// =============================================================

METHOD *FindMethod(CLASS *klass, const char *methodName, int paramCount)
{
    if (!klass)
        return nullptr;

    return klass->Method(methodName, paramCount);
}

METHOD *FindMethodInHierarchy(CLASS *klass, const char *methodName, int paramCount)
{
    while (klass)
    {
        if (auto method = FindMethod(klass, methodName, paramCount))
            return method;

        klass = (CLASS *)IL2CPP::API::il2cpp_class_get_parent(klass);
    }

    return nullptr;
}

FIELD *FindFieldInHierarchy(CLASS *klass, const char *fieldName)
{
    while (klass)
    {
        if (auto field = klass->Field(fieldName))
            return field;

        klass = (CLASS *)IL2CPP::API::il2cpp_class_get_parent(klass);
    }

    return nullptr;
}

OBJECT *GetSingletonInstance(CLASS *klass)
{
    if (!klass)
        return nullptr;

    auto instanceField = FindFieldInHierarchy(klass, FIELD_INSTANCE);
    if (!instanceField)
        return nullptr;

    return instanceField->GetStaticObject();
}

void *GetCoinsController()
{
    if (g_CoinsController)
        return g_CoinsController;

    auto assembly = IL2CPP::Assembly(ASSEMBLY_GAME);
    auto coinsClass = assembly ? assembly->Namespace(NAMESPACE_MONETIZATION)->Class(CLASS_COINS_CONTROLLER) : nullptr;
    g_CoinsController = GetSingletonInstance(coinsClass);

    return g_CoinsController;
}

void *GetLevelsController()
{
    auto assembly = IL2CPP::Assembly(ASSEMBLY_GAME);
    auto levelsClass = assembly ? assembly->Namespace(NAMESPACE_DANCING_LINE)->Class(CLASS_LEVELS_CONTROLLER) : nullptr;
    return GetSingletonInstance(levelsClass);
}

void *GetCurrentLevel()
{
    if (g_CurrentLevel)
        return g_CurrentLevel;

    auto assembly = IL2CPP::Assembly(ASSEMBLY_GAME);
    if (!assembly)
        return nullptr;

    auto levelsClass = assembly->Namespace(NAMESPACE_DANCING_LINE)->Class(CLASS_LEVELS_CONTROLLER);
    OBJECT *levelsController = GetSingletonInstance(levelsClass);
    if (levelsController)
    {
        if (auto currentLevelField = FindFieldInHierarchy(levelsClass, FIELD_CURRENT_LEVEL))
            g_CurrentLevel = currentLevelField->GetObject(levelsController);
    }

    if (!g_CurrentLevel && g_GameCharacter)
    {
        OBJECT *character = (OBJECT *)g_GameCharacter;
        if (auto currentLevelField = FindFieldInHierarchy(character->Class(), FIELD_GAME_CHARACTER_CURRENT_LEVEL))
            g_CurrentLevel = currentLevelField->GetObject(character);
    }

    return g_CurrentLevel;
}

std::string ToUtf8(STRING *value)
{
    if (!value)
        return {};

    std::string result;
    result.reserve(value->length);
    for (int i = 0; i < value->length; ++i)
    {
        auto ch = value->chars[i];
        result.push_back(ch > 0 && ch < 0x80 ? static_cast<char>(ch) : '?');
    }

    return result;
}

bool TypeNameContains(const Il2CppType *type, const char *needle)
{
    if (!type || !needle)
        return false;

    const char *typeName = IL2CPP::API::il2cpp_type_get_name(type);
    return typeName && std::strstr(typeName, needle);
}

CLASS *FindClassByImageScan(const char *className, const char *namespaceNeedle)
{
    size_t assemblyCount = 0;
    auto assemblies = IL2CPP::API::il2cpp_domain_get_assemblies(IL2CPP::API::il2cpp_domain_get(), &assemblyCount);
    if (!assemblies)
        return nullptr;

    CLASS *fallback = nullptr;
    for (size_t assemblyIndex = 0; assemblyIndex < assemblyCount; ++assemblyIndex)
    {
        auto image = IL2CPP::API::il2cpp_assembly_get_image(assemblies[assemblyIndex]);
        if (!image)
            continue;

        size_t classCount = IL2CPP::API::il2cpp_image_get_class_count(image);
        for (size_t classIndex = 0; classIndex < classCount; ++classIndex)
        {
            auto klass = (CLASS *)IL2CPP::API::il2cpp_image_get_class(image, classIndex);
            if (!klass)
                continue;

            const char *name = IL2CPP::API::il2cpp_class_get_name(klass);
            if (!name || std::strcmp(name, className) != 0)
                continue;

            const char *namespaze = IL2CPP::API::il2cpp_class_get_namespace(klass);
            bool namespaceMatches = !namespaceNeedle || (namespaze && std::strstr(namespaze, namespaceNeedle));
            if (namespaceMatches)
                return klass;

            if (!fallback)
                fallback = klass;
        }
    }

    return fallback;
}

CLASS *GetEIntClass()
{
    if (g_EIntClass)
        return g_EIntClass;

    auto assembly = IL2CPP::Assembly(ASSEMBLY_GAME);
    if (assembly)
    {
        g_EIntClass = assembly->Namespace(NAMESPACE_SECURITY)->Class(CLASS_EINT);

        if (!g_EIntClass)
            g_EIntClass = assembly->Namespace(NAMESPACE_GLOBAL)->Class(CLASS_EINT);
    }

    if (!g_EIntClass)
        g_EIntClass = FindClassByImageScan(CLASS_EINT, "Security");

    return g_EIntClass;
}

CLASS *GetUpdateLoopClass()
{
    if (g_UpdateLoopClass)
        return g_UpdateLoopClass;

    auto assembly = IL2CPP::Assembly(ASSEMBLY_GAME);
    if (assembly)
        g_UpdateLoopClass = assembly->Namespace(NAMESPACE_GAME_LOOP)->Class(CLASS_UPDATE_LOOP_HANDLER);

    if (!g_UpdateLoopClass)
        g_UpdateLoopClass = FindClassByImageScan(CLASS_UPDATE_LOOP_HANDLER, NAMESPACE_GAME_LOOP);

    return g_UpdateLoopClass;
}

METHOD *FindMethodByParams(CLASS *klass, const char *methodName, const char *param0, const char *param1, const char *param2)
{
    if (!klass)
        return nullptr;

    void *iter = nullptr;
    const MethodInfo *method = nullptr;
    while ((method = IL2CPP::API::il2cpp_class_get_methods(klass, &iter)))
    {
        if (std::strcmp(IL2CPP::API::il2cpp_method_get_name(method), methodName) != 0)
            continue;

        if (IL2CPP::API::il2cpp_method_get_param_count(method) != 3)
            continue;

        if (!TypeNameContains(IL2CPP::API::il2cpp_method_get_param(method, 0), param0))
            continue;

        if (!TypeNameContains(IL2CPP::API::il2cpp_method_get_param(method, 1), param1))
            continue;

        if (!TypeNameContains(IL2CPP::API::il2cpp_method_get_param(method, 2), param2))
            continue;

        return (METHOD *)method;
    }

    return nullptr;
}

METHOD *FindMethodByParam(CLASS *klass, const char *methodName, const char *param0)
{
    if (!klass)
        return nullptr;

    void *iter = nullptr;
    const MethodInfo *method = nullptr;
    while ((method = IL2CPP::API::il2cpp_class_get_methods(klass, &iter)))
    {
        if (std::strcmp(IL2CPP::API::il2cpp_method_get_name(method), methodName) != 0)
            continue;

        if (IL2CPP::API::il2cpp_method_get_param_count(method) != 1)
            continue;

        if (TypeNameContains(IL2CPP::API::il2cpp_method_get_param(method, 0), param0))
            return (METHOD *)method;
    }

    return nullptr;
}

METHOD *FindIntToEIntMethod(CLASS *eintClass)
{
    if (!eintClass)
        return nullptr;

    void *iter = nullptr;
    const MethodInfo *method = nullptr;
    while ((method = IL2CPP::API::il2cpp_class_get_methods(eintClass, &iter)))
    {
        const char *name = IL2CPP::API::il2cpp_method_get_name(method);
        if (!name || std::strcmp(name, "op_Implicit") != 0)
            continue;

        if (IL2CPP::API::il2cpp_method_get_param_count(method) != 1)
            continue;

        if (TypeNameContains(IL2CPP::API::il2cpp_method_get_return_type(method), "eint") &&
            TypeNameContains(IL2CPP::API::il2cpp_method_get_param(method, 0), "Int32"))
        {
            return (METHOD *)method;
        }
    }

    return nullptr;
}

bool CallSaveData()
{
    auto assembly = IL2CPP::Assembly(ASSEMBLY_GAME);
    if (!assembly)
        return false;

    auto saveClass = assembly->Namespace(NAMESPACE_GLOBAL)->Class(CLASS_USER_DATA_MANAGER);
    auto saveMethod = FindMethod(saveClass, METHOD_SAVE_DATA, 0);
    if (!saveMethod)
        return false;

    auto SaveData = saveMethod->FindFunction<StaticBoolMethod_t>();
    return SaveData && SaveData(saveMethod);
}

bool SaveStarsToUserData(int value)
{
    auto assembly = IL2CPP::Assembly(ASSEMBLY_GAME);
    if (!assembly)
        return false;

    auto userDataClass = assembly->Namespace(NAMESPACE_GLOBAL)->Class(CLASS_USER_DATA_MANAGER);
    auto saveMethod = FindMethodByParams(userDataClass, METHOD_SAVE, "String", "Int32", "Boolean");
    if (!saveMethod)
        return false;

    SaveInt_t SaveInt = saveMethod->FindFunction<SaveInt_t>();
    if (!SaveInt)
        return false;

    SaveInt(IL2CPP::String(COINS_DATA_KEY), value, false, saveMethod);
    CallSaveData();
    return true;
}

bool RefreshStoreTab(const char *className)
{
    auto assembly = IL2CPP::Assembly(ASSEMBLY_GAME);
    auto tabClass = assembly ? assembly->Namespace(NAMESPACE_STORE)->Class(className) : nullptr;
    auto refreshMethod = FindMethod(tabClass, METHOD_REFRESH, 0);
    auto Refresh = refreshMethod ? refreshMethod->FindFunction<StaticVoidMethod_t>() : nullptr;
    if (!Refresh)
        return false;

    Refresh(refreshMethod);
    return true;
}

int UnlockAllLineSkins()
{
    auto assembly = IL2CPP::Assembly(ASSEMBLY_GAME);
    auto controllerClass = assembly ? assembly->Namespace(NAMESPACE_CUSTOMIZATION)->Class(CLASS_LINE_CUSTOMIZATION_CONTROLLER) : nullptr;
    auto controller = GetSingletonInstance(controllerClass);
    auto settingsMethod = FindMethod(controllerClass, METHOD_GET_SETTINGS, 0);
    auto GetSettings = settingsMethod ? settingsMethod->FindFunction<GetArray_t>() : nullptr;
    if (!controller || !GetSettings)
    {
        std::cout << "[!] LineCustomizationController is not available yet.\n";
        return -1;
    }

    auto settingsArray = GetSettings(controller, settingsMethod);
    if (!settingsArray)
        return 0;

    int unlockedCount = 0;
    for (size_t i = 0; i < settingsArray->MaxLength(); ++i)
    {
        auto settings = settingsArray->GetObject(i);
        if (!settings)
            continue;

        auto settingsClass = settings->Class();
        auto isUnlockedMethod = FindMethod(settingsClass, METHOD_GET_IS_UNLOCKED, 0);
        auto unlockMethod = FindMethod(settingsClass, METHOD_UNLOCK, 0);
        auto IsUnlocked = isUnlockedMethod ? isUnlockedMethod->FindFunction<GetBool_t>() : nullptr;
        auto Unlock = unlockMethod ? unlockMethod->FindFunction<VoidMethod_t>() : nullptr;
        if (!Unlock)
            continue;

        if (IsUnlocked && IsUnlocked(settings, isUnlockedMethod))
            continue;

        Unlock(settings, unlockMethod);
        ++unlockedCount;
    }

    return unlockedCount;
}

int UnlockAllLineHats()
{
    auto assembly = IL2CPP::Assembly(ASSEMBLY_GAME);
    auto ornamentClass = assembly ? assembly->Namespace(NAMESPACE_GLOBAL)->Class(CLASS_ORNAMENT_CONTROLLER) : nullptr;
    auto unlockMethod = FindMethod(ornamentClass, METHOD_SET_HAVE_ACT_REWARD, 1);
    auto UnlockHat = unlockMethod ? unlockMethod->FindFunction<StaticSetInt_t>() : nullptr;
    if (!UnlockHat)
    {
        std::cout << "[!] OrnamentController is not available yet.\n";
        return -1;
    }

    constexpr int hatTypes[] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    for (int hatType : hatTypes)
        UnlockHat(hatType, unlockMethod);

    return static_cast<int>(sizeof(hatTypes) / sizeof(hatTypes[0]));
}

bool ApplyUnlockAllSkinsAndDecorations()
{
    int skinCount = UnlockAllLineSkins();
    int hatCount = UnlockAllLineHats();
    if (skinCount < 0 || hatCount < 0)
        return false;

    CallSaveData();
    g_StoreTabsRefreshQueued = true;

    std::cout << "[+] Unlocked line skins/hats. Skins changed: " << skinCount
              << ", hats processed: " << hatCount << ".\n";
    return true;
}

bool BuildEInt(int value, EInt &out)
{
    auto assembly = IL2CPP::Assembly(ASSEMBLY_GAME);
    if (!assembly)
    {
        std::cout << "[!] BuildEInt failed: assembly unavailable.\n";
        return false;
    }

    auto eintClass = GetEIntClass();
    if (!eintClass)
    {
        std::cout << "[!] BuildEInt failed: eint class unavailable.\n";
        return false;
    }

    auto implicitMethod = FindIntToEIntMethod(eintClass);
    if (!implicitMethod)
    {
        std::cout << "[!] BuildEInt failed: int -> eint method not found.\n";
        return false;
    }

    IntToEInt_t IntToEInt = implicitMethod->FindFunction<IntToEInt_t>();
    if (!IntToEInt)
    {
        std::cout << "[!] BuildEInt failed: int -> eint function pointer missing.\n";
        return false;
    }

    out = IntToEInt(value, implicitMethod);
    return true;
}

bool SetCoinsFieldOnly(void *coinsController, CLASS *coinsClass, int value)
{
    if (!coinsController || !coinsClass)
        return false;

    EInt coinsValue{};
    if (!BuildEInt(value, coinsValue))
    {
        std::cout << "[!] Failed to build live stars value.\n";
        return false;
    }

    if (auto coinsField = FindFieldInHierarchy(coinsClass, FIELD_COINS))
    {
        coinsField->SetValue((OBJECT *)coinsController, &coinsValue);
        return true;
    }

    return false;
}

bool RefreshCoinsCounterThroughController()
{
    void *coinsController = GetCoinsController();
    auto assembly = IL2CPP::Assembly(ASSEMBLY_GAME);
    auto coinsClass = assembly ? assembly->Namespace(NAMESPACE_MONETIZATION)->Class(CLASS_COINS_CONTROLLER) : nullptr;
    if (!coinsController || !coinsClass)
        return false;

    if (auto refreshMethod = FindMethod(coinsClass, METHOD_REFRESH_COUNTER, 0))
    {
        if (g_CoinsRefreshHook && g_CoinsRefreshHook->get_original<VoidMethod_t>())
        {
            g_CoinsRefreshHook->get_original<VoidMethod_t>()(coinsController, refreshMethod);
            return true;
        }
        else if (auto RefreshCounter = refreshMethod->FindFunction<VoidMethod_t>())
        {
            RefreshCounter(coinsController, refreshMethod);
            return true;
        }
    }

    return false;
}

void ProcessQueuedStarsUiRefresh()
{
    if (!g_StarsUiRefreshQueued)
        return;

    g_StarsUiRefreshQueued = false;
    if (!RefreshCoinsCounterThroughController())
        g_StarsUiRefreshQueued = true;
}

void ProcessQueuedStoreTabsRefresh()
{
    if (!g_StoreTabsRefreshQueued)
        return;

    g_StoreTabsRefreshQueued = false;
    RefreshStoreTab(CLASS_STORE_PANEL_TAB_SKIN);
    RefreshStoreTab(CLASS_STORE_PANEL_TAB_HATS);
}

bool ApplyStarsToCoinsController(int value)
{
    if (value < 0)
        value = 0;

    void *coinsController = GetCoinsController();
    auto assembly = IL2CPP::Assembly(ASSEMBLY_GAME);
    auto coinsClass = assembly ? assembly->Namespace(NAMESPACE_MONETIZATION)->Class(CLASS_COINS_CONTROLLER) : nullptr;
    if (!coinsController || !coinsClass)
    {
        std::cout << "[!] CoinsController is not available yet.\n";
        return false;
    }

    if (!SetCoinsFieldOnly(coinsController, coinsClass, value))
        return false;

    SaveStarsToUserData(value);
    g_StarsUiRefreshQueued = true;

    return true;
}

bool ApplyNotesToMusicialManager(int value)
{
    if (value < 0)
        value = 0;

    auto assembly = IL2CPP::Assembly(ASSEMBLY_GAME);
    auto musicClass = assembly ? assembly->Namespace(NAMESPACE_MUSICIAL)->Class(CLASS_MUSICIAL_MANAGER) : nullptr;
    auto getNotesMethod = FindMethod(musicClass, METHOD_GET_MUSICIAL_NOTE_AMOUNT, 0);
    auto addNotesMethod = FindMethod(musicClass, METHOD_ADD_MUSIC_NOTE, 1);
    auto reduceNotesMethod = FindMethod(musicClass, METHOD_REDUCE_MUSIC_NOTE, 1);
    if (!getNotesMethod || !addNotesMethod || !reduceNotesMethod)
    {
        std::cout << "[!] MusicialManager note methods are not available yet.\n";
        return false;
    }

    auto GetNotes = getNotesMethod->FindFunction<StaticGetInt_t>();
    auto AddNotes = addNotesMethod->FindFunction<StaticSetInt_t>();
    auto ReduceNotes = reduceNotesMethod->FindFunction<StaticReduceInt_t>();
    if (!GetNotes || !AddNotes || !ReduceNotes)
        return false;

    int currentValue = GetNotes(getNotesMethod);
    int delta = value - currentValue;
    if (delta > 0)
        AddNotes(delta, addNotesMethod);
    else if (delta < 0 && !ReduceNotes(-delta, reduceNotesMethod))
        return false;

    CallSaveData();
    return true;
}

int GetListCount(void *list)
{
    if (!list)
        return 0;

    auto listObject = (OBJECT *)list;
    if (auto countMethod = FindMethod(listObject->Class(), "get_Count", 0))
    {
        if (auto GetCount = countMethod->FindFunction<ListCount_t>())
            return GetCount(list, countMethod);
    }

    if (auto sizeField = FindFieldInHierarchy(listObject->Class(), FIELD_LIST_SIZE))
        return sizeField->GetValue<int>(listObject);

    return 0;
}

void *GetListItem(void *list, int index)
{
    if (!list || index < 0)
        return nullptr;

    auto listObject = (OBJECT *)list;
    auto itemsField = FindFieldInHierarchy(listObject->Class(), FIELD_LIST_ITEMS);
    if (!itemsField)
        return nullptr;

    auto items = itemsField->GetArray(listObject);
    if (!items || index >= static_cast<int>(items->MaxLength()))
        return nullptr;

    return items->GetObject(index);
}

bool ListContainsObject(void *list, void *item)
{
    if (!list || !item)
        return false;

    auto method = FindMethod(((OBJECT *)list)->Class(), "Contains", 1);
    auto Contains = method ? method->FindFunction<ListContainsObject_t>() : nullptr;
    return Contains && Contains(list, item, method);
}

void ApplyQueuedCommands()
{
    if (g_ApplyingCommands)
        return;

    g_ApplyingCommands = true;

    if (g_StarsCommand.active && ApplyStarsToCoinsController(g_StarsCommand.value))
    {
        std::cout << "[+] Stars set to " << g_StarsCommand.value << ".\n";
        g_StarsCommand.active = false;
    }
    else if (g_StarsCommand.active)
    {
        std::cout << "[!] Stars apply remains queued.\n";
    }

    if (g_NotesCommand.active && ApplyNotesToMusicialManager(g_NotesCommand.value))
    {
        std::cout << "[+] Notes set to " << g_NotesCommand.value << ".\n";
        g_NotesCommand.active = false;
    }
    else if (g_NotesCommand.active)
    {
        std::cout << "[!] Notes apply remains queued.\n";
    }

    g_ApplyingCommands = false;
    ProcessQueuedStarsUiRefresh();
    ProcessQueuedStoreTabsRefresh();
}

void ApplyPerfectResultStats(CLASS *levelClass, void *levelInstance)
{
    bool enabled = true;
    bool disabled = false;
    int zero = 0;
    int crowns = 3;
    int diamonds = 10;

    if (auto checkpointField = levelClass->Field(FIELD_STARTING_FROM_CHECKPOINT))
        checkpointField->SetValue((OBJECT *)levelInstance, &disabled);

    if (auto checkpointCounterField = levelClass->Field(FIELD_STARTING_FROM_CHECKPOINT_COUNTER))
        checkpointCounterField->SetValue((OBJECT *)levelInstance, &zero);

    if (auto currentDiamondsField = levelClass->Field(FIELD_CURRENT_DIAMONTS_COLLECTED))
        currentDiamondsField->SetValue((OBJECT *)levelInstance, &diamonds);

    if (auto starReachedMethod = FindMethod(levelClass, METHOD_STAR_REACHED, 1))
    {
        auto StarReached = starReachedMethod->FindFunction<SetInt_t>();
        for (int i = 0; i < crowns; ++i)
            StarReached(levelInstance, i, starReachedMethod);
    }

    if (auto starsMethod = FindMethod(levelClass, METHOD_SET_STARS_COLLECTED, 1))
        starsMethod->FindFunction<SetInt_t>()(levelInstance, crowns, starsMethod);

    if (auto diamondsMethod = FindMethod(levelClass, METHOD_SET_DIAMONTS_COLLECTED, 1))
        diamondsMethod->FindFunction<SetInt_t>()(levelInstance, diamonds, diamondsMethod);

    if (auto perfectMethod = FindMethod(levelClass, METHOD_SET_IS_PERFECT_REACHED_BEFORE, 1))
        perfectMethod->FindFunction<SetBool_t>()(levelInstance, enabled, perfectMethod);
}

void ApplyFinishLevelPerfectly(void *levelInstance)
{
    levelInstance = levelInstance ? levelInstance : GetCurrentLevel();

    if (!levelInstance)
    {
        std::cout << "[!] LevelBase instance is not available yet. Finish command remains queued.\n";
        return;
    }

    auto assembly = IL2CPP::Assembly(ASSEMBLY_GAME);
    auto levelClass = assembly ? assembly->Namespace(NAMESPACE_LEVEL)->Class(CLASS_LEVEL_BASE) : nullptr;
    if (!levelClass)
        return;

    if (auto startedField = FindFieldInHierarchy(levelClass, FIELD_STARTED))
    {
        bool started = startedField->GetValue<bool>((OBJECT *)levelInstance);
        if (!started)
        {
            if (!g_WaitingForLevelStartLogged)
            {
                std::cout << "[*] Finish Level Perfectly is queued until the level starts moving.\n";
                g_WaitingForLevelStartLogged = true;
            }
            return;
        }
    }

    g_WaitingForLevelStartLogged = false;
    g_ForcePerfectResult = true;
    g_PerfectResultLevel = levelInstance;

    ApplyPerfectResultStats(levelClass, levelInstance);

    if (auto reachedMethod = FindMethod(levelClass, METHOD_REACHED_100, 0))
        reachedMethod->FindFunction<VoidMethod_t>()(levelInstance, reachedMethod);

    if (auto completeMethod = FindMethod(levelClass, METHOD_COMPLETE_LEVEL, 0))
        completeMethod->FindFunction<VoidMethod_t>()(levelInstance, completeMethod);

    if (auto showMethod = FindMethod(levelClass, METHOD_SHOW_LEVEL_AFTER_COMPLETED, 0))
        showMethod->FindFunction<VoidMethod_t>()(levelInstance, showMethod);

    ApplyPerfectResultStats(levelClass, levelInstance);
    CallSaveData();

    g_FinishLevelPerfectlyRequested = false;
    std::cout << "[+] Finish Level Perfectly applied.\n";
}

// =============================================================
// Hooks
// =============================================================

void Detour_LevelBase_OnUpdate(void *instance, float deltaTime, float smoothDeltaTime, float unscaledDeltaTime, const MethodInfo *method)
{
    if (instance)
        g_CurrentLevel = instance;

    if (g_LevelUpdateHook && g_LevelUpdateHook->get_original<LevelBaseOnUpdate_t>())
        g_LevelUpdateHook->get_original<LevelBaseOnUpdate_t>()(instance, deltaTime, smoothDeltaTime, unscaledDeltaTime, method);

    ApplyQueuedCommands();

    if (g_FinishLevelPerfectlyRequested)
        ApplyFinishLevelPerfectly(instance);
}

void Detour_UpdateLoop_Update(void *instance, const MethodInfo *method)
{
    if (g_UpdateLoopHook && g_UpdateLoopHook->get_original<VoidMethod_t>())
        g_UpdateLoopHook->get_original<VoidMethod_t>()(instance, method);

    ApplyQueuedCommands();
}

void Detour_GameCharacter_OnUpdate(void *instance, float deltaTime, float smoothDeltaTime, float unscaledDeltaTime, const MethodInfo *method)
{
    if (instance)
    {
        g_GameCharacter = instance;
        OBJECT *character = (OBJECT *)instance;
        if (auto currentLevelField = FindFieldInHierarchy(character->Class(), FIELD_GAME_CHARACTER_CURRENT_LEVEL))
            g_CurrentLevel = currentLevelField->GetObject(character);
    }

    if (g_GameCharacterUpdateHook && g_GameCharacterUpdateHook->get_original<LevelBaseOnUpdate_t>())
        g_GameCharacterUpdateHook->get_original<LevelBaseOnUpdate_t>()(instance, deltaTime, smoothDeltaTime, unscaledDeltaTime, method);

    ApplyQueuedCommands();

    if (g_FinishLevelPerfectlyRequested)
        ApplyFinishLevelPerfectly(GetCurrentLevel());
}

void Detour_CoinsController_Start(void *instance, const MethodInfo *method)
{
    if (instance)
        g_CoinsController = instance;

    if (g_CoinsStartHook && g_CoinsStartHook->get_original<VoidMethod_t>())
        g_CoinsStartHook->get_original<VoidMethod_t>()(instance, method);

    ApplyQueuedCommands();
}

void Detour_CoinsController_RefreshCounter(void *instance, const MethodInfo *method)
{
    if (instance)
        g_CoinsController = instance;

    if (g_CoinsRefreshHook && g_CoinsRefreshHook->get_original<VoidMethod_t>())
        g_CoinsRefreshHook->get_original<VoidMethod_t>()(instance, method);

    ApplyQueuedCommands();
}

int Detour_LevelBase_GetCurrentRunStars(void *instance, const MethodInfo *method)
{
    if (g_ForcePerfectResult && (!g_PerfectResultLevel || g_PerfectResultLevel == instance))
        return 3;

    if (g_LevelCurrentRunStarsHook && g_LevelCurrentRunStarsHook->get_original<GetInt_t>())
        return g_LevelCurrentRunStarsHook->get_original<GetInt_t>()(instance, method);

    return 0;
}

int Detour_LevelBase_GetCurrentRunStarsCollected(void *instance, const MethodInfo *method)
{
    if (g_ForcePerfectResult && (!g_PerfectResultLevel || g_PerfectResultLevel == instance))
        return 3;

    if (g_LevelCurrentRunStarsCollectedHook && g_LevelCurrentRunStarsCollectedHook->get_original<GetInt_t>())
        return g_LevelCurrentRunStarsCollectedHook->get_original<GetInt_t>()(instance, method);

    return 0;
}

int Detour_LevelBase_GetCurrentRunStarsWhenDie(void *instance, const MethodInfo *method)
{
    if (g_ForcePerfectResult && (!g_PerfectResultLevel || g_PerfectResultLevel == instance))
        return 3;

    if (g_LevelCurrentRunStarsWhenDieHook && g_LevelCurrentRunStarsWhenDieHook->get_original<GetInt_t>())
        return g_LevelCurrentRunStarsWhenDieHook->get_original<GetInt_t>()(instance, method);

    return 0;
}

int Detour_LevelBase_GetCurrentDiamonds(void *instance, const MethodInfo *method)
{
    if (g_ForcePerfectResult && (!g_PerfectResultLevel || g_PerfectResultLevel == instance))
        return 10;

    if (g_LevelCurrentDiamondsHook && g_LevelCurrentDiamondsHook->get_original<GetInt_t>())
        return g_LevelCurrentDiamondsHook->get_original<GetInt_t>()(instance, method);

    return 0;
}

bool Detour_LevelBase_GetIsPerfectOnCurrentRun(void *instance, const MethodInfo *method)
{
    if (g_ForcePerfectResult && (!g_PerfectResultLevel || g_PerfectResultLevel == instance))
        return true;

    if (g_LevelIsPerfectHook && g_LevelIsPerfectHook->get_original<GetBool_t>())
        return g_LevelIsPerfectHook->get_original<GetBool_t>()(instance, method);

    return false;
}

void Detour_Hero_Kill(void *instance, void *obstacle, const MethodInfo *method)
{
    if (g_FinishLevelPerfectlyRequested)
    {
        ApplyFinishLevelPerfectly(GetCurrentLevel());
        return;
    }

    if (g_NoCollisionEnabled)
        return;

    if (g_HeroKillHook && g_HeroKillHook->get_original<HeroKill_t>())
        g_HeroKillHook->get_original<HeroKill_t>()(instance, obstacle, method);
}

void Detour_HeroClassic_Kill(void *instance, void *obstacle, const MethodInfo *method)
{
    if (g_FinishLevelPerfectlyRequested)
    {
        ApplyFinishLevelPerfectly(GetCurrentLevel());
        return;
    }

    if (g_NoCollisionEnabled)
        return;

    if (g_HeroClassicKillHook && g_HeroClassicKillHook->get_original<HeroKill_t>())
        g_HeroClassicKillHook->get_original<HeroKill_t>()(instance, obstacle, method);
}

void Detour_Hero_OnCollisionEnter(void *instance, void *collision, const MethodInfo *method)
{
    if (g_FinishLevelPerfectlyRequested)
    {
        ApplyFinishLevelPerfectly(GetCurrentLevel());
        return;
    }

    if (g_NoCollisionEnabled)
        return;

    if (g_HeroCollisionHook && g_HeroCollisionHook->get_original<CollisionEnter_t>())
        g_HeroCollisionHook->get_original<CollisionEnter_t>()(instance, collision, method);
}

void Detour_GameCharacter_Killed(void *instance, const MethodInfo *method)
{
    if (g_FinishLevelPerfectlyRequested)
    {
        ApplyFinishLevelPerfectly(GetCurrentLevel());
        return;
    }

    if (g_NoCollisionEnabled)
        return;

    if (g_GameCharacterKilledHook && g_GameCharacterKilledHook->get_original<VoidMethod_t>())
        g_GameCharacterKilledHook->get_original<VoidMethod_t>()(instance, method);
}

void Detour_LevelBase_HeroKilled(void *instance, void *hero, const MethodInfo *method)
{
    if (g_FinishLevelPerfectlyRequested)
    {
        ApplyFinishLevelPerfectly(instance);
        return;
    }

    if (g_NoCollisionEnabled)
        return;

    if (g_LevelHeroKilledHook && g_LevelHeroKilledHook->get_original<LevelBaseHeroKilled_t>())
        g_LevelHeroKilledHook->get_original<LevelBaseHeroKilled_t>()(instance, hero, method);
}

void Detour_KillHaracter_Kill(void *instance, const MethodInfo *method)
{
    if (g_FinishLevelPerfectlyRequested)
    {
        ApplyFinishLevelPerfectly(GetCurrentLevel());
        return;
    }

    if (g_NoCollisionEnabled)
        return;

    if (g_KillHaracterHook && g_KillHaracterHook->get_original<VoidMethod_t>())
        g_KillHaracterHook->get_original<VoidMethod_t>()(instance, method);
}

void Detour_LevelBorder_OnTriggerEnter(void *instance, void *collider, const MethodInfo *method)
{
    if (g_FinishLevelPerfectlyRequested)
    {
        ApplyFinishLevelPerfectly(GetCurrentLevel());
        return;
    }

    if (g_NoCollisionEnabled)
        return;

    if (g_LevelBorderHook && g_LevelBorderHook->get_original<TriggerEnter_t>())
        g_LevelBorderHook->get_original<TriggerEnter_t>()(instance, collider, method);
}

void Detour_FakeGameCharacter_Killed(void *instance, const MethodInfo *method)
{
    if (g_FinishLevelPerfectlyRequested)
    {
        ApplyFinishLevelPerfectly(GetCurrentLevel());
        return;
    }

    if (g_NoCollisionEnabled)
        return;

    if (g_FakeGameCharacterKilledHook && g_FakeGameCharacterKilledHook->get_original<VoidMethod_t>())
        g_FakeGameCharacterKilledHook->get_original<VoidMethod_t>()(instance, method);
}

bool HookLevel(MEMORY &memory, ASSEMBLY *assembly)
{
    auto levelClass = assembly->Namespace(NAMESPACE_LEVEL)->Class(CLASS_LEVEL_BASE);
    if (!levelClass)
        return false;

    if (auto updateMethod = FindMethod(levelClass, METHOD_ON_UPDATE, 3))
        g_LevelUpdateHook = updateMethod->Hook<LevelBaseOnUpdate_t>(memory, Detour_LevelBase_OnUpdate);
    else
        return false;

    if (auto heroKilledMethod = FindMethod(levelClass, METHOD_HERO_KILLED, 1))
        g_LevelHeroKilledHook = heroKilledMethod->Hook<LevelBaseHeroKilled_t>(memory, Detour_LevelBase_HeroKilled);

    if (auto currentRunStarsMethod = FindMethod(levelClass, METHOD_GET_CURRENT_RUN_STARS, 0))
        g_LevelCurrentRunStarsHook = currentRunStarsMethod->Hook<GetInt_t>(memory, Detour_LevelBase_GetCurrentRunStars);

    if (auto currentRunStarsCollectedMethod = FindMethod(levelClass, METHOD_GET_CURRENT_RUN_STARS_COLLECTED, 0))
        g_LevelCurrentRunStarsCollectedHook = currentRunStarsCollectedMethod->Hook<GetInt_t>(memory, Detour_LevelBase_GetCurrentRunStarsCollected);

    if (auto currentRunStarsWhenDieMethod = FindMethod(levelClass, METHOD_GET_CURRENT_RUN_STARS_WHEN_DIE, 0))
        g_LevelCurrentRunStarsWhenDieHook = currentRunStarsWhenDieMethod->Hook<GetInt_t>(memory, Detour_LevelBase_GetCurrentRunStarsWhenDie);

    if (auto currentDiamondsMethod = FindMethod(levelClass, METHOD_GET_CURRENT_DIAMONTS_COLLECTED, 0))
        g_LevelCurrentDiamondsHook = currentDiamondsMethod->Hook<GetInt_t>(memory, Detour_LevelBase_GetCurrentDiamonds);

    if (auto isPerfectMethod = FindMethod(levelClass, METHOD_GET_IS_PERFECT_ON_CURRENT_RUN, 0))
        g_LevelIsPerfectHook = isPerfectMethod->Hook<GetBool_t>(memory, Detour_LevelBase_GetIsPerfectOnCurrentRun);

    return true;
}

bool HookGameCharacterUpdate(MEMORY &memory, ASSEMBLY *assembly)
{
    auto characterNs = assembly->Namespace(NAMESPACE_CHARACTER);
    auto gameCharacterClass = characterNs ? characterNs->Class(CLASS_GAME_CHARACTER) : nullptr;
    auto updateMethod = FindMethod(gameCharacterClass, METHOD_ON_UPDATE, 3);
    if (!updateMethod)
        return false;

    g_GameCharacterUpdateHook = updateMethod->Hook<LevelBaseOnUpdate_t>(memory, Detour_GameCharacter_OnUpdate);
    return g_GameCharacterUpdateHook != nullptr;
}

bool HookUpdateLoop(MEMORY &memory, ASSEMBLY *assembly)
{
    auto loopClass = GetUpdateLoopClass();
    auto updateMethod = FindMethod(loopClass, METHOD_UPDATE, 0);
    if (!updateMethod)
        return false;

    g_UpdateLoopHook = updateMethod->Hook<VoidMethod_t>(memory, Detour_UpdateLoop_Update);
    return g_UpdateLoopHook != nullptr;
}

bool HookCurrency(MEMORY &memory, ASSEMBLY *assembly)
{
    auto coinsClass = assembly->Namespace(NAMESPACE_MONETIZATION)->Class(CLASS_COINS_CONTROLLER);
    if (!coinsClass)
        return false;

    if (auto startMethod = FindMethod(coinsClass, METHOD_START, 0))
        g_CoinsStartHook = startMethod->Hook<VoidMethod_t>(memory, Detour_CoinsController_Start);

    if (auto refreshMethod = FindMethod(coinsClass, METHOD_REFRESH_COUNTER, 0))
        g_CoinsRefreshHook = refreshMethod->Hook<VoidMethod_t>(memory, Detour_CoinsController_RefreshCounter);

    return g_CoinsStartHook || g_CoinsRefreshHook;
}

bool HookNoCollision(MEMORY &memory, ASSEMBLY *assembly)
{
    auto characterNs = assembly->Namespace(NAMESPACE_CHARACTER);
    if (!characterNs)
        return false;

    auto heroClass = characterNs->Class(CLASS_HERO);
    if (auto killMethod = FindMethod(heroClass, METHOD_KILL, 1))
        g_HeroKillHook = killMethod->Hook<HeroKill_t>(memory, Detour_Hero_Kill);

    if (auto collisionMethod = FindMethod(heroClass, METHOD_ON_COLLISION_ENTER, 1))
        g_HeroCollisionHook = collisionMethod->Hook<CollisionEnter_t>(memory, Detour_Hero_OnCollisionEnter);

    auto heroClassicClass = characterNs->Class(CLASS_HERO_CLASSIC);
    if (auto killMethod = FindMethod(heroClassicClass, METHOD_KILL, 1))
        g_HeroClassicKillHook = killMethod->Hook<HeroKill_t>(memory, Detour_HeroClassic_Kill);

    auto gameCharacterClass = characterNs->Class(CLASS_GAME_CHARACTER);
    if (auto killedMethod = FindMethod(gameCharacterClass, METHOD_GAME_CHARACTER_KILLED, 0))
        g_GameCharacterKilledHook = killedMethod->Hook<VoidMethod_t>(memory, Detour_GameCharacter_Killed);

    auto levelNs = assembly->Namespace(NAMESPACE_LEVEL_TRIGGERS);
    auto borderClass = levelNs ? levelNs->Class(CLASS_LEVEL_BORDER) : nullptr;
    if (auto borderMethod = FindMethod(borderClass, METHOD_ON_TRIGGER_ENTER, 1))
        g_LevelBorderHook = borderMethod->Hook<TriggerEnter_t>(memory, Detour_LevelBorder_OnTriggerEnter);

    auto dancingLineNs = assembly->Namespace(NAMESPACE_DANCING_LINE);
    auto killHaracterClass = dancingLineNs ? dancingLineNs->Class(CLASS_KILL_HARACTER) : nullptr;
    if (auto killHaracterMethod = FindMethod(killHaracterClass, METHOD_KILL, 0))
        g_KillHaracterHook = killHaracterMethod->Hook<VoidMethod_t>(memory, Detour_KillHaracter_Kill);

    auto fakeNs = assembly->Namespace("Levels.Level_TheLine");
    auto fakeCharacterClass = fakeNs ? fakeNs->Class(CLASS_FAKE_GAME_CHARACTER) : nullptr;
    if (auto fakeKilledMethod = FindMethod(fakeCharacterClass, METHOD_GAME_CHARACTER_KILLED, 0))
        g_FakeGameCharacterKilledHook = fakeKilledMethod->Hook<VoidMethod_t>(memory, Detour_FakeGameCharacter_Killed);

    return g_HeroKillHook || g_HeroClassicKillHook || g_HeroCollisionHook || g_GameCharacterKilledHook || g_LevelBorderHook || g_KillHaracterHook || g_FakeGameCharacterKilledHook;
}

// =============================================================
// Initialization
// =============================================================

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

    if (!HookLevel(memory, assembly))
    {
        std::cout << "[!] Failed to install LevelBase hooks.\n";
        return false;
    }

    if (!HookGameCharacterUpdate(memory, assembly))
        std::cout << "[!] GameCharacter update hook was not installed.\n";

    if (!HookUpdateLoop(memory, assembly))
        std::cout << "[!] UpdateLoopHandler hook was not installed. Stars UI refresh may wait for another game update hook.\n";

    if (!HookCurrency(memory, assembly))
        std::cout << "[!] CoinsController refresh hooks were not installed. Set Stars will still save directly.\n";

    if (!HookNoCollision(memory, assembly))
        std::cout << "[!] No Collision hooks were not installed.\n";

    g_IsInitialized = true;
    std::cout << "[+] Dancing Line IL2CPP initialization complete.\n";
    return true;
}

// =============================================================
// Exported Functions
// =============================================================

extern "C" __declspec(dllexport) DWORD WINAPI SetStars(LPVOID lpParam)
{
    int value = *(int *)lpParam;
    if (!InitializeIL2CPP())
        return 0;

    g_StarsCommand.value = value;
    std::cout << "[+] Set Stars requested: " << value << "\n";

    if (ApplyStarsToCoinsController(value))
    {
        g_StarsCommand.active = false;
        std::cout << "[+] Stars set to " << value << ".\n";
    }
    else
    {
        g_StarsCommand.active = true;
        std::cout << "[!] Stars apply queued for Unity update.\n";
    }

    return 1;
}

extern "C" __declspec(dllexport) DWORD WINAPI SetNotes(LPVOID lpParam)
{
    int value = *(int *)lpParam;
    if (!InitializeIL2CPP())
        return 0;

    g_NotesCommand.value = value;
    g_NotesCommand.active = true;
    std::cout << "[+] Set Notes queued: " << value << "\n";

    return 1;
}

extern "C" __declspec(dllexport) DWORD WINAPI FinishLevelPerfectly(LPVOID lpParam)
{
    if (!InitializeIL2CPP())
        return 0;

    g_FinishLevelPerfectlyRequested = true;
    std::cout << "[+] Finish Level Perfectly queued.\n";

    return 1;
}

extern "C" __declspec(dllexport) DWORD WINAPI EnableRemovedLevels(LPVOID lpParam)
{
    std::cout << "[!] Enable Removed Levels is temporarily disabled.\n";
    return 0;
}

extern "C" __declspec(dllexport) DWORD WINAPI TestRemovedLevelBundles(LPVOID lpParam)
{
    std::cout << "[!] Removed level bundle probe is temporarily disabled.\n";
    return 0;
}

extern "C" __declspec(dllexport) DWORD WINAPI UnlockAllSkinsAndDecorations(LPVOID lpParam)
{
    if (!InitializeIL2CPP())
        return 0;

    return ApplyUnlockAllSkinsAndDecorations() ? 1 : 0;
}

extern "C" __declspec(dllexport) DWORD WINAPI ToggleNoCollision(LPVOID lpParam)
{
    bool enabled = *(bool *)lpParam;
    if (!InitializeIL2CPP())
        return 0;

    g_NoCollisionEnabled = enabled;
    std::cout << "[+] No Collision set to: " << (enabled ? "ON" : "OFF") << "\n";
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
