// trainer.h
#pragma once

#include <optional>

#include "TrainerBase.h"

// Geometry Dash 2.2091 (Steam). Native x64 C++ on cocos2d-x; every hook below was found and proven
// live with the native discovery pipeline - see docs/native_trainer_discovery.md.
//
// There is no fixed address in this file. Each site is an AOB with its displacements wildcarded,
// and every field offset and game function a cave uses is read out of the instruction the game
// itself uses it in, inside a matched AOB. A game update breaks a cheat only when the code around
// one of its signatures really changed; verify_aobs.py says which. Geode, the mod loader, hooks
// function entries - every signature here sits inside a function body, clear of those.
class Trainer : public TrainerBase
{
public:
    Trainer() : TrainerBase(L"GeometryDash.exe") {} // x64

    static inline const wchar_t *moduleName = L"GeometryDash.exe";

    // Auto Pickup Coins: whenever a level starts or resets, the chosen coins are picked up the way
    // GJBaseGameLayer::collisionCheckObjects does when the player touches one - destroyObject, then
    // pickupItem unless hasUniqueCoin says the coin is already saved. A coin's number (1-3, the one
    // GJGameLevel::getCoinKey files it under) decides whether it is chosen. Switching it on also
    // collects them in the level being played.
    bool toggleAutoPickupCoins(bool enable, bool coin1, bool coin2, bool coin3, std::string &message)
    {
        if (!enable)
            return !isActive(LEVEL_TICK) ||
                   (writeCaveData(LEVEL_TICK, "coinsMask", uint32_t{0}) && writeCaveData(LEVEL_TICK, "coinsNow", uint32_t{0}));

        const uint32_t mask = (coin1 ? 1u : 0u) | (coin2 ? 2u : 0u) | (coin3 ? 4u : 0u);
        if (mask == 0)
        {
            message = "Select at least one coin.";
            return false;
        }
        if (!installLevelHooks())
            return false;
        if (!pickup)
            return reportUnresolved(&Trainer::resolvePickup);
        return writeCaveData(LEVEL_TICK, "coinsMask", mask) && writeCaveData(LEVEL_TICK, "coinsNow", mask);
    }

    // Instant Complete: whenever a level starts or resets, the level tick starts the game's own
    // end-of-level animation - PlayLayer::playEndAnimationToPos, or playPlatformerEndAnimationToPos
    // in a platformer level. It waits for the anti-cheat spike to be hit first, a frame or two in,
    // because levelComplete saves that result with the completion. levelComplete also throws the
    // result away (no progress, no coins) when a rated level is finished in under 10 seconds; the one
    // write of that flag is removed while this is on. Switching it on also finishes the level being
    // played.
    bool toggleInstantComplete(bool enable)
    {
        if (!enable)
        {
            bool stopped = !isActive(LEVEL_TICK) ||
                           (writeCaveData(LEVEL_TICK, "completeOn", uint32_t{0}) && writeCaveData(LEVEL_TICK, "completeNow", uint32_t{0}));
            bool restored = disable("InstantCompleteSave");
            return stopped && restored;
        }

        if (!installLevelHooks())
            return false;
        if (!completion)
            return reportUnresolved(&Trainer::resolveCompletion);
        if (!patchCode("InstantCompleteSave", CodeSite::aob(moduleName, SIG_TOO_FAST, 0x33), std::vector<BYTE>(7, 0x90)))
            return false;
        if (writeCaveData(LEVEL_TICK, "completeWait", uint32_t{0}) &&
            writeCaveData(LEVEL_TICK, "completeOn", uint32_t{1}) &&
            writeCaveData(LEVEL_TICK, "completeNow", uint32_t{1}))
            return true;
        disable("InstantCompleteSave");
        return false;
    }

    // Jump Hack: a jump press marks the player as on the ground before PlayerObject::pushButton acts
    // on it, so every press is a jump from wherever the player is, in every mode that jumps.
    bool toggleJumpHack(bool enable)
    {
        if (!enable)
            return disable("JumpHack");

        int32_t onGround = 0;
        if (!readCodeInt32(findCode(CodeSite::aob(moduleName, SIG_ON_GROUND, 0x12)), onGround))
            return false;
        return hookCode("JumpHack", CodeSite::aob(moduleName, SIG_PUSH_BUTTON), 7,
                        [onGround](CaveBuilder &cave) { buildJumpHack(cave, onGround); });
    }

    // Song Trash Can: the trash button on a level's page (CustomSongWidget::deleteSong) deletes only
    // the level's main song, and a level that uses several songs or SFX gets no trash button at all.
    // With this on, every song and SFX the level uses - the widget's own lists of them - is deleted
    // the way the game deletes one: MusicDownloadManager::pathForSong / pathForSFX, then the CRT
    // remove() it calls. GD's bundled audio is never touched. Each one deleted is marked not
    // downloaded and the widget refreshed, so its download button comes back. Four pieces:
    //   SongTrashCan        deleteSong, after it removes the main song: the rest follow
    //   SongTrashCanNoSong  deleteSong's entry, for a level whose main song is GD's own: there is no
    //                       song object, so deleteSong would return - the rest are deleted instead
    //   SongTrashCanPrompt  onDelete, which asks first, skips the same check
    //   SongTrashCanButton  updateSongInfo shows the button on a multi-asset level once everything
    //                       is downloaded, where the download button would otherwise sit
    bool toggleSongTrashCan(bool enable, bool songs, bool sfx, std::string &message)
    {
        if (!enable)
        {
            if (!isActive("SongTrashCan"))
                return disable("SongTrashCan"); // says why
            return removeSongTrashCan();
        }

        const uint32_t mode = (songs ? 1u : 0u) | (sfx ? 2u : 0u);
        if (mode == 0)
        {
            message = "Select songs, SFX or both.";
            return false;
        }

        TrashCan trash;
        if (!resolveTrashCan(trash))
            return false;
        const bool installed =
            hookCode("SongTrashCan", CodeSite::aob(moduleName, SIG_DELETE_SONG, 0x36), 5,
                     [trash, mode](CaveBuilder &cave) { buildSongTrashCan(cave, trash, mode, false); }) &&
            hookCode("SongTrashCanNoSong", CodeSite::aob(moduleName, SIG_DELETE_SONG_START, 0x03), 8,
                     [trash, mode](CaveBuilder &cave) { buildSongTrashCan(cave, trash, mode, true); }) &&
            patchCode("SongTrashCanPrompt", CodeSite::aob(moduleName, SIG_ON_DELETE, 0x08), std::vector<BYTE>(6, 0x90)) &&
            hookCode("SongTrashCanButton", CodeSite::aob(moduleName, SIG_DELETE_BUTTON, 0x12), 12,
                     [trash](CaveBuilder &cave) { buildTrashButton(cave, trash); });
        if (!installed)
            removeSongTrashCan();
        return installed;
    }

