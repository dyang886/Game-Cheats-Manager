// Dancing Line trainer (protected IL2CPP build).
//
// The game ships with a commercial protector (Virbox/SenseShield-class): the
// global-metadata is encrypted and decrypted on-demand, and the il2cpp API exports
// are renamed + control-flow-obfuscated. Metadata-touching API calls (class_from_name,
// image_get_class, ...) also return null when called from an injected (non-runtime)
// thread. So we do NOT use the il2cpp API at all.
//
// Instead we resolve everything by reading the live runtime structures directly
// (see mem_resolver.h): scan the heap for Il2CppClass structs anchored on the
// Assembly-CSharp image pointer, then read methods/fields straight from the structs.
// Class and method names are plaintext in the live structures.
//
// All game methods are CALLED from inside MinHook detours, which run on the game's
// own (runtime-attached) threads. The exported cheat functions run on injected
// remote threads, so they only set request flags; a per-frame hook applies them.

#include <windows.h>
#include <cstdio>
#include <cstdint>
#include <iostream>
#include "MinHook.h"
#include "mem_resolver.h"

// Logging: plain std::cout. In debug builds a console is allocated so output is visible.

// =============================================================
// Class / method names (plaintext in the live runtime)
// =============================================================

namespace N
{
    const char *MONET = "DancingLine.Monetyzation";
    const char *CHAR = "DancingLine.Character";
    const char *DL = "DancingLine";
    const char *GAMELOOP = "DancingLine.GameLoop";
    const char *LEVEL = "DancingLine.Level";
    const char *LEVEL_TRIG = "DancingLine.Level.Triggers";
    const char *MUSIC = "DancingLine.Musicial";
    const char *CUSTOM = "DancingLine.Character.Customization";
    const char *STORE = "DancingLine.UI.Store";
    const char *GLOBAL = "";
    const char *LEVEL_THELINE = "Levels.Level_TheLine";
}

// =============================================================
// Method signatures (il2cpp passes a hidden trailing const MethodInfo*)
// =============================================================

// ACTk "eint" (DancingLine.Security.eint, in Assembly-CSharp-firstpass). Oversized to 16
// bytes to safely cover the real struct; on Win x64 any non-1/2/4/8 size uses the same ABI
// (sret return, by-ref param), so the exact size only needs to be <= this.
struct EInt { int32_t a, b, c, d; };
typedef EInt (*IntToEInt_t)(int value, const void *method);              // eint.op_Implicit(int)
typedef void (*SetCoinsEInt_t)(void *inst, EInt value, const void *method); // CoinsController.set_Coins(eint)

typedef void (*VoidInst_t)(void *inst, const void *method);
typedef int (*GetIntInst_t)(void *inst, const void *method);
typedef bool (*GetBoolInst_t)(void *inst, const void *method);
typedef void (*IntInst_t)(void *inst, int v, const void *method);
typedef void (*IntBoolInst_t)(void *inst, int v, bool b, const void *method);
typedef void (*BoolInst_t)(void *inst, bool v, const void *method);
typedef void *(*GetPtrInst_t)(void *inst, const void *method);
// statics (no instance)
typedef void (*StaticVoid_t)(const void *method);
typedef bool (*StaticBool_t)(const void *method);
typedef int (*StaticGetInt_t)(const void *method);
typedef void (*StaticSetInt_t)(int v, const void *method);
// hooks
typedef void (*OnUpdate_t)(void *inst, float a, float b, float c, const void *method);
typedef void (*Kill_t)(void *inst, void *arg, const void *method);
typedef void (*HeroKilled_t)(void *inst, void *hero, const void *method);

// =============================================================
// Resolver helpers
// =============================================================

struct Method
{
    void *mi = nullptr;  // MethodInfo*
    void *ptr = nullptr; // methodPointer (mi[0])
    explicit operator bool() const { return ptr != nullptr; }
};

static Method Resolve(const char *ns, const char *cls, const char *name, int paramCount = -1)
{
    Method m;
    if (void *k = MemRes::GetClass(ns, cls))
    {
        m.mi = MemRes::FindMethodInfo(k, name, paramCount);
        if (m.mi)
            m.ptr = *reinterpret_cast<void **>(m.mi);
    }
    return m;
}

