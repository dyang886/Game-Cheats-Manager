using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using PugTool.Unity.Singleton;
using UnityEngine;

public class MainThreadDispatcher : MonoBehaviour
{
    private static MainThreadDispatcher instance;
    private readonly Queue<Action> actionQueue = new Queue<Action>();

    private void Awake()
    {
        if (instance != null)
        {
            Destroy(this);
            return;
        }

        instance = this;
        DontDestroyOnLoad(gameObject);
    }

    private void Update()
    {
        while (true)
        {
            Action action;
            lock (actionQueue)
            {
                if (actionQueue.Count == 0)
                    break;

                action = actionQueue.Dequeue();
            }

            try
            {
                action.Invoke();
            }
            catch (Exception ex)
            {
                GCMInjection.LogException("Queued action failed", ex);
            }
        }

        GCMInjection.Tick();
    }

    public static bool Enqueue(Action action)
    {
        if (instance == null)
            return false;

        lock (instance.actionQueue)
        {
            instance.actionQueue.Enqueue(action);
        }

        return true;
    }
}

public static class GCMInjection
{
    [DllImport("MonoBridge.dll")]
    private static extern void SendData(string message);

    [DllImport("MonoBridge.dll")]
    private static extern void SendResponse(string message);

    /// <summary>
    /// Reply for an apply action that succeeded. Anything else the trainer receives is a message to
    /// show the user, run through its own translation table - so the strings below are the keys.
    /// </summary>
    private const string ResponseOk = "OK";

    /// <summary>
    /// Every cheat that leans on the player's attribute pipeline registers at this priority.
    /// PlayerCore sorts its calculations by descending priority, so the lowest value runs last -
    /// which is what keeps these on top of the game's own passes, RealityPlayerAttrChange included
    /// (it claims -999999 to force the second run's 10 max HP).
    /// </summary>
    private const int AttrPriority = int.MinValue;

    /// <summary>
    /// Ceiling for the talisman slot count. CreateNotchSelection only sells eight upgrades over the
    /// flag's default, and both ChallengeSave and DebugHelper park the flag at 10, so 10 is the
    /// highest number the game itself ever writes.
    /// </summary>
    private const int TalismanSlotCap = 10;

    /// <summary>
    /// Poison_Debuff.AddPoison divides every tick it adds by Attr.debuffAttr.poisonResistance, so a
    /// large enough divisor keeps the meter at zero and DeadByPoison out of reach.
    /// </summary>
    private const float PoisonImmunityResistance = 1E+09f;

    private static bool godModeEnabled;
    private static LevelManager godModeGuardedLevelManager;
    private static bool godModeOriginalEnableHurt;
    private static bool godModeOriginalEnableDead;
    private static bool godModeOriginalEnableBeCatched;

    private static bool infiniteHealthEnabled;
    private static PlayerAttrCalculation infiniteHealthCalculation;

    private static bool moveSpeedEnabled;
    private static float moveSpeed = 7f;
    private static PlayerAttrCalculation moveSpeedCalculation;

    private static bool damageMultiplierEnabled;
    private static float damageMultiplier = 1f;
    private static PlayerAttrCalculation damageMultiplierCalculation;

    private static bool talismanSlotsEnabled;
    private static int talismanSlots = TalismanSlotCap;
    private static GameFlagInt talismanSlotsGuardedFlag;
    private static int talismanSlotsOriginal;

    private static void Log(string message)
    {
        SendData("[GCMInjection] " + message);
    }

    public static void LogException(string context, Exception ex)
    {
        Log(context + ": " + ex.GetType().Name + ": " + ex.Message + "\n" + ex.StackTrace);
    }

    public static void Initialize()
    {
        if (GameObject.Find("GCM MainThreadDispatcher") != null)
        {
            Log("Rubinite injection already initialized.");
            return;
        }

        GameObject go = new GameObject("GCM MainThreadDispatcher");
        go.AddComponent<MainThreadDispatcher>();
        Log("Initialized Rubinite injection.");
    }

    public static void Tick()
    {
        try
        {
            if (godModeEnabled)
                ApplyGodMode();

            if (infiniteHealthEnabled)
                ApplyInfiniteHealth();

            if (moveSpeedEnabled)
                KeepAttrCalculation(moveSpeedCalculation);

            if (damageMultiplierEnabled)
                KeepAttrCalculation(damageMultiplierCalculation);

            if (talismanSlotsEnabled)
                ApplyTalismanSlots();
        }
        catch (Exception ex)
        {
            Log("Tick failed: " + ex.Message);
        }
    }