    // Noclip: PlayLayer::destroyPlayer first checks whether the player hit the level's anti-cheat
    // spike, records that it did and returns; only then does it kill the player. Sending everything
    // past that check to its return leaves the anti-cheat spike working - levelComplete saves its
    // result - while nothing else can kill the player.
    bool toggleNoclip(bool enable)
    {
        if (!enable)
            return disable("Noclip");
        return patchCode("Noclip", CodeSite::aob(moduleName, SIG_DESTROY_PLAYER, 0x0C), {0xEB, 0xF9}); // jmp to the return above
    }

private:
    // ------------------------------------------------------------------------------------------------
    // Signatures. The offsets beside each are into its match.
    // ------------------------------------------------------------------------------------------------

    // PlayLayer::postUpdate, after its prologue: `mov rax,[rsi] ; movaps xmm1,xmm6 ; mov rcx,rsi` at
    // +0x11, with rsi the PlayLayer. GJBaseGameLayer::update has the same nine bytes, so the
    // signature starts before them.
    static constexpr const char *SIG_LEVEL_TICK =
        "0F 28 F1 75 ?? 48 8B 89 ?? ?? 00 00 E8 ?? ?? ?? ?? 48 8B 06 0F 28 CE 48 8B CE FF 90 ?? ?? 00 00 48 8B CE E8 ?? ?? ?? ??";

    // PlayLayer::resetLevel, after its prologue: `mov rsi,rcx ; mov rcx,[rcx+...]`.
    static constexpr const char *SIG_RESET_LEVEL =
        "48 8B F1 48 8B 89 ?? ?? 00 00 45 33 E4 44 38 A6 ?? ?? 00 00 75 ?? 48 85 C9 74 ?? 48 8B 01 FF 90 ?? ?? 00 00 4C 89 A6";

    // PlayLayer::levelComplete's too-fast check: +0x0B the isPlatformer byte's offset, +0x33 the
    // `mov byte ptr [rdi+...],1` that throws the completion away.
    static constexpr const char *SIG_TOO_FAST =
        "F2 0F 10 05 ?? ?? ?? ?? 40 38 B7 ?? ?? 00 00 74 ?? 66 0F 2F 87 ?? ?? 00 00 76 ?? 66 0F 2F C1 76 ?? EB ?? "
        "44 88 7C 24 ?? 66 0F 2F C1 76 ?? 44 88 7C 24 ?? C6 87 ?? ?? 00 00 01";

    // GJBaseGameLayer::collisionCheckObjects, the secret coin case: +0x02 the practice-mode byte's
    // offset; rel32 calls at +0x3C destroyObject, +0x52 hasUniqueCoin, +0x61 pickupItem.
    static constexpr const char *SIG_COIN_TOUCHED =
        "80 BE ?? ?? 00 00 00 0F 85 ?? ?? ?? ?? 48 8B 07 45 33 C9 45 8B 86 ?? ?? 00 00 48 8B D6 48 8B CF FF 90 ?? ?? 00 00 "
        "48 8B 07 0F 28 CE 48 8B CF FF 90 ?? ?? 00 00 48 8B D7 48 8B CE E8 ?? ?? ?? ?? 48 8B D7 48 8B CE E8 ?? ?? ?? ?? "
        "48 8B D7 48 8B CE E8 ?? ?? ?? ?? 84 C0 75 ?? 48 8B D7 48 8B CE E8 ?? ?? ?? ??";

    // hasUniqueCoin comparing the coin's number with 1: +0x02 its offset.
    static constexpr const char *SIG_COIN_NUMBER =
        "8B 87 ?? ?? 00 00 83 F8 01 75 ?? 48 8B 83 ?? ?? 00 00 8B 88 ?? ?? 00 00 2B 88 ?? ?? 00 00 83 F9 01 0F 94 C0";

    // collisionCheckObjects' switch on the object type: +0x02 GameObject's type offset.
    static constexpr const char *SIG_OBJECT_TYPE =
        "8B 87 ?? ?? 00 00 83 C0 FD 83 F8 ?? 0F 87 ?? ?? ?? ?? 48 98 41 8B 8C 87";

    // PlayLayer::addObject adding every object to its object array: +0x03 the array's offset.
    static constexpr const char *SIG_ADD_OBJECT =
        "49 8B 8E ?? ?? 00 00 48 8B D6 FF 15 ?? ?? ?? ?? 41 B0 01 48 8B D6 49 8B CE E8";

    // PlayLayer::checkForEnd: rel32 call to playEndAnimationToPos at +0x26.
    static constexpr const char *SIG_END_ANIMATION =
        "48 8B 1D ?? ?? ?? ?? 48 8B 9B ?? ?? 00 00 48 8B 07 48 8D 54 24 ?? 48 8B CF FF 90 ?? ?? 00 00 48 8B D0 48 8B CB E8 ?? ?? ?? ??";

    // PlayLayer::activatePlatformerEndTrigger: rel32 call to playPlatformerEndAnimationToPos at +0x22.
    static constexpr const char *SIG_PLATFORMER_END =
        "0F B6 9B ?? ?? 00 00 48 8D 54 24 ?? 48 8D 4C 24 ?? FF 15 ?? ?? ?? ?? 44 0F B6 C3 48 8B CF 48 8B D0 E8 ?? ?? ?? ??";

    // PlayerObject::pushButton, after its prologue: `cmp byte ptr [rbx+...],0`, with rbx the player
    // and edi the button.
    static constexpr const char *SIG_PUSH_BUTTON =
        "80 BB ?? ?? 00 00 00 0F 85 ?? ?? ?? ?? 80 BB ?? ?? 00 00 00 0F 85 ?? ?? ?? ?? "
        "48 89 6C 24 ?? 48 89 74 24 ?? 4C 89 74 24 ?? 4C 89 7C 24 ?? 83 FF 01";

    // pushButton's ball check reading m_isOnGround: +0x12 its offset.
    static constexpr const char *SIG_ON_GROUND =
        "38 83 ?? ?? 00 00 75 ?? 38 83 ?? ?? 00 00 74 ?? 38 83 ?? ?? 00 00 75 ?? 4C 8B 74 24";