template <class Fn>
static Fn As(const Method &m) { return reinterpret_cast<Fn>(m.ptr); }

// Install a MinHook detour on a resolved method; returns the original trampoline.
template <class Fn>
static Fn InstallHook(const char *ns, const char *cls, const char *name, int paramCount, void *detour)
{
    Method m = Resolve(ns, cls, name, paramCount);
    if (!m)
    {
        std::cout << "[!] hook target not found: " << ns << "." << cls << "::" << name << "\n";
        return nullptr;
    }
    void *orig = nullptr;
    if (MH_CreateHook(m.ptr, detour, &orig) == MH_OK && MH_EnableHook(m.ptr) == MH_OK)
        return reinterpret_cast<Fn>(orig);
    std::cout << "[!] MinHook failed for " << ns << "." << cls << "::" << name << "\n";
    return nullptr;
}

// Note: we deliberately do NOT heap-scan for instances. A scan for "object whose
// klass-pointer == target class" also matches stack slots and field references that
// merely hold the pointer, and calling a method on such a bogus "instance" crashes.
// Real instances are captured from hook detours (the genuine `this`).

// =============================================================
// State
// =============================================================

static bool g_initialized = false;

// command requests (set by exported funcs, applied on the game thread)
static bool g_setStarsReq = false; static int g_starsValue = 0;
static bool g_setNotesReq = false; static int g_notesValue = 0;
static bool g_finishReq = false;
static bool g_unlockReq = false;
static bool g_noCollision = false;

// captured instances
static void *g_coins = nullptr;       // CoinsController
static void *g_level = nullptr;       // LevelBase (fallback for GetCurrentLevel)
static void *g_customCtrl = nullptr;  // LineCustomizationController (captured via get_Settings)
static bool g_hatsDone = false;       // unlock-hats applied once
static bool g_unlockApplied = false;  // an unlock ran this session (drives refresh-on-tab-show)

// finish-level forced-result state
static bool g_forcePerfect = false;
static bool g_finishing = false; // re-entrancy guard for the finish sequence
static void *g_perfectLevel = nullptr;

// hook trampolines (originals)
static OnUpdate_t o_LevelOnUpdate = nullptr;
static VoidInst_t o_UpdateLoop = nullptr;
static VoidInst_t o_CoinsStart = nullptr;
static VoidInst_t o_CoinsRefresh = nullptr;
static GetIntInst_t o_RunStars = nullptr;
static GetIntInst_t o_RunStarsCollected = nullptr;
static GetIntInst_t o_RunStarsWhenDie = nullptr;
static GetIntInst_t o_RunDiamonds = nullptr;
static GetBoolInst_t o_IsPerfect = nullptr;
static Kill_t o_HeroKill = nullptr;
static Kill_t o_HeroClassicKill = nullptr;
static Kill_t o_HeroCollision = nullptr;
static VoidInst_t o_CharKilled = nullptr;
static HeroKilled_t o_LevelHeroKilled = nullptr;
static VoidInst_t o_KillHaracter = nullptr;
static Kill_t o_LevelBorder = nullptr;
static VoidInst_t o_FakeKilled = nullptr;
static GetPtrInst_t o_GetSettings = nullptr;
static VoidInst_t o_SkinTabShow = nullptr;
static VoidInst_t o_HatsTabShow = nullptr;

// =============================================================
// Cheat application (always called on the game thread, from a hook)
// =============================================================

static bool CallSaveData()
{
    Method m = Resolve(N::GLOBAL, "UserDataManager", "SaveData", -1);
    if (!m)
        return false;
    As<StaticBool_t>(m)(m.mi);
    return true;
}

// Build an eint from an int via DancingLine.Security.eint.op_Implicit(int) (the value
// encryption). Picks the int->eint overload (param 0 == I4) among the op_Implicit methods.
static bool BuildEInt(int value, EInt &out)
{
    void *eintClass = MemRes::GetClass("DancingLine.Security", "eint");
    if (!eintClass)
        return false;
    void *mi = MemRes::FindMethodInfoP0(eintClass, "op_Implicit", 1, MemRes::IL2CPP_TYPE_I4);
    if (!mi)
        return false;
    out = reinterpret_cast<IntToEInt_t>(*reinterpret_cast<void **>(mi))(value, mi);
    return true;
}