    // ============================================================
    // Trainer entry points
    // ============================================================

    public static void ToggleGodMode(bool enabled)
    {
        RunCheat(delegate
        {
            godModeEnabled = enabled;

            if (enabled)
                ApplyGodMode();
            else
                RestoreGodMode();

            Log("God Mode " + (enabled ? "enabled" : "disabled") + ".");
            return null;
        });
    }

    public static void ToggleInfiniteHealth(bool enabled)
    {
        RunCheat(delegate
        {
            infiniteHealthEnabled = enabled;

            if (enabled)
            {
                if (infiniteHealthCalculation == null)
                    infiniteHealthCalculation = MakeAttrCalculation("GCM_InfiniteHealth", InfiniteHealthCalculation);

                ApplyInfiniteHealth();
            }
            else
            {
                RemoveAttrCalculation(infiniteHealthCalculation);
            }

            Log("Infinite Health " + (enabled ? "enabled" : "disabled") + ".");
            return null;
        });
    }

    public static void SetMoveSpeed(bool enabled, float value)
    {
        RunCheat(delegate
        {
            moveSpeedEnabled = enabled;
            moveSpeed = Mathf.Clamp(SanitizeFloat(value, 7f), 0.1f, 1000f);

            if (enabled)
            {
                if (moveSpeedCalculation == null)
                    moveSpeedCalculation = MakeAttrCalculation("GCM_MoveSpeed", MoveSpeedCalculation);

                KeepAttrCalculation(moveSpeedCalculation);
                RefreshAttr();
            }
            else
            {
                RemoveAttrCalculation(moveSpeedCalculation);
            }

            Log("Move Speed " + (enabled ? "set to " + moveSpeed : "restored") + ".");
            return null;
        });
    }

    public static void SetDamageMultiplier(bool enabled, float value)
    {
        RunCheat(delegate
        {
            damageMultiplierEnabled = enabled;
            damageMultiplier = Mathf.Clamp(SanitizeFloat(value, 1f), 0f, 10000f);

            if (enabled)
            {
                if (damageMultiplierCalculation == null)
                    damageMultiplierCalculation = MakeAttrCalculation("GCM_DamageMultiplier", DamageMultiplierCalculation);

                KeepAttrCalculation(damageMultiplierCalculation);
                RefreshAttr();
            }
            else
            {
                RemoveAttrCalculation(damageMultiplierCalculation);
            }

            Log("Damage Multiplier " + (enabled ? "set to " + damageMultiplier : "restored") + ".");
            return null;
        });
    }

    public static void SetTalismanSlots(bool enabled, int value)
    {
        RunCheat(delegate
        {
            talismanSlotsEnabled = enabled;
            talismanSlots = value;

            if (enabled)
                ApplyTalismanSlots();
            else
                RestoreTalismanSlots();

            Log("Talisman Slots " + (enabled ? "set to " + talismanSlots : "restored") + ".");
            return null;
        });
    }

    /// <summary>Overwrites the Blood Dust the player is carrying.</summary>
    public static void SetBloodDust(int value)
    {
        RunCheat(delegate
        {
            GamePlayCore core = Core;
            if (core == null || core.dust == null)
                return "Please load a save first.";

            core.dust.SetValue(Math.Max(0, value), "GCMInjection.SetBloodDust");
            Log("Set Blood Dust to " + core.dust.GetValue() + ".");
            return null;
        });
    }

    /// <summary>
    /// Overwrites the Impure Crystals the player is carrying. The field keeps the crystals' in-world
    /// name rather than the one the shop shows, but this is the currency CreateNotchSelection and
    /// CreateHPBottleSelection charge against - talisman notches and health bottle upgrades.
    /// </summary>
    public static void SetImpureCrystal(int value)
    {
        RunCheat(delegate
        {
            GamePlayCore core = Core;
            if (core == null || core.PurebloodCrystalShard_Count == null)
                return "Please load a save first.";

            core.PurebloodCrystalShard_Count.SetValue(Math.Max(0, value), "GCMInjection.SetImpureCrystal");
            Log("Set Impure Crystal to " + core.PurebloodCrystalShard_Count.GetValue() + ".");
            return null;
        });
    }

    // ============================================================
    // Cheat implementations
    // ============================================================