    // PlayLayer::destroyPlayer: the anti-cheat spike branch sets its verified byte (+0x02 the
    // offset) and ends `jmp <return>` at +0x07; the death path starts at +0x0C.
    static constexpr const char *SIG_DESTROY_PLAYER =
        "C6 81 ?? ?? 00 00 01 E9 ?? ?? ?? ?? BA ?? 00 00 00 FF 15";

    // CustomSongWidget::deleteSong removing the main song's file: +0x03 the MusicDownloadManager
    // singleton's static (rip-relative), +0x1B rel32 pathForSong, +0x32 the remove() import slot
    // (rip-relative), +0x36 `lea rcx,[rsp+58]` - the hook - +0x3C rel32 the std::string destructor and
    // +0x44 rel32 updateSongInfo.
    static constexpr const char *SIG_DELETE_SONG =
        "48 8B 0D ?? ?? ?? ?? 4C 8B 87 ?? ?? 00 00 45 8B 80 ?? ?? 00 00 48 8D 54 24 ?? E8 ?? ?? ?? ?? "
        "48 8D 4C 24 ?? 48 83 7C 24 ?? 0F 48 0F 47 4C 24 ?? FF 15 ?? ?? ?? ?? 48 8D 4C 24 ?? E8 ?? ?? ?? ?? 48 8B CF E8 ?? ?? ?? ??";

    // CustomSongWidget::deleteSong's first check, `cmp qword ptr [rcx+...],0` - no song object, return
    // - at +0x03, its offset at +0x06; rdi is already the widget. CustomSongCell::onDelete has the
    // same code on other stack slots.
    static constexpr const char *SIG_DELETE_SONG_START =
        "48 8B F9 48 83 B9 ?? ?? 00 00 00 0F 84 ?? ?? ?? ?? 48 8D 44 24 38 48 89 44 24 30 48 8B 0D";

    // CustomSongWidget::onDelete's same check: its je at +0x08. Four other widgets' onDelete share
    // the first 30 bytes; only this one stores the prompt's text pointer straight from the call.
    static constexpr const char *SIG_ON_DELETE =
        "48 83 B9 ?? ?? 00 00 00 0F 84 ?? ?? ?? ?? 48 8D 99 ?? ?? 00 00 33 C0 48 85 C9 48 0F 44 D8 "
        "0F 57 C0 0F 11 44 24 ?? 48 89 44 24 ?? 48 89 44 24 ?? B9 ?? 00 00 00 E8 ?? ?? ?? ?? 48 89 44 24 ?? 48 C7 44 24";

    // CustomSongWidget::updateSongInfo choosing its multi-asset layout: +0x03 m_hasMultiAssets, +0x0E
    // m_hasUndownloadedAssets.
    static constexpr const char *SIG_MULTI_ASSET_FLAGS =
        "41 80 BE ?? ?? 00 00 00 74 ?? 41 0F B6 96 ?? ?? 00 00 E9";

    // The multi-asset layout hiding the delete button: `mov rcx,[r14+...] ; mov rax,[rcx] ; xor edx,edx`
    // before its setVisible at +0x12.
    static constexpr const char *SIG_DELETE_BUTTON =
        "49 8B 8E ?? ?? 00 00 48 8B 01 33 D2 FF 90 ?? ?? 00 00 49 8B 8E ?? ?? 00 00 48 8B 01 33 D2 FF 90 ?? ?? 00 00 "
        "49 8B 8E ?? ?? 00 00 48 85 C9 0F 84";

    // CustomSFXWidget::deleteSFX removing an SFX's file: +0x18 rel32 pathForSFX.
    static constexpr const char *SIG_DELETE_SFX =
        "48 8B 0D ?? ?? ?? ?? 4C 8B 83 ?? ?? 00 00 45 8B 40 ?? 48 8D 54 24 ?? E8 ?? ?? ?? ?? "
        "48 8D 4C 24 ?? 48 83 7C 24 ?? 0F 48 0F 47 4C 24 ?? FF 15";

    // CustomSongWidget::downloadAssetFinished marking an asset downloaded in its map: +0x08 the song
    // map's offset (asset type 1), +0x16 the SFX map's (type 2).
    static constexpr const char *SIG_ASSET_DOWNLOADED =
        "83 F8 01 75 ?? 48 81 C1 ?? ?? 00 00 EB ?? 83 F8 02 75 ?? 48 81 C1 ?? ?? 00 00 48 8B D3 E8 ?? ?? ?? ?? C6 00 01";

    // The same function's tail: rel32 call to updateMultiAssetInfo at +0x0F.
    static constexpr const char *SIG_MULTI_ASSET_INFO =
        "48 39 87 ?? ?? 00 00 74 ?? 33 D2 48 8B CF E8 ?? ?? ?? ?? 48 8B CF E8 ?? ?? ?? ?? 48 8B 5C 24 ?? 48 83 C4 20 5F C3";

    // ------------------------------------------------------------------------------------------------
    // What the caves need from the game
    // ------------------------------------------------------------------------------------------------

    // GameObjectType values collisionCheckObjects dispatches coins on.
    static constexpr BYTE SECRET_COIN = 22;
    static constexpr BYTE USER_COIN = 31;
    // cocos2d::CCArray keeps its ccArray at +0x38; a ccArray has its count at +0 and items at +0x10.
    static constexpr BYTE CCARRAY_DATA = 0x38;
    static constexpr BYTE CCARRAY_ITEMS = 0x10;
    // Frames Instant Complete waits for the anti-cheat spike before finishing anyway, for a level
    // that has none. The spike is hit on the first or second frame of an attempt.
    static constexpr BYTE SPIKE_WAIT_FRAMES = 60;

    struct Completion
    {
        int32_t isPlatformer = 0;  // PlayLayer, byte
        int32_t spikeVerified = 0; // PlayLayer, byte - the anti-cheat spike was hit this attempt
        uintptr_t playEndAnimation = 0;
        uintptr_t playPlatformerEndAnimation = 0;
    };

    struct Pickup
    {
        int32_t isPractice = 0; // PlayLayer, byte
        int32_t objects = 0;    // PlayLayer, CCArray *
        int32_t objectType = 0; // GameObject, int
        int32_t coinNumber = 0; // GameObject, int - 1 to 3 on a coin
        uintptr_t destroyObject = 0;
        uintptr_t hasUniqueCoin = 0;
        uintptr_t pickupItem = 0;
    };