// Original working method: set the eint value directly via the setter, then refresh the UI.
static bool ApplyStars(int value)
{
    if (value < 0)
        value = 0;
    void *coinsClass = MemRes::GetClass(N::MONET, "CoinsController");
    void *inst = g_coins ? g_coins : MemRes::GetSingleton(coinsClass); // singleton, no hook needed
    if (!inst || !coinsClass)
        return false;

    EInt e{};
    if (!BuildEInt(value, e))
        return false;

    Method setCoins = Resolve(N::MONET, "CoinsController", "set_Coins", 1);
    if (!setCoins)
        return false;
    reinterpret_cast<SetCoinsEInt_t>(setCoins.ptr)(inst, e, setCoins.mi);

    if (Method rc = Resolve(N::MONET, "CoinsController", "RefreshCounter", 0))
        As<VoidInst_t>(rc)(inst, rc.mi);
    CallSaveData();
    std::cout << "[+] Stars set to " << value << ".\n";
    return true;
}

// Original working method: add/reduce by delta (these fire the note-counter change event
// that refreshes the UI; the direct set_MusicialNoteAmount setter does not). All static.
static bool ApplyNotes(int value)
{
    if (value < 0)
        value = 0;
    Method get = Resolve(N::MUSIC, "MusicialManager", "get_MusicialNoteAmount", 0);
    Method add = Resolve(N::MUSIC, "MusicialManager", "addMusicNote", 1);
    Method reduce = Resolve(N::MUSIC, "MusicialManager", "reduceMusicNote", 1);
    if (!get || !add || !reduce)
        return false;
    int cur = As<StaticGetInt_t>(get)(get.mi);
    int delta = value - cur;
    if (delta > 0)
        As<StaticSetInt_t>(add)(delta, add.mi);
    else if (delta < 0)
        As<StaticSetInt_t>(reduce)(-delta, reduce.mi);
    CallSaveData();
    std::cout << "[+] Notes set to " << value << " (was " << cur << ").\n";
    return true;
}

static void ApplyPerfectStats(void *klass, void *level)
{
    int crowns = 3, diamonds = 10;
    if (Method m = Resolve(N::LEVEL, "LevelBase", "StarReached", 1))
        for (int i = 0; i < crowns; ++i)
            As<IntInst_t>(m)(level, i, m.mi);
    if (Method m = Resolve(N::LEVEL, "LevelBase", "set_StarsCollected", 1))
        As<IntInst_t>(m)(level, crowns, m.mi);
    if (Method m = Resolve(N::LEVEL, "LevelBase", "set_DiamontsCollected", 1))
        As<IntInst_t>(m)(level, diamonds, m.mi);
    if (Method m = Resolve(N::LEVEL, "LevelBase", "set_IsPerfectReachedBefore", 1))
        As<BoolInst_t>(m)(level, true, m.mi);

    int diamondsOff = MemRes::FindFieldOffset(klass, "_currentDiamontsCollected");
    if (diamondsOff >= 0)
        *reinterpret_cast<int *>((uint8_t *)level + diamondsOff) = diamonds;
}

// The current level from the LevelsController singleton (never stale, unlike a captured
// instance from a previous level). Backing fields are obfuscated, but the getter name is intact.
static void *GetCurrentLevel()
{
    void *lcClass = MemRes::GetClass(N::DL, "LevelsController");
    void *lc = MemRes::GetSingleton(lcClass);
    Method getCur = Resolve(N::DL, "LevelsController", "get_CurrentLevel", 0);
    if (lc && getCur)
        return As<GetPtrInst_t>(getCur)(lc, getCur.mi);
    return g_level; // fallback to the captured instance
}

// True only while the line is actually moving (in-level), false in the "tap to play" / ready
// pre-level states. Replaces the original's obfuscated `<started>` field check.
static bool LevelIsPlaying(void *level)
{
    if (!level)
        return false;
    Method m = Resolve(N::LEVEL, "LevelBase", "get_IsPlaying", 0);
    return m && As<GetBoolInst_t>(m)(level, m.mi);
}