    /// <summary>
    /// Shuts the player's three vulnerability gates. Every way the game can hurt, kill or grab the
    /// player runs through PlayerCore.enableHurt / enableDead / enableBeCatched, and each is an AND
    /// of a field on the player and a GameObject the level manager owns - so closing both halves of
    /// all three leaves no path open:
    ///
    ///   - BeHurt returns immediately without enableHurt, which also covers the one-shot second
    ///     phase, where RealityPlayerAttrChange drops max HP to 10 but leaves this path alone.
    ///   - HurtDirectlyByFire wants both gates before it subtracts.
    ///   - Poison_Debuff.AddPoison and Combustion_Debuff.AddCombusion bail on either gate, so the
    ///     meters never fill and DeadByPoison / HurtByCombustion are never reached.
    ///   - Dead() itself checks enableDead, which catches the scripted deaths - BeCatchedAction.Exit
    ///     being the notable one.
    ///   - GrabAttackCollider.Update and GrabProjectile both test enableBeCatched before they reach
    ///     SetBeCatchStart, so the grab never takes hold in the first place.
    ///
    /// The player's own fields get rewritten constantly (dashing, striking and the end of a hurt
    /// animation all set m_EnableHurt), hence the reassert every frame.
    /// </summary>
    private static void ApplyGodMode()
    {
        GamePlayCore core = Core;
        if (core == null)
            return;

        LevelManager levelManager = core.levelManager;
        if (levelManager != null)
        {
            // Remember what the level started with before the first override lands on it, so a
            // tutorial or cutscene that had deliberately closed a gate gets it back on the way out.
            if (godModeGuardedLevelManager != levelManager)
            {
                godModeGuardedLevelManager = levelManager;
                godModeOriginalEnableHurt = levelManager.enableHurt.activeSelf;
                godModeOriginalEnableDead = levelManager.enableDead.activeSelf;
                godModeOriginalEnableBeCatched = levelManager.enableBeCatched.activeSelf;
            }

            if (levelManager.enableHurt.activeSelf)
                levelManager.enableHurt.SetActive(false);

            if (levelManager.enableDead.activeSelf)
                levelManager.enableDead.SetActive(false);

            if (levelManager.enableBeCatched.activeSelf)
                levelManager.enableBeCatched.SetActive(false);
        }

        PlayerCore player = core.player;
        if (player == null)
            return;

        player.m_EnableHurt = false;
        player.m_EnableDead = false;
        player.m_EnableBeCatched = false;

        // The gate above only stops a grab from starting - one already holding the player runs to
        // its own end, and GrabProjectile starts its hold with autoRelease off, so that end never
        // comes on its own. SetBeCatchEnd is the game's own release signal and drives the normal
        // get-up animation out of it.
        if (player.state == PlayerCore.State.BeCatched)
            player.SetBeCatchEnd();
    }

    private static void RestoreGodMode()
    {
        GamePlayCore core = Core;
        if (core == null)
        {
            godModeGuardedLevelManager = null;
            return;
        }

        LevelManager levelManager = core.levelManager;
        if (levelManager != null)
        {
            bool sameLevel = godModeGuardedLevelManager == levelManager;
            levelManager.enableHurt.SetActive(!sameLevel || godModeOriginalEnableHurt);
            levelManager.enableDead.SetActive(!sameLevel || godModeOriginalEnableDead);
            levelManager.enableBeCatched.SetActive(!sameLevel || godModeOriginalEnableBeCatched);
        }

        godModeGuardedLevelManager = null;

        // The player's own flags are per-action state rather than a setting, and every action that
        // clears them sets them again when it exits - so handing back the permissive value is both
        // the safe direction and the one the next transition would have written anyway.
        PlayerCore player = core.player;
        if (player != null)
        {
            player.m_EnableHurt = true;
            player.m_EnableDead = true;
            player.m_EnableBeCatched = true;
        }
    }

    /// <summary>
    /// Leaves the player hittable - the hit reaction, knockback and i-frames all still play - but
    /// keeps the health bar full. The attribute pass zeroes every multiplier the damage maths runs
    /// through, and the top-up afterwards repairs anything that reached HP before the toggle went on.
    ///
    /// Health is deliberately pinned to Attr.maxHP rather than a fixed number: the second phase
    /// hides the bar and RealityPlayerAttrChange rewrites max HP to 10, so reading the cap back out
    /// of the attributes is what makes one implementation cover both phases. Holding HP at the cap
    /// is also what keeps the scripted "HP &lt;= 0" deaths - BeCatchedAction.Exit above all - out of
    /// reach without touching the death gate God Mode owns.
    /// </summary>
    private static void ApplyInfiniteHealth()
    {
        KeepAttrCalculation(infiniteHealthCalculation);

        PlayerCore player = Player;
        if (player == null || player.Attr == null)
            return;

        if (player.HP < player.Attr.maxHP)
            player.HP = player.Attr.maxHP;
    }