    struct TrashCan
    {
        int32_t songInfo = 0;         // CustomSongWidget, SongInfoObject * - the main song, null for GD's own
        int32_t songs = 0;            // CustomSongWidget, std::map<int, bool> - asset ID to downloaded
        int32_t sfx = 0;              // the same for SFX
        int32_t hasMultiAssets = 0;   // CustomSongWidget, bool
        int32_t hasUndownloaded = 0;  // CustomSongWidget, bool - set by updateMultiAssetInfo
        uintptr_t musicManager = 0;   // the static holding MusicDownloadManager *
        uintptr_t pathForSong = 0;
        uintptr_t pathForSFX = 0;
        uintptr_t removeFile = 0;     // remove()'s import slot
        uintptr_t freeString = 0;     // std::string's destructor
        uintptr_t updateMultiAssetInfo = 0;
        uintptr_t updateSongInfo = 0;
    };

    static constexpr const char *TRASH_CAN_PARTS[] = {"SongTrashCan", "SongTrashCanNoSong", "SongTrashCanPrompt", "SongTrashCanButton"};

    // MSVC's std::map node: left, parent, right, then _Isnil at +0x19 and the pair at +0x1C - an int
    // key, and the bool value at +0x20. std::string keeps its capacity at +0x18; past 15 the text is
    // on the heap. Both are the compiler's library, not the game's, and do not move with an update.
    static constexpr BYTE NODE_PARENT = 0x08, NODE_RIGHT = 0x10, NODE_ISNIL = 0x19, NODE_KEY = 0x1C, NODE_VALUE = 0x20;

    // GD's bundled songs and SFX, which Song Trash Can (github.com/SawBlade21/Song-Trash-Can) refuses
    // to delete. A level can list them, and pathForSong / pathForSFX then name files the game ships.
    static constexpr int BUNDLED_AUDIO[] = {
        288, 458, 459, 462, 463, 470, 474, 500, 520, 536, 537, 566, 567, 599, 715, 740, 766, 767, 783, 788,
        886, 994, 996, 998, 1032, 1064, 1065, 1072, 1076, 1078, 1200, 1202, 1274, 1313, 1372, 1374, 1375,
        1377, 1528, 1548, 1552, 1553, 1554, 1558, 1564, 1573, 1601, 1611, 1618, 1619, 1739, 1740, 1837,
        2031, 2075, 2079, 2195, 2235, 2237, 2238, 2265, 2267, 2268, 2269, 2270, 2271, 2282, 2283, 2284,
        2286, 2297, 2326, 2366, 2367, 2441, 2454, 2508, 2516, 2529, 2530, 2552, 2581, 2582, 2588, 2590,
        2591, 2597, 2612, 2614, 2707, 2708, 2714, 2782, 2785, 2842, 2847, 2849, 2853, 2858, 2974, 2975,
        2981, 3016, 3116, 3120, 3121, 3122, 3125, 3335, 3376, 3448, 3533, 3556, 4055, 4158, 4261, 4272,
        4273, 4274, 4278, 4290, 4292, 4293, 4295, 4328, 4329, 4332, 4348, 4349, 4394, 4397, 4400, 4401,
        4404, 4451, 4467, 4504, 4538, 4550, 4811, 4821, 4829, 4831, 4846, 4848, 4849, 4857, 4881, 4882,
        4888, 4921, 5035, 5043, 5107, 5143, 5170, 5212, 5218, 5219, 5223, 6018, 6019, 6026, 6769, 6883,
        6892, 6897, 7220, 7243, 7245, 7250, 7295, 7309, 7310, 7326, 7342, 7359, 7360, 7361, 7376, 7383,
        7387, 7391, 7580, 7645, 7647, 7657, 7688, 7731, 7798, 7838, 7841, 7940, 7975, 8039, 8191, 8386,
        9465, 9483, 9484, 9485, 9489, 9623, 9702, 9877, 10117, 10162, 10163, 10271, 10322, 10335, 10362,
        10412, 10775, 12863, 12911, 12912, 12913, 12918, 12919, 13143, 13231, 13341, 13575, 13585, 13587,
        13700, 13702, 13844, 14011, 14013, 14046, 14061};

    // The level hooks are shared by Instant Complete and Auto Pickup Coins, which set the tick's data.
    // They are inert with both off, so they stay installed until the trainer closes. LevelReset
    // writes into LevelTick's cave, and disableAll() goes in name order, so it is undone first.
    static inline const std::string LEVEL_TICK = "LevelTick";
    static inline const std::string LEVEL_RESET = "LevelReset";
    std::optional<Completion> completion;
    std::optional<Pickup> pickup;

    // Each half of the tick is built only if its code was found, so a game update that breaks one
    // cheat leaves the other working.
    bool installLevelHooks()
    {
        if (!isActive(LEVEL_TICK))
        {
            Completion c;
            Pickup p;
            completion = resolveCompletion(c) ? std::optional<Completion>(c) : std::nullopt;
            pickup = resolvePickup(p) ? std::optional<Pickup>(p) : std::nullopt;
            if (!completion && !pickup)
                return false;
            if (!hookCode(LEVEL_TICK, CodeSite::aob(moduleName, SIG_LEVEL_TICK, 0x11), 9,
                          [c = completion, p = pickup](CaveBuilder &cave) { buildLevelTick(cave, c, p); }))
                return false;
        }
        if (isActive(LEVEL_RESET))
            return true;

        const uintptr_t levelReset = caveLabelAddress(LEVEL_TICK, "levelReset");
        return levelReset != 0 &&
               hookCode(LEVEL_RESET, CodeSite::aob(moduleName, SIG_RESET_LEVEL), 10,
                        [levelReset](CaveBuilder &cave) { buildResetLevel(cave, levelReset); });
    }

    bool resolveCompletion(Completion &c)
    {
        const uintptr_t tooFast = findCode(CodeSite::aob(moduleName, SIG_TOO_FAST));
        const uintptr_t spike = findCode(CodeSite::aob(moduleName, SIG_DESTROY_PLAYER));
        const uintptr_t end = findCode(CodeSite::aob(moduleName, SIG_END_ANIMATION));
        const uintptr_t platformerEnd = findCode(CodeSite::aob(moduleName, SIG_PLATFORMER_END));
        return readCodeInt32(tooFast ? tooFast + 0x0B : 0, c.isPlatformer) &&
               readCodeInt32(spike ? spike + 0x02 : 0, c.spikeVerified) &&
               readCallTarget(end ? end + 0x26 : 0, c.playEndAnimation) &&
               readCallTarget(platformerEnd ? platformerEnd + 0x22 : 0, c.playPlatformerEndAnimation);
    }