// True only when `level` is the on-screen, playable level — i.e. the "ready / tap to play"
// pre-level state or the line-moving in-level state. False on the menu, store, level-select,
// and the post-run results screen. Used to decide whether a finish request is honored or
// silently dropped, so triggering "Finish Level Perfectly" outside a level does nothing.
static bool LevelIsActiveContext(void *level)
{
    if (!level)
        return false;
    // A completed run is the results screen, not an active level.
    if (Method m = Resolve(N::LEVEL, "LevelBase", "get_IsRunCompleted", 0))
        if (As<GetBoolInst_t>(m)(level, m.mi))
            return false;
    // Line moving -> definitely in-level.
    if (LevelIsPlaying(level))
        return true;
    // Not playing yet: only "ready" if the level scene is actually loaded (resources up).
    // At the menu / level-select the previous level's resources are unloaded, so this is false.
    if (Method m = Resolve(N::LEVEL, "LevelBase", "get_AreResourcesLoaded", 0))
        return As<GetBoolInst_t>(m)(level, m.mi);
    return false;
}

static bool g_finishWaitLogged = false;
static bool g_finishDropLogged = false;

static bool ApplyFinishLevel()
{
    if (g_finishing)
        return true; // re-entrant call (e.g. a kill fires mid-sequence): no-op
    void *level = GetCurrentLevel();
    void *klass = MemRes::GetClass(N::LEVEL, "LevelBase");

    // Not in a level (menu / store / level-select / results screen): drop the request so it
    // does nothing, instead of letting it linger and fire when the next level starts moving.
    if (!level || !klass || !LevelIsActiveContext(level))
    {
        if (!g_finishDropLogged)
        {
            std::cout << "[*] Finish Level Perfectly ignored (not in a level).\n";
            g_finishDropLogged = true;
        }
        return true; // clears g_finishReq in Pump
    }

    if (!LevelIsPlaying(level))
    {
        // In a level but still in the "ready / tap to play" pre-level state: keep it queued
        // and let it apply the moment the line starts moving.
        if (!g_finishWaitLogged)
        {
            std::cout << "[*] Finish Level Perfectly queued until the line starts moving.\n";
            g_finishWaitLogged = true;
        }
        return false; // wait for the actual in-level state
    }
    g_finishWaitLogged = false;
    g_finishDropLogged = false;

    // One-shot: clear the request and mark "finishing" up front so the heavy CompleteLevel/
    // ShowLevelAfterCompleted sequence runs exactly once, even though calling them re-enters
    // the game's update/kill hooks. Kill detours suppress death while g_finishing is set.
    g_finishing = true;
    g_finishReq = false;
    g_forcePerfect = true;
    g_perfectLevel = level;
    ApplyPerfectStats(klass, level);

    if (Method m = Resolve(N::LEVEL, "LevelBase", "Reached100", 0))
        As<VoidInst_t>(m)(level, m.mi);
    if (Method m = Resolve(N::LEVEL, "LevelBase", "CompleteLevel", 0))
        As<VoidInst_t>(m)(level, m.mi);
    if (Method m = Resolve(N::LEVEL, "LevelBase", "ShowLevelAfterCompleted", 0))
        As<VoidInst_t>(m)(level, m.mi);
    ApplyPerfectStats(klass, level);
    CallSaveData();
    g_finishing = false;
    std::cout << "[+] Finish Level Perfectly applied.\n";
    return true;
}