    /// <summary>
    /// hurtScale is the only multiplier on the two places that subtract from player HP - BeHurt and
    /// HurtDirectlyByFire - so zeroing it settles the direct damage. The debuff resistances close
    /// the two indirect paths: Combustion_Debuff.Hurt deals 2 * (1 - combustionResistance), and
    /// poison never fills its meter once the divisor is large enough.
    /// </summary>
    private static PlayerAttr InfiniteHealthCalculation(PlayerAttr attrIn)
    {
        attrIn.hurtScale = 0f;

        if (attrIn.debuffAttr != null)
        {
            attrIn.debuffAttr.combustionResistance = 1f;
            attrIn.debuffAttr.poisonResistance = PoisonImmunityResistance;
        }

        return attrIn;
    }

    private static PlayerAttr MoveSpeedCalculation(PlayerAttr attrIn)
    {
        attrIn.moveSpeed = moveSpeed;
        return attrIn;
    }

    /// <summary>
    /// Scales the attack power the player's damage is built from, which is what makes one multiplier
    /// cover every weapon at once. Each damage route reads its own base out of PlayerAttr and then
    /// applies its own bonuses on top - slash and special attacks through
    /// PlayerSlashDamage.SlashHurtEnemy, shots through ShootAction and
    /// PlayerProjectileDamage.ProjectileHurtEnemy, critical strikes through CalculateATKOfStrike,
    /// and the boss scripts that subtract from HP directly off Attr.shootATK - so scaling the bases
    /// hands every one of them the same factor while leaving the mark, talisman and story-mode
    /// bonuses to compose on top as usual.
    ///
    /// Deflected enemy projectiles are the one thing this leaves alone: ProjectileCore.Rebound and
    /// ThrowingKnife.Rebound hardcode their damage instead of reading it off the player.
    /// </summary>
    private static PlayerAttr DamageMultiplierCalculation(PlayerAttr attrIn)
    {
        attrIn.slash_0_ATK *= damageMultiplier;
        attrIn.slash_3_ATK *= damageMultiplier;
        attrIn.specailAtk *= damageMultiplier;

        attrIn.shootATK *= damageMultiplier;
        attrIn.shootATK_WhenCharged *= damageMultiplier;

        attrIn.atk1_OfCriticalStrike *= damageMultiplier;
        attrIn.atk2_OfCriticalStrike *= damageMultiplier;
        attrIn.atk3_OfCriticalStrike *= damageMultiplier;
        attrIn.atk4_OfCriticalStrike *= damageMultiplier;
        attrIn.atk5_OfCriticalStrike *= damageMultiplier;
        attrIn.extraStrikeAddition_ATK *= damageMultiplier;

        return attrIn;
    }

    /// <summary>
    /// Holds the talisman notch flag at the requested count. The value is written straight into the
    /// flag's data rather than through VariableInt.SetValue so it stays out of the dirty list, and
    /// the original is handed back the moment the toggle goes off - the trainer also runs that on
    /// its way out, so nothing is left behind in the save.
    /// </summary>
    private static void ApplyTalismanSlots()
    {
        GamePlayCore core = Core;
        if (core == null || core.talismanNotchesMax == null)
            return;

        GameFlagInt flag = core.talismanNotchesMax.flag;
        if (flag == null)
            return;

        if (talismanSlotsGuardedFlag != flag)
        {
            talismanSlotsGuardedFlag = flag;
            talismanSlotsOriginal = flag.data.currentValue;
        }

        int target = ClampTalismanSlots(talismanSlots);
        if (flag.data.currentValue != target)
            flag.data.currentValue = target;
    }

    private static void RestoreTalismanSlots()
    {
        if (talismanSlotsGuardedFlag != null)
        {
            talismanSlotsGuardedFlag.data.currentValue = talismanSlotsOriginal;
            talismanSlotsGuardedFlag = null;
        }
    }