    bool resolvePickup(Pickup &p)
    {
        const uintptr_t coin = findCode(CodeSite::aob(moduleName, SIG_COIN_TOUCHED));
        const uintptr_t number = findCode(CodeSite::aob(moduleName, SIG_COIN_NUMBER));
        const uintptr_t type = findCode(CodeSite::aob(moduleName, SIG_OBJECT_TYPE));
        const uintptr_t add = findCode(CodeSite::aob(moduleName, SIG_ADD_OBJECT));
        return readCodeInt32(coin ? coin + 0x02 : 0, p.isPractice) &&
               readCodeInt32(number ? number + 0x02 : 0, p.coinNumber) &&
               readCodeInt32(type ? type + 0x02 : 0, p.objectType) &&
               readCodeInt32(add ? add + 0x03 : 0, p.objects) &&
               readCallTarget(coin ? coin + 0x3C : 0, p.destroyObject) &&
               readCallTarget(coin ? coin + 0x52 : 0, p.hasUniqueCoin) &&
               readCallTarget(coin ? coin + 0x61 : 0, p.pickupItem);
    }

    bool resolveTrashCan(TrashCan &t)
    {
        const uintptr_t start = findCode(CodeSite::aob(moduleName, SIG_DELETE_SONG_START));
        const uintptr_t song = findCode(CodeSite::aob(moduleName, SIG_DELETE_SONG));
        const uintptr_t sfx = findCode(CodeSite::aob(moduleName, SIG_DELETE_SFX));
        const uintptr_t maps = findCode(CodeSite::aob(moduleName, SIG_ASSET_DOWNLOADED));
        const uintptr_t info = findCode(CodeSite::aob(moduleName, SIG_MULTI_ASSET_INFO));
        const uintptr_t flags = findCode(CodeSite::aob(moduleName, SIG_MULTI_ASSET_FLAGS));
        // A rip-relative operand ends its instruction just as a rel32 does, so both resolve alike.
        return readCodeInt32(start ? start + 0x06 : 0, t.songInfo) &&
               readCallTarget(song ? song + 0x03 : 0, t.musicManager) &&
               readCallTarget(song ? song + 0x1B : 0, t.pathForSong) &&
               readCallTarget(song ? song + 0x32 : 0, t.removeFile) &&
               readCallTarget(song ? song + 0x3C : 0, t.freeString) &&
               readCallTarget(song ? song + 0x44 : 0, t.updateSongInfo) &&
               readCallTarget(sfx ? sfx + 0x18 : 0, t.pathForSFX) &&
               readCodeInt32(maps ? maps + 0x08 : 0, t.songs) &&
               readCodeInt32(maps ? maps + 0x16 : 0, t.sfx) &&
               readCodeInt32(flags ? flags + 0x03 : 0, t.hasMultiAssets) &&
               readCodeInt32(flags ? flags + 0x0E : 0, t.hasUndownloaded) &&
               readCallTarget(info ? info + 0x0F : 0, t.updateMultiAssetInfo);
    }

    bool removeSongTrashCan()
    {
        bool removed = true;
        for (const char *part : TRASH_CAN_PARTS)
            if (isActive(part))
                removed = disable(part) && removed;
        return removed;
    }

    /// Runs a resolver again for its messages - the tick was installed without that half.
    template <typename T>
    bool reportUnresolved(bool (Trainer::*resolve)(T &))
    {
        T unused;
        (this->*resolve)(unused);
        return false;
    }

    /// The int32 at address - a displacement inside a matched instruction. 0 is a site already
    /// reported missing.
    bool readCodeInt32(uintptr_t address, int32_t &out)
    {
        if (address == 0)
            return false;
        if (!readMemory(address, out))
        {
            std::cerr << "[!] Could not read the game's code at 0x" << std::hex << address << std::dec << ".\n";
            return false;
        }
        return true;
    }

    /// Where the call whose rel32 is at address goes.
    bool readCallTarget(uintptr_t address, uintptr_t &out)
    {
        int32_t rel = 0;
        if (!readCodeInt32(address, rel))
            return false;
        out = address + 4 + static_cast<intptr_t>(rel);
        return true;
    }

    static uint64_t point(float x, float y) // a cocos2d::CCPoint as one qword
    {
        uint32_t lo = 0, hi = 0;
        std::memcpy(&lo, &x, 4);
        std::memcpy(&hi, &y, 4);
        return lo | (static_cast<uint64_t>(hi) << 32);
    }

    // ------------------------------------------------------------------------------------------------
    // Caves
    // ------------------------------------------------------------------------------------------------