// Original working method: get the LineCustomizationController, enumerate get_Settings()
// (an Il2CppArray of settings objects), and Unlock each not-yet-unlocked one. The controller
// is created lazily (exists once the customization screen is touched), so we use the instance
// captured by the get_Settings hook, falling back to the static singleton.
static int UnlockLineSkins()
{
    void *ctrlClass = MemRes::GetClass(N::CUSTOM, "LineCustomizationController");
    void *ctrl = g_customCtrl ? g_customCtrl : MemRes::GetSingleton(ctrlClass);
    Method getSettings = Resolve(N::CUSTOM, "LineCustomizationController", "get_Settings", 0);
    if (!ctrl || !getSettings)
        return -1; // controller not available yet; retry once the skins screen is opened

    // call via the original trampoline to avoid re-entering the capture detour
    auto arr = o_GetSettings ? o_GetSettings(ctrl, getSettings.mi)
                             : As<GetPtrInst_t>(getSettings)(ctrl, getSettings.mi);
    if (!arr)
        return 0;
    // Il2CppArray (x64): max_length @ 0x18, data starts @ 0x20
    uint32_t len = *reinterpret_cast<uint32_t *>((uint8_t *)arr + 0x18);
    void **items = reinterpret_cast<void **>((uint8_t *)arr + 0x20);
    int unlocked = 0;
    for (uint32_t i = 0; i < len && i < 4096; ++i)
    {
        void *settings = items[i];
        if (!settings)
            continue;
        // resolve Unlock / get_IsUnlocked on the element's actual runtime class
        void *sKlass = *reinterpret_cast<void **>(settings); // object->klass @ offset 0
        if (!sKlass)
            continue;
        void *miUnlock = MemRes::FindMethodInfo(sKlass, "Unlock", 0);
        void *miIs = MemRes::FindMethodInfo(sKlass, "get_IsUnlocked", 0);
        if (miIs && reinterpret_cast<GetBoolInst_t>(*(void **)miIs)(settings, miIs))
            continue;
        if (miUnlock)
        {
            reinterpret_cast<VoidInst_t>(*(void **)miUnlock)(settings, miUnlock);
            ++unlocked;
        }
    }
    return unlocked;
}

static int UnlockLineHats()
{
    Method m = Resolve(N::GLOBAL, "OrnamentController", "setHaveActReward", 1);
    if (!m)
        return -1;
    const int hatTypes[] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    for (int t : hatTypes)
        As<StaticSetInt_t>(m)(t, m.mi);
    return (int)(sizeof(hatTypes) / sizeof(hatTypes[0]));
}

// Refresh a store panel tab so the UI reflects the unlocks (Refresh is static).
static bool RefreshStoreTab(const char *className)
{
    Method m = Resolve(N::STORE, className, "Refresh", 0);
    if (!m)
        return false;
    As<StaticVoid_t>(m)(m.mi);
    return true;
}

static bool ApplyUnlock()
{
    // Hats (OrnamentController.setHaveActReward, static) apply immediately, once.
    if (!g_hatsDone && UnlockLineHats() >= 0)
        g_hatsDone = true;

    // Skins need the LineCustomizationController, which is created lazily. Keep the request
    // queued (return false) until it exists, so opening the customization screen completes it.
    int skins = UnlockLineSkins();
    if (skins < 0)
        return false;

    CallSaveData();
    g_unlockApplied = true;               // make each tab refresh itself when next shown
    RefreshStoreTab("StorePanelTabSkin"); // refresh the currently-visible tab now
    RefreshStoreTab("StorePanelTabHats");
    std::cout << "[+] Unlock applied. skins changed=" << skins << " hatsDone=" << g_hatsDone << "\n";
    return true;
}

// pump all queued commands (runs on the game thread)
static bool g_pumping = false;
static void Pump()
{
    if (g_pumping)
        return;
    g_pumping = true;
    if (g_setStarsReq && ApplyStars(g_starsValue))
        g_setStarsReq = false;
    if (g_setNotesReq && ApplyNotes(g_notesValue))
        g_setNotesReq = false;
    if (g_finishReq && ApplyFinishLevel())
        g_finishReq = false;
    if (g_unlockReq && ApplyUnlock())
        g_unlockReq = false;
    g_pumping = false;
}

// =============================================================
// Detours
// =============================================================

static void Detour_UpdateLoop(void *inst, const void *method)
{
    if (o_UpdateLoop)
        o_UpdateLoop(inst, method);
    Pump();
}

static void Detour_LevelOnUpdate(void *inst, float a, float b, float c, const void *method)
{
    if (inst)
        g_level = inst;
    if (o_LevelOnUpdate)
        o_LevelOnUpdate(inst, a, b, c, method);
    Pump();
}