    /// <summary>
    /// Keeps the notch count inside what the talisman menu can draw. Both TalismanCollectMenu.Process
    /// and UpdateNotch walk their animator and icon arrays with the flag as the loop bound rather
    /// than the array length, so a count past the shortest of them throws the moment the menu opens.
    /// </summary>
    private static int ClampTalismanSlots(int value)
    {
        int max = TalismanSlotCap;

        MenuCore menuCore = MenuCore.Instance;
        TalismanCollectMenu menu = (menuCore != null && menuCore.collectMenu != null)
            ? menuCore.collectMenu.talismanCollectMenu
            : null;

        if (menu != null)
        {
            max = Math.Min(max, Length(menu.notches));
            max = Math.Min(max, Length(menu.notchesEquip));
            max = Math.Min(max, Length(menu.notchesEquip_Ani));
            max = Math.Min(max, Length(menu.notchesEquip_Ani_Excess));
        }

        return Mathf.Clamp(value, 1, Math.Max(1, max));
    }

    private static int Length(Array array)
    {
        return (array == null) ? 0 : array.Length;
    }

    // ============================================================
    // Attribute pipeline helpers
    // ============================================================

    private static PlayerAttrCalculation MakeAttrCalculation(string name, PlayerAttrCalculation.Del_Calculation callBack)
    {
        PlayerAttrCalculation calculation = new PlayerAttrCalculation();
        calculation.name = name;
        calculation.callBack = callBack;
        calculation.priority = AttrPriority;
        return calculation;
    }

    /// <summary>
    /// Makes sure a calculation is registered with the player currently in play. Loading a save
    /// builds a fresh GamePlayCore and PlayerCore.SetUp starts the calculation list over, so a
    /// toggle left on across a load has to notice its entry is gone and add it again.
    /// </summary>
    private static void KeepAttrCalculation(PlayerAttrCalculation calculation)
    {
        if (calculation == null)
            return;

        PlayerCore player = Player;
        if (player == null || player.Attr == null)
            return;

        if (!player.AttrContains(calculation))
            player.AddAttrCalculation(calculation);
    }

    private static void RemoveAttrCalculation(PlayerAttrCalculation calculation)
    {
        if (calculation == null)
            return;

        PlayerCore player = Player;
        if (player == null || player.Attr == null)
            return;

        player.RemoveAttrCalculation(calculation);
    }

    /// <summary>Recomputes the attributes after a value changed without the calculation list moving.</summary>
    private static void RefreshAttr()
    {
        PlayerCore player = Player;
        if (player != null && player.Attr != null)
            player.UpdateAttr();
    }

    // ============================================================
    // Helpers
    // ============================================================

    /// <summary>
    /// GamePlayCore.Instance walks through the ApplicationCore singleton, which hands back null once
    /// the game is quitting or the singleton has been torn down - so go the same way and stop at the
    /// first null instead of letting that throw once a frame.
    /// </summary>
    private static GamePlayCore Core
    {
        get
        {
            ApplicationCore app = SingletonPrefab<ApplicationCore>.Instance;
            return (app == null) ? null : app.GamePlayCore;
        }
    }

    private static PlayerCore Player
    {
        get
        {
            GamePlayCore core = Core;
            return (core == null) ? null : core.player;
        }
    }

    /// <summary>
    /// Runs a cheat on the game's main thread and answers the trainer exactly once. The cheat
    /// returns null to report success and a message to refuse; the trainer looks that message up in
    /// its own translation table, so it has to match a key in translations.json.
    ///
    /// Toggles and apply actions both come through here, and the trainer blocks on the reply either
    /// way - so every path out, an unhandled exception included, has to answer.
    /// </summary>
    private static void RunCheat(Func<string> cheat)
    {
        bool queued = MainThreadDispatcher.Enqueue(delegate
        {
            string message;

            try
            {
                message = cheat.Invoke();
            }
            catch (Exception ex)
            {
                LogException("Cheat failed", ex);
                message = ex.Message;
            }

            if (!string.IsNullOrEmpty(message))
                Log("[!] " + message);

            SendResponse(string.IsNullOrEmpty(message) ? ResponseOk : message);
        });

        // Nothing will ever run the cheat, so answer here rather than leaving the trainer to sit on
        // the response channel until it times out.
        if (!queued)
        {
            Log("[!] The dispatcher is not running; the cheat was dropped.");
            SendResponse("Please restart the game and the trainer.");
        }
    }

    private static float SanitizeFloat(float value, float fallback)
    {
        if (float.IsNaN(value) || float.IsInfinity(value))
            return fallback;

        return value;
    }
}