    // At PlayLayer::postUpdate+0x31: rsi is the PlayLayer, rsp is 16-aligned, and every volatile
    // register is about to be overwritten, so the cave calls game functions freely. Requests the
    // trainer can withdraw are taken with xchg, so each is either acted on or gone.
    static void buildLevelTick(CaveBuilder &cave, const std::optional<Completion> &c, const std::optional<Pickup> &p)
    {
        using Jcc = CaveBuilder::Jcc;

        // A reset since the last frame is a new attempt: re-arm whichever cheats are on.
        cave.bytes({0x31, 0xC0})                                             // xor eax,eax
            .ripRelative({0x87, 0x05}, "levelReset")                         // xchg [levelReset],eax
            .bytes({0x85, 0xC0})                                             // test eax,eax
            .jumpTo(Jcc::E, "coins")
            .ripRelative({0x8B, 0x05}, "coinsMask")                          // mov eax,[coinsMask]
            .ripRelative({0x89, 0x05}, "coinsNow")                           // mov [coinsNow],eax
            .ripRelative({0x8B, 0x05}, "completeOn")                         // mov eax,[completeOn]
            .ripRelative({0x89, 0x05}, "completeNow")                        // mov [completeNow],eax
            .bytes({0x31, 0xC0})                                             // xor eax,eax
            .ripRelative({0x89, 0x05}, "completeWait")                       // mov [completeWait],eax
            .label("coins");

        if (p)
        {
            // Auto Pickup Coins: every object in the level, coins only, those whose number is in the
            // mask. A coin numbered outside 1-3 is taken whenever any coin is chosen.
            cave.bytes({0x31, 0xC0})                                         // xor eax,eax
                .ripRelative({0x87, 0x05}, "coinsNow")                       // xchg [coinsNow],eax - the mask
                .bytes({0x85, 0xC0})                                         // test eax,eax
                .jumpTo(Jcc::E, "complete")
                .bytes({0x80, 0xBE}).value(p->isPractice).bytes({0x00})      // cmp byte ptr [rsi+isPractice],0
                .jumpTo(Jcc::NE, "complete")                                 // the game collects no coins there
                .bytes({0x53, 0x57, 0x41, 0x54, 0x41, 0x55})                 // push rbx ; push rdi ; push r12 ; push r13
                .bytes({0x48, 0x83, 0xEC, 0x20})                             // sub rsp,20
                .bytes({0x44, 0x8B, 0xE8})                                   // mov r13d,eax
                .bytes({0x48, 0x8B, 0x86}).value(p->objects)                 // mov rax,[rsi+objects]
                .bytes({0x48, 0x85, 0xC0})                                   // test rax,rax
                .jumpTo(Jcc::E, "restore")
                .bytes({0x48, 0x8B, 0x40, CCARRAY_DATA})                     // mov rax,[rax+38]
                .bytes({0x48, 0x85, 0xC0})                                   // test rax,rax
                .jumpTo(Jcc::E, "restore")
                .bytes({0x8B, 0x18})                                         // mov ebx,[rax] - count
                .bytes({0x48, 0x8B, 0x78, CCARRAY_ITEMS})                    // mov rdi,[rax+10] - items
                .label("loop")
                .bytes({0x85, 0xDB})                                         // test ebx,ebx
                .jumpTo(Jcc::E, "restore")
                .bytes({0x4C, 0x8B, 0x27})                                   // mov r12,[rdi]
                .bytes({0x4D, 0x85, 0xE4})                                   // test r12,r12
                .jumpTo(Jcc::E, "next")
                .bytes({0x41, 0x8B, 0x84, 0x24}).value(p->objectType)        // mov eax,[r12+objectType]
                .bytes({0x83, 0xF8, SECRET_COIN})                            // cmp eax,16
                .jumpTo(Jcc::E, "coin")
                .bytes({0x83, 0xF8, USER_COIN})                              // cmp eax,1F
                .jumpTo(Jcc::NE, "next")
                .label("coin")
                .bytes({0x41, 0x8B, 0x8C, 0x24}).value(p->coinNumber)        // mov ecx,[r12+coinNumber]
                .bytes({0xFF, 0xC9})                                         // dec ecx
                .bytes({0x83, 0xF9, 0x02})                                   // cmp ecx,2
                .jumpTo(Jcc::A, "pick")
                .bytes({0x41, 0x0F, 0xA3, 0xCD})                             // bt r13d,ecx
                .jumpTo(Jcc::AE, "next")                                     // not chosen
                .label("pick")
                .bytes({0x48, 0x8B, 0xCE, 0x49, 0x8B, 0xD4})                 // mov rcx,rsi ; mov rdx,r12
                .call(p->destroyObject)
                .bytes({0x48, 0x8B, 0xCE, 0x49, 0x8B, 0xD4})                 // mov rcx,rsi ; mov rdx,r12
                .call(p->hasUniqueCoin)
                .bytes({0x84, 0xC0})                                         // test al,al
                .jumpTo(Jcc::NE, "next")
                .bytes({0x48, 0x8B, 0xCE, 0x49, 0x8B, 0xD4})                 // mov rcx,rsi ; mov rdx,r12
                .call(p->pickupItem)
                .label("next")
                .bytes({0x48, 0x83, 0xC7, 0x08})                             // add rdi,8
                .bytes({0xFF, 0xCB})                                         // dec ebx
                .jumpTo(Jcc::Always, "loop")
                .label("restore")
                .bytes({0x48, 0x83, 0xC4, 0x20})                             // add rsp,20
                .bytes({0x41, 0x5D, 0x41, 0x5C, 0x5F, 0x5B});                // pop r13 ; pop r12 ; pop rdi ; pop rbx
        }
        cave.label("complete");

        if (c)
        {
            // Instant Complete, after the coins so they are in before the level ends. The end
            // animation takes its CCPoint by reference and writes to it.
            cave.ripRelative({0x8B, 0x05}, "completeNow")                    // mov eax,[completeNow]
                .bytes({0x85, 0xC0})                                         // test eax,eax
                .jumpTo(Jcc::E, "done")
                .bytes({0x80, 0xBE}).value(c->spikeVerified).bytes({0x00})   // cmp byte ptr [rsi+spikeVerified],0
                .jumpTo(Jcc::NE, "finish")
                .ripRelative({0xFF, 0x05}, "completeWait")                   // inc dword ptr [completeWait]
                .ripRelative({0x8B, 0x05}, "completeWait")                   // mov eax,[completeWait]
                .bytes({0x83, 0xF8, SPIKE_WAIT_FRAMES})                      // cmp eax,3C
                .jumpTo(Jcc::B, "done")
                .label("finish")
                .bytes({0x31, 0xC0})                                         // xor eax,eax
                .ripRelative({0x87, 0x05}, "completeNow")                    // xchg [completeNow],eax
                .bytes({0x85, 0xC0})                                         // test eax,eax
                .jumpTo(Jcc::E, "done")                                      // withdrawn meanwhile
                .bytes({0x31, 0xC0})                                         // xor eax,eax
                .ripRelative({0x89, 0x05}, "completeWait")                   // mov [completeWait],eax
                .bytes({0x48, 0x83, 0xEC, 0x30})                             // sub rsp,30
                .bytes({0x48, 0x8B, 0xCE})                                   // mov rcx,rsi
                .bytes({0x48, 0x8D, 0x54, 0x24, 0x20})                       // lea rdx,[rsp+20]
                .bytes({0x80, 0xBE}).value(c->isPlatformer).bytes({0x00})    // cmp byte ptr [rsi+isPlatformer],0
                .jumpTo(Jcc::NE, "platformer")
                .bytes({0x48, 0xB8}).value(point(2.0f, 2.0f))                // mov rax,{2,2}
                .bytes({0x48, 0x89, 0x44, 0x24, 0x20})                       // mov [rsp+20],rax
                .call(c->playEndAnimation)
                .jumpTo(Jcc::Always, "animating")
                .label("platformer")
                .bytes({0x48, 0xB8}).value(point(0.0f, 105.0f))              // mov rax,{0,105}
                .bytes({0x48, 0x89, 0x44, 0x24, 0x20})                       // mov [rsp+20],rax
                .bytes({0x41, 0xB0, 0x01})                                   // mov r8b,1 - instant
                .call(c->playPlatformerEndAnimation)
                .label("animating")
                .bytes({0x48, 0x83, 0xC4, 0x30});                            // add rsp,30
        }
        cave.label("done")
            .original() // mov rax,[rsi] ; movaps xmm1,xmm6 ; mov rcx,rsi
            .jmpBack();

        // The trainer writes these dwords while the game runs, so they are kept aligned.
        cave.label("data");
        cave.bytes(std::vector<BYTE>((4 - cave.labelOffset("data") % 4) % 4, 0xCC));
        cave.label("levelReset").value<uint32_t>(0)    // set by LevelReset
            .label("coinsMask").value<uint32_t>(0)     // Auto Pickup Coins: the chosen coins, 0 when off
            .label("coinsNow").value<uint32_t>(0)      // coins to collect on the next frame
            .label("completeOn").value<uint32_t>(0)    // Instant Complete is on
            .label("completeNow").value<uint32_t>(0)   // finish the level once the spike is hit
            .label("completeWait").value<uint32_t>(0); // frames spent waiting for it
    }