// Capture-only detours: just record the instance. Commands are applied from the main
// update-loop ticks (Pump), never mid-UI-render, to avoid stalling the frame.
static void Detour_CoinsStart(void *inst, const void *method)
{
    if (inst)
        g_coins = inst;
    if (o_CoinsStart)
        o_CoinsStart(inst, method);
}

static void Detour_CoinsRefresh(void *inst, const void *method)
{
    if (inst)
        g_coins = inst;
    if (o_CoinsRefresh)
        o_CoinsRefresh(inst, method);
}

// capture the LineCustomizationController instance when the game accesses its settings
// (i.e. when the customization/skins screen is shown), so Unlock can run.
static void *Detour_GetSettings(void *inst, const void *method)
{
    if (inst)
        g_customCtrl = inst;
    return o_GetSettings ? o_GetSettings(inst, method) : nullptr;
}

// After an unlock, refresh each store tab when it's shown. The static Refresh only reaches the
// currently-subscribed (visible) tab, so navigating to the other page would otherwise stay stale.
static void Detour_SkinTabShow(void *inst, const void *method)
{
    if (o_SkinTabShow)
        o_SkinTabShow(inst, method);
    if (g_unlockApplied)
        RefreshStoreTab("StorePanelTabSkin");
}
static void Detour_HatsTabShow(void *inst, const void *method)
{
    if (o_HatsTabShow)
        o_HatsTabShow(inst, method);
    if (g_unlockApplied)
        RefreshStoreTab("StorePanelTabHats");
}

// forced perfect-result getters
static int Detour_RunStars(void *inst, const void *method)
{
    if (g_forcePerfect && (!g_perfectLevel || g_perfectLevel == inst))
        return 3;
    return o_RunStars ? o_RunStars(inst, method) : 0;
}
static int Detour_RunStarsCollected(void *inst, const void *method)
{
    if (g_forcePerfect && (!g_perfectLevel || g_perfectLevel == inst))
        return 3;
    return o_RunStarsCollected ? o_RunStarsCollected(inst, method) : 0;
}
static int Detour_RunStarsWhenDie(void *inst, const void *method)
{
    if (g_forcePerfect && (!g_perfectLevel || g_perfectLevel == inst))
        return 3;
    return o_RunStarsWhenDie ? o_RunStarsWhenDie(inst, method) : 0;
}
static int Detour_RunDiamonds(void *inst, const void *method)
{
    if (g_forcePerfect && (!g_perfectLevel || g_perfectLevel == inst))
        return 10;
    return o_RunDiamonds ? o_RunDiamonds(inst, method) : 0;
}
static bool Detour_IsPerfect(void *inst, const void *method)
{
    if (g_forcePerfect && (!g_perfectLevel || g_perfectLevel == inst))
        return true;
    return o_IsPerfect ? o_IsPerfect(inst, method) : false;
}

// no-collision suppressors
static void Detour_HeroKill(void *inst, void *arg, const void *method)
{
    if (g_finishReq || g_finishing) return; // suppress death; Pump applies the finish
    if (g_noCollision) return;
    if (o_HeroKill) o_HeroKill(inst, arg, method);
}
static void Detour_HeroClassicKill(void *inst, void *arg, const void *method)
{
    if (g_finishReq || g_finishing) return; // suppress death; Pump applies the finish
    if (g_noCollision) return;
    if (o_HeroClassicKill) o_HeroClassicKill(inst, arg, method);
}
static void Detour_HeroCollision(void *inst, void *arg, const void *method)
{
    if (g_finishReq || g_finishing) return; // suppress death; Pump applies the finish
    if (g_noCollision) return;
    if (o_HeroCollision) o_HeroCollision(inst, arg, method);
}
static void Detour_CharKilled(void *inst, const void *method)
{
    if (g_finishReq || g_finishing) return; // suppress death; Pump applies the finish
    if (g_noCollision) return;
    if (o_CharKilled) o_CharKilled(inst, method);
}
static void Detour_LevelHeroKilled(void *inst, void *hero, const void *method)
{
    if (inst) g_level = inst;
    if (g_finishReq || g_finishing) return; // suppress death; Pump applies the finish
    if (g_noCollision) return;
    if (o_LevelHeroKilled) o_LevelHeroKilled(inst, hero, method);
}
static void Detour_KillHaracter(void *inst, const void *method)
{
    if (g_finishReq || g_finishing) return; // suppress death; Pump applies the finish
    if (g_noCollision) return;
    if (o_KillHaracter) o_KillHaracter(inst, method);
}
static void Detour_LevelBorder(void *inst, void *col, const void *method)
{
    if (g_finishReq || g_finishing) return; // suppress death; Pump applies the finish
    if (g_noCollision) return;
    if (o_LevelBorder) o_LevelBorder(inst, col, method);
}
static void Detour_FakeKilled(void *inst, const void *method)
{
    if (g_finishReq || g_finishing) return; // suppress death; Pump applies the finish
    if (g_noCollision) return;
    if (o_FakeKilled) o_FakeKilled(inst, method);
}

// =============================================================
// Initialization
// =============================================================

static bool Initialize()
{
    if (g_initialized)
        return true;

    if (!MemRes::Init())
    {
        std::cout << "[!] MemRes::Init failed (no Assembly-CSharp image).\n";
        return false;
    }
    std::cout << "[+] Resolver ready. classes=" << MemRes::g_classes.size() << "\n";

    MH_Initialize();

    // tick sources (apply queued commands on the game thread). UpdateLoopHandler.Update fires
    // every frame in every screen (menu included); LevelBase.OnUpdate also captures the level.
    o_UpdateLoop = InstallHook<VoidInst_t>(N::GAMELOOP, "UpdateLoopHandler", "Update", 0, (void *)Detour_UpdateLoop);
    o_LevelOnUpdate = InstallHook<OnUpdate_t>(N::LEVEL, "LevelBase", "OnUpdate", 3, (void *)Detour_LevelOnUpdate);
    o_CoinsStart = InstallHook<VoidInst_t>(N::MONET, "CoinsController", "Start", 0, (void *)Detour_CoinsStart);
    o_CoinsRefresh = InstallHook<VoidInst_t>(N::MONET, "CoinsController", "RefreshCounter", 0, (void *)Detour_CoinsRefresh);
    o_GetSettings = InstallHook<GetPtrInst_t>(N::CUSTOM, "LineCustomizationController", "get_Settings", 0, (void *)Detour_GetSettings);
    o_SkinTabShow = InstallHook<VoidInst_t>(N::STORE, "StorePanelTabSkin", "Show", 0, (void *)Detour_SkinTabShow);
    o_HatsTabShow = InstallHook<VoidInst_t>(N::STORE, "StorePanelTabHats", "Show", 0, (void *)Detour_HatsTabShow);

    // forced perfect-result getters
    o_RunStars = InstallHook<GetIntInst_t>(N::LEVEL, "LevelBase", "get_CurrentRunStars", 0, (void *)Detour_RunStars);
    o_RunStarsCollected = InstallHook<GetIntInst_t>(N::LEVEL, "LevelBase", "get_CurrentRunStarsCollected", 0, (void *)Detour_RunStarsCollected);
    o_RunStarsWhenDie = InstallHook<GetIntInst_t>(N::LEVEL, "LevelBase", "get_CurrentRunStarsCollectedWhenDie", 0, (void *)Detour_RunStarsWhenDie);
    o_RunDiamonds = InstallHook<GetIntInst_t>(N::LEVEL, "LevelBase", "get_CurrentDiamontsCollected", 0, (void *)Detour_RunDiamonds);
    o_IsPerfect = InstallHook<GetBoolInst_t>(N::LEVEL, "LevelBase", "get_IsPerfectOnCurrentRun", 0, (void *)Detour_IsPerfect);

    // no-collision / finish-on-death hooks
    o_HeroKill = InstallHook<Kill_t>(N::CHAR, "Hero", "Kill", 1, (void *)Detour_HeroKill);
    o_HeroClassicKill = InstallHook<Kill_t>(N::CHAR, "HeroClassic", "Kill", 1, (void *)Detour_HeroClassicKill);
    o_HeroCollision = InstallHook<Kill_t>(N::CHAR, "Hero", "OnCollisionEnter", 1, (void *)Detour_HeroCollision);
    o_CharKilled = InstallHook<VoidInst_t>(N::CHAR, "GameCharacter", "Killed", 0, (void *)Detour_CharKilled);
    o_LevelHeroKilled = InstallHook<HeroKilled_t>(N::LEVEL, "LevelBase", "HeroKilled", 1, (void *)Detour_LevelHeroKilled);
    o_KillHaracter = InstallHook<VoidInst_t>(N::DL, "KillHaracter", "Kill", 0, (void *)Detour_KillHaracter);
    o_LevelBorder = InstallHook<Kill_t>(N::LEVEL_TRIG, "LevelBorder", "OnTriggerEnter", 1, (void *)Detour_LevelBorder);
    o_FakeKilled = InstallHook<VoidInst_t>(N::LEVEL_THELINE, "FakeGameCharacter", "Killed", 0, (void *)Detour_FakeKilled);

    g_initialized = true;
    std::cout << "[+] Dancing Line trainer initialized.\n";
    return true;
}