    // At PlayLayer::resetLevel+0x39, where rax is free.
    static void buildResetLevel(CaveBuilder &cave, uintptr_t levelReset)
    {
        cave.bytes({0x48, 0xB8}).value<uint64_t>(levelReset) // mov rax,levelReset
            .bytes({0xC7, 0x00, 0x01, 0x00, 0x00, 0x00})     // mov dword ptr [rax],1
            .original()                                      // mov rsi,rcx ; mov rcx,[rcx+...]
            .jmpBack();
    }

    // Two sites in CustomSongWidget::deleteSong, both with rdi the widget, rsp 16-aligned and the
    // volatile registers free:
    //   +0x1BB (noMainSong false), just after the main song's file is removed - the replayed
    //          `lea rcx,[rsp+58]` sets rcx again;
    //   +0x1F  (noMainSong true), its `cmp qword ptr [rcx+songInfo],0`. With a song object the cave
    //          leaves it to the other site; without one it deletes the rest, refreshes the widget
    //          itself, and the replayed cmp sends deleteSong to its return.
    static void buildSongTrashCan(CaveBuilder &cave, const TrashCan &t, uint32_t mode, bool noMainSong)
    {
        using Jcc = CaveBuilder::Jcc;

        int top = 0;
        for (int id : BUNDLED_AUDIO)
            top = id > top ? id : top;
        std::vector<BYTE> bundled(top / 8 + 1, 0);
        for (int id : BUNDLED_AUDIO)
            bundled[id / 8] |= static_cast<BYTE>(1 << (id % 8));
        const uint32_t limit = static_cast<uint32_t>(bundled.size() * 8);

        if (noMainSong)
            cave.bytes({0x48, 0x83, 0xB9}).value(t.songInfo).bytes({0x00})   // cmp qword ptr [rcx+songInfo],0
                .jumpTo(Jcc::NE, "done");

        cave.ripRelative({0x8B, 0x05}, "mode")                                // mov eax,[mode]
            .bytes({0x85, 0xC0})                                              // test eax,eax
            .jumpTo(Jcc::E, "done")
            .bytes({0x53, 0x56, 0x41, 0x54, 0x41, 0x55, 0x41, 0x56, 0x41, 0x57}) // push rbx ; rsi ; r12 ; r13 ; r14 ; r15
            .bytes({0x48, 0x83, 0xEC, 0x40})                                  // sub rsp,40 - shadow space, a std::string at +20
            .bytes({0x44, 0x8B, 0xF8})                                        // mov r15d,eax - what to delete
            .bytes({0x45, 0x33, 0xF6})                                        // xor r14d,r14d - files deleted
            .bytes({0x41, 0xF6, 0xC7, 0x01})                                  // test r15b,1 - songs
            .jumpTo(Jcc::E, "sfx");
        emitAssetWalk(cave, "song", t.songs, t.pathForSong, t, limit);
        cave.label("sfx")
            .bytes({0x41, 0xF6, 0xC7, 0x02})                                  // test r15b,2 - SFX
            .jumpTo(Jcc::E, "refresh");
        emitAssetWalk(cave, "sfx", t.sfx, t.pathForSFX, t, limit);
        cave.label("refresh")
            .bytes({0x45, 0x85, 0xF6})                                        // test r14d,r14d
            .jumpTo(Jcc::E, "restore")
            .bytes({0x48, 0x8B, 0xCF, 0xB2, 0x01})                            // mov rcx,rdi ; mov dl,1
            .call(t.updateMultiAssetInfo);                                    // as updateWithMultiAssets ends
        if (noMainSong)
            cave.bytes({0x48, 0x8B, 0xCF})                                    // mov rcx,rdi
                .call(t.updateSongInfo);                                      // deleteSong would have
        cave.label("restore")
            .bytes({0x48, 0x83, 0xC4, 0x40})                                  // add rsp,40
            .bytes({0x41, 0x5F, 0x41, 0x5E, 0x41, 0x5D, 0x41, 0x5C, 0x5E, 0x5B}); // pop r15 ; r14 ; r13 ; r12 ; rsi ; rbx
        if (noMainSong)
            cave.bytes({0x48, 0x8B, 0xCF});                                   // mov rcx,rdi - the cmp reads it
        cave.label("done")
            .original() // lea rcx,[rsp+58] at +0x1BB; cmp qword ptr [rcx+songInfo],0 at +0x1F
            .jmpBack();

        cave.label("data");
        cave.bytes(std::vector<BYTE>((4 - cave.labelOffset("data") % 4) % 4, 0xCC));
        cave.label("mode").value<uint32_t>(mode) // bit 0 songs, bit 1 SFX
            .label("bundled").bytes(bundled);    // one bit per ID in BUNDLED_AUDIO
    }

    // At CustomSongWidget::updateSongInfo+0xAD2, in the multi-asset layout: rcx and rax are loaded
    // for the delete button's setVisible, and edx - its argument - is set to "everything downloaded"
    // instead of false. The no-song layout runs the same code, so it stays hidden without assets.
    static void buildTrashButton(CaveBuilder &cave, const TrashCan &t)
    {
        cave.original(0, 10)                                                  // mov rcx,[r14+deleteBtn] ; mov rax,[rcx]
            .bytes({0x33, 0xD2})                                              // xor edx,edx
            .bytes({0x41, 0x80, 0xBE}).value(t.hasMultiAssets).bytes({0x00}) // cmp byte ptr [r14+hasMultiAssets],0
            .jumpTo(CaveBuilder::Jcc::E, "done")
            .bytes({0x41, 0x80, 0xBE}).value(t.hasUndownloaded).bytes({0x00}) // cmp byte ptr [r14+hasUndownloaded],0
            .bytes({0x0F, 0x94, 0xC2})                                        // sete dl
            .label("done")
            .jmpBack();                                                       // to its setVisible call
    }

    // Deletes every asset in one of the widget's maps, in order: r12 the map's head, rbx the node,
    // esi its ID. Counts each file removed in r14d and marks its node not downloaded.
    static void emitAssetWalk(CaveBuilder &cave, const std::string &map, int32_t mapOffset, uintptr_t pathFor,
                              const TrashCan &t, uint32_t limit)
    {
        using Jcc = CaveBuilder::Jcc;
        auto at = [&map](const char *name) { return map + "." + name; };

        cave.bytes({0x4C, 0x8B, 0xA7}).value(mapOffset)                       // mov r12,[rdi+map] - its head
            .bytes({0x4D, 0x85, 0xE4})                                        // test r12,r12
            .jumpTo(Jcc::E, at("end"))
            .bytes({0x49, 0x8B, 0x1C, 0x24})                                  // mov rbx,[r12] - leftmost node
            .label(at("loop"))
            .bytes({0x80, 0x7B, NODE_ISNIL, 0x00})                            // cmp byte ptr [rbx+19],0
            .jumpTo(Jcc::NE, at("end"))                                       // back at the head
            .bytes({0x8B, 0x73, NODE_KEY})                                    // mov esi,[rbx+1C]
            .bytes({0x81, 0xFE}).value(limit)                                 // cmp esi,limit
            .jumpTo(Jcc::AE, at("delete"))
            .ripRelative({0x0F, 0xA3, 0x35}, "bundled")                       // bt [bundled],esi
            .jumpTo(Jcc::B, at("next"))                                       // GD's own - kept
            .label(at("delete"))
            .bytes({0x48, 0xB8}).value<uint64_t>(t.musicManager)              // mov rax,&manager
            .bytes({0x48, 0x8B, 0x08})                                        // mov rcx,[rax]
            .bytes({0x48, 0x85, 0xC9})                                        // test rcx,rcx
            .jumpTo(Jcc::E, at("next"))
            .bytes({0x48, 0x8D, 0x54, 0x24, 0x20})                            // lea rdx,[rsp+20]
            .bytes({0x44, 0x8B, 0xC6})                                        // mov r8d,esi
            .call(pathFor)                                                    // the file's path, into [rsp+20]
            .bytes({0x48, 0x8D, 0x4C, 0x24, 0x20})                            // lea rcx,[rsp+20]
            .bytes({0x48, 0x83, 0x7C, 0x24, 0x38, 0x0F})                      // cmp qword ptr [rsp+38],F
            .bytes({0x48, 0x0F, 0x47, 0x4C, 0x24, 0x20})                      // cmova rcx,[rsp+20]
            .bytes({0x48, 0xB8}).value<uint64_t>(t.removeFile)                // mov rax,&remove
            .bytes({0xFF, 0x10})                                              // call [rax]
            .bytes({0x85, 0xC0})                                              // test eax,eax
            .jumpTo(Jcc::NE, at("free"))                                      // missing or in use
            .bytes({0xC6, 0x43, NODE_VALUE, 0x00})                            // mov byte ptr [rbx+20],0
            .bytes({0x41, 0xFF, 0xC6})                                        // inc r14d
            .label(at("free"))
            .bytes({0x48, 0x8D, 0x4C, 0x24, 0x20})                            // lea rcx,[rsp+20]
            .call(t.freeString)
            .label(at("next"))                                                // the in-order successor
            .bytes({0x48, 0x8B, 0x43, NODE_RIGHT})                            // mov rax,[rbx+10]
            .bytes({0x80, 0x78, NODE_ISNIL, 0x00})                            // cmp byte ptr [rax+19],0
            .jumpTo(Jcc::NE, at("up"))
            .bytes({0x48, 0x8B, 0xD8})                                        // mov rbx,rax
            .label(at("down"))
            .bytes({0x48, 0x8B, 0x03})                                        // mov rax,[rbx]
            .bytes({0x80, 0x78, NODE_ISNIL, 0x00})                            // cmp byte ptr [rax+19],0
            .jumpTo(Jcc::NE, at("loop"))
            .bytes({0x48, 0x8B, 0xD8})                                        // mov rbx,rax
            .jumpTo(Jcc::Always, at("down"))
            .label(at("up"))
            .bytes({0x48, 0x8B, 0x43, NODE_PARENT})                           // mov rax,[rbx+8]
            .label(at("climb"))
            .bytes({0x80, 0x78, NODE_ISNIL, 0x00})                            // cmp byte ptr [rax+19],0
            .jumpTo(Jcc::NE, at("climbed"))
            .bytes({0x48, 0x3B, 0x58, NODE_RIGHT})                            // cmp rbx,[rax+10]
            .jumpTo(Jcc::NE, at("climbed"))
            .bytes({0x48, 0x8B, 0xD8})                                        // mov rbx,rax
            .bytes({0x48, 0x8B, 0x40, NODE_PARENT})                           // mov rax,[rax+8]
            .jumpTo(Jcc::Always, at("climb"))
            .label(at("climbed"))
            .bytes({0x48, 0x8B, 0xD8})                                        // mov rbx,rax
            .jumpTo(Jcc::Always, at("loop"))
            .label(at("end"));
    }

    // At PlayerObject::pushButton+0x2D: rbx is the player, edi the button (1 = jump).
    static void buildJumpHack(CaveBuilder &cave, int32_t onGround)
    {
        cave.bytes({0x83, 0xFF, 0x01})                          // cmp edi,1
            .jumpTo(CaveBuilder::Jcc::NE, "original")
            .bytes({0xC6, 0x83}).value(onGround).bytes({0x01}) // mov byte ptr [rbx+onGround],1
            .label("original")
            .original()                                         // cmp byte ptr [rbx+...],0 - its jne follows
            .jmpBack();
    }
};