// =============================================================
// Exported cheat functions (run on injected remote threads; set flags only)
// =============================================================

extern "C" __declspec(dllexport) DWORD WINAPI SetStars(LPVOID lpParam)
{
    if (!Initialize())
        return 0;
    g_starsValue = *reinterpret_cast<int *>(lpParam);
    g_setStarsReq = true;
    std::cout << "[+] Set Stars requested: " << g_starsValue << "\n";
    return 1;
}

extern "C" __declspec(dllexport) DWORD WINAPI SetNotes(LPVOID lpParam)
{
    if (!Initialize())
        return 0;
    g_notesValue = *reinterpret_cast<int *>(lpParam);
    g_setNotesReq = true;
    std::cout << "[+] Set Notes requested: " << g_notesValue << "\n";
    return 1;
}

extern "C" __declspec(dllexport) DWORD WINAPI FinishLevelPerfectly(LPVOID lpParam)
{
    if (!Initialize())
        return 0;
    // Prevent multiple queuing: ignore repeat presses while a request is pending or applying.
    if (g_finishReq || g_finishing)
    {
        std::cout << "[*] Finish Level Perfectly already pending; ignoring repeat.\n";
        return 1;
    }
    // The game thread decides (next tick) whether we're actually in a level: it honors the
    // request in the ready / in-level states and silently drops it everywhere else.
    g_finishWaitLogged = false;
    g_finishDropLogged = false;
    g_finishReq = true;
    std::cout << "[+] Finish Level Perfectly requested.\n";
    return 1;
}

extern "C" __declspec(dllexport) DWORD WINAPI UnlockAllSkinsAndDecorations(LPVOID lpParam)
{
    if (!Initialize())
        return 0;
    g_unlockReq = true;
    std::cout << "[+] Unlock All queued.\n";
    return 1;
}

extern "C" __declspec(dllexport) DWORD WINAPI ToggleNoCollision(LPVOID lpParam)
{
    if (!Initialize())
        return 0;
    g_noCollision = *reinterpret_cast<bool *>(lpParam);
    std::cout << "[+] No Collision: " << (g_noCollision ? "ON" : "OFF") << "\n";
    return 1;
}

// =============================================================
// DllMain
// =============================================================

DWORD WINAPI MainThread(HMODULE)
{
#ifdef _DEBUG
    AllocConsole();
    freopen_s(reinterpret_cast<FILE **>(stdout), "CONOUT$", "w", stdout);

    // Disable the console's QuickEdit mode. Otherwise a click (or click-drag) in the console
    // enters text-selection mode, which blocks the next WriteConsole until you click away. Since
    // the cheats log through std::cout from inside the detours (i.e. on the game's own threads),
    // that selection would stall the game thread and freeze the whole game.
    HANDLE hConIn = GetStdHandle(STD_INPUT_HANDLE);
    DWORD conMode = 0;
    if (hConIn != INVALID_HANDLE_VALUE && GetConsoleMode(hConIn, &conMode))
        SetConsoleMode(hConIn, (conMode & ~ENABLE_QUICK_EDIT_MODE) | ENABLE_EXTENDED_FLAGS);
#endif
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hModule);
        CreateThread(nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(MainThread), hModule, 0, nullptr);
    }
    return TRUE;
}
