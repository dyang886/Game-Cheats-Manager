using System;
using System.Collections;
using System.Collections.Generic;
using System.Reflection;
using System.Runtime.InteropServices;
using PunchForEscape;
using UnityEngine;

/// <summary>
/// Gives the payload a frame of its own: Tick() runs on the game's main thread once per frame.
/// </summary>
public class MainThreadDispatcher : MonoBehaviour
{
    private static MainThreadDispatcher instance;

    /// <summary>Plain flag rather than a Unity null check, so the trainer's thread can read it.</summary>
    public static bool Alive { get; private set; }

    private void Awake()
    {
        if (instance != null)
        {
            Destroy(this);
            return;
        }

        instance = this;
        Alive = true;
        DontDestroyOnLoad(gameObject);
    }

    private void OnDestroy()
    {
        if (instance == this)
        {
            instance = null;
            Alive = false;
        }
    }

    private void Update()
    {
        GCMInjection.Tick();
    }

    private void LateUpdate()
    {
        GCMInjection.LateTick();
    }
}

public static class GCMInjection
{
    [DllImport("MonoBridge.dll")]
    private static extern void SendData(string message);

    [DllImport("MonoBridge.dll")]
    private static extern void SendResponse(string message);

    /// <summary>Success. Anything else is shown to the user under the trainer's generic line.</summary>
    private const string ResponseOk = "OK";

    // Wallet.Money has a private setter. Wallet.Set() is the public way in, but it also empties the
    // income-per-second window the HUD reads, which holding it every frame would keep at zero.
    private static readonly MethodInfo moneySetter =
        typeof(Wallet).GetProperty("Money", BindingFlags.Public | BindingFlags.Static).GetSetMethod(true);

    // The moment the next charged punch is allowed, in unscaled time.
    private static readonly FieldInfo chargeReadyField =
        typeof(PunchRig).GetField("chargeReady", BindingFlags.NonPublic | BindingFlags.Instance);

    // The cubes a broken wall has queued, waiting for DebrisPool's next FixedUpdate to create them.
    private static readonly FieldInfo debrisPendingField =
        typeof(DebrisPool).GetField("pending", BindingFlags.NonPublic | BindingFlags.Instance);

    // ------------------------------------------------------------------
    // What the user asked for. Written by the entry points on the trainer's thread, read by Tick() on
    // the game's - one field per cheat, so Tick() never sees half of a change.
    // ------------------------------------------------------------------

    private const int Off = -1;

    private static int coinsTarget = Off;
    private static int dynamiteTarget = Off;
    private static int c4Target = Off;
    private static int vacuumCapacityTarget = Off;
    private static bool clearVacuumRequested;
    private static bool noDebris;
    private static bool noChargeCooldown;

    /// <summary>1 is off: a multiplier of one changes nothing.</summary>
    private static float shredderMultiplierRequest = 1f;

    // ------------------------------------------------------------------
    // What Tick() has actually done. Main thread only.
    // ------------------------------------------------------------------

    /// <summary>Whether the vacuum's capacity is ours, so letting go puts the upgrade's back once.</summary>
    private static bool vacuumCapacityHeld;

    // DebrisPool.pending for the pool it was read from; a new scene brings a new pool.
    private static DebrisPool debrisPool;
    private static IList debrisPending;

    private static float shredderMultiplierHandled = 1f;
    private static float shredderMultiplierActive = 1f;

    // The game's own per-tier cube values, kept while the multiplier is applied over them. Null means
    // the table currently holds the game's values, which is what stops a second apply from taking an
    // already-scaled table as the original.
    private static float[] baseLayerValues;

    // Every treasure the multiplier has scaled, with the value it had before.
    private static readonly Dictionary<Valuable, int> baseTreasureValues = new Dictionary<Valuable, int>();
    private static readonly List<Valuable> deadTreasures = new List<Valuable>();

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
            Log("Punch The Walls injection already initialized.");
            return;
        }

        GameObject go = new GameObject("GCM MainThreadDispatcher");
        go.AddComponent<MainThreadDispatcher>();
        Log("Initialized Punch The Walls injection.");
    }

    /// <summary>Once per frame, on the game's main thread: where every cheat is actually applied.</summary>
    public static void Tick()
    {
        try
        {
            // A one-off, so it is consumed whether or not it can still be carried out - never left
            // pending to empty the tank of whatever save is loaded next.
            if (clearVacuumRequested)
            {
                clearVacuumRequested = false;
                ClearVacuumTank();
            }

            // A new game and a loaded save both start by resetting the wallet and the item counts, so
            // these hold only inside a running game and pick up again right after it has loaded.
            if (GameSession.Playing)
            {
                HoldCoins(coinsTarget);

                bool countsChanged = HoldCount(ShopItemId.Dynamite, dynamiteTarget);
                countsChanged |= HoldCount(ShopItemId.C4, c4Target);

                // Progress.Changed is what the shop and the hotbar listen to.
                if (countsChanged)
                    Progress.NotifyChanged();

                // After the counts: NotifyChanged() re-applies the upgrades, vacuum capacity included.
                HoldVacuumCapacity(vacuumCapacityTarget);
            }

            if (noChargeCooldown)
                ClearChargeCooldown();

            ApplyShredderMultiplier();
        }
        catch (Exception ex)
        {
            Log("Tick failed: " + ex.GetType().Name + ": " + ex.Message);
        }
    }

    /// <summary>
    /// Once per frame after every Update(): the last point before the next FixedUpdate() at which
    /// what this frame did can still be undone.
    /// </summary>
    public static void LateTick()
    {
        try
        {
            if (noDebris)
                DropWallDebris();
        }
        catch (Exception ex)
        {
            Log("LateTick failed: " + ex.GetType().Name + ": " + ex.Message);
        }
    }

    // ============================================================
    // Trainer entry points
    // ============================================================

    public static void SetCoins(bool enabled, int value)
    {
        RunCheat(delegate
        {
            coinsTarget = enabled ? NotNegative(value) : Off;
            return null;
        });
    }

    public static void SetDynamite(bool enabled, int value)
    {
        RunCheat(delegate
        {
            dynamiteTarget = enabled ? NotNegative(value) : Off;
            return null;
        });
    }

    public static void SetC4(bool enabled, int value)
    {
        RunCheat(delegate
        {
            c4Target = enabled ? NotNegative(value) : Off;
            return null;
        });
    }

    public static void SetVacuumCapacity(bool enabled, int value)
    {
        RunCheat(delegate
        {
            vacuumCapacityTarget = enabled ? ((value < 1) ? 1 : value) : Off;
            return null;
        });
    }

    public static void ClearVacuum()
    {
        RunCheat(delegate
        {
            // Both checks are safe from this thread: GameSession.Playing is WorldBuilder.I plus a
            // property, and Unity's null test on a MonoBehaviour only compares its cached pointer.
            if (!GameSession.Playing || Vacuum.I == null)
                return "Please load a save first.";

            clearVacuumRequested = true;
            return null;
        });
    }

    public static void ToggleNoDebris(bool enabled)
    {
        RunCheat(delegate
        {
            noDebris = enabled;
            return null;
        });
    }

    public static void ToggleNoChargeCooldown(bool enabled)
    {
        RunCheat(delegate
        {
            noChargeCooldown = enabled;
            return null;
        });
    }

    public static void SetShredderPayoutsMultiplier(bool enabled, float value)
    {
        RunCheat(delegate
        {
            shredderMultiplierRequest = (enabled && value > 0f) ? value : 1f;
            return null;
        });
    }

    // ============================================================
    // Cheats
    // ============================================================

    private static void HoldCoins(int coins)
    {
        if (coins != Off && Wallet.Money != coins)
            moneySetter.Invoke(null, new object[] { coins });
    }

    /// <summary>
    /// The count behind the hotbar slot, the "xN" beside it and PunchRig.Has(), which is what lets the
    /// tool be equipped at all. Dynamite.TryPlace() and C4.TryPlace() spend from it, so holding it is
    /// what makes them free.
    /// </summary>
    private static bool HoldCount(ShopItemId id, int count)
    {
        if (count == Off || Progress.Count(id) == count)
            return false;

        Progress.SetCount(id, count);
        return true;
    }

    /// <summary>
    /// Vacuum.capacity is what Suck(), Collect() and the tank display all read. Progress.Apply() puts
    /// the upgrade's value back on every upgrade, purchase and item change, so it is held. It goes in
    /// through the game's own SetStats(), as a capacity upgrade does - and the cone, which SetStats()
    /// rebuilds only when the range changes, is left alone by passing the current range.
    ///
    /// Suction follows the game's own rule for a tank, which is that it fills in five seconds
    /// (flowRate = capacity / 5), but without the 300 a second SetStats() caps it at, so it keeps
    /// rising with the capacity. flowRate paces both the grab and the capture in Suck(); past a few
    /// thousand a second the limit becomes how many cubes are in the cone, not the rate.
    ///
    /// Letting go re-runs Progress.Apply() for the capacity and suction the upgrades actually give.
    /// </summary>
    private static void HoldVacuumCapacity(int capacity)
    {
        if (capacity == Off)
        {
            if (vacuumCapacityHeld)
            {
                vacuumCapacityHeld = false;
                Progress.Apply();
            }
            return;
        }

        vacuumCapacityHeld = true;

        Vacuum vacuum = Vacuum.I;
        if (vacuum == null)
            return;

        if (vacuum.capacity != capacity)
            vacuum.SetStats(vacuum.range, capacity);

        // Checked on its own: when the upgrades already give this exact capacity, SetStats() is not
        // re-run above, but its capped flow rate is still in place.
        float flow = capacity / 5f;
        if (vacuum.flowRate != flow)
            vacuum.flowRate = flow;
    }

    /// <summary>
    /// The tank is StoredTiers - one entry per cube, which is all Stored, Full, the display, Eject()
    /// and the save read. Nothing else counts it, so emptying the list empties the tank. Cubes still
    /// flying into the nozzle land through Collect() afterwards, as they would have anyway.
    /// </summary>
    private static void ClearVacuumTank()
    {
        Vacuum vacuum = Vacuum.I;
        if (GameSession.Playing && vacuum != null)
            vacuum.StoredTiers.Clear();
    }

    /// <summary>
    /// Every cube a wall lets go of - punched, charged-punched, blown by dynamite or C4, or dropped
    /// once nothing holds it up - goes through DebrisPool.SpawnNextStep(), which only queues it in
    /// pending; DebrisPool.FixedUpdate() creates the queued cubes at the next physics step. Nothing
    /// else queues there (the vacuum blows its cubes out through Spawn() directly), and every wall
    /// break happens in an Update() - PunchRig's, through the hand's impact callback, Dynamite's and
    /// C4's - so emptying the queue here, after all of them, drops exactly the wall's cubes before any
    /// exists. The wall still breaks and counts as broken; cubes already lying around are left alone.
    /// </summary>
    private static void DropWallDebris()
    {
        DebrisPool pool = DebrisPool.I;
        if (pool == null)
            return;

        if ((object)pool != (object)debrisPool)
        {
            debrisPool = pool;
            debrisPending = (IList)debrisPendingField.GetValue(pool);
        }

        if (debrisPending != null && debrisPending.Count > 0)
            debrisPending.Clear();
    }

    /// <summary>
    /// PunchRig.ChargedPunch() pushes chargeReady 50 seconds ahead and Update() refuses to start a new
    /// charge until unscaled time passes it. Pulling it back to now is the whole cheat; the HUD dial
    /// reads the same field through CooldownLeft, so it shows ready too.
    /// </summary>
    private static void ClearChargeCooldown()
    {
        PunchRig rig = PunchRig.I;
        if (rig == null || rig.CooldownLeft <= 0f)
            return;

        chargeReadyField.SetValue(rig, 0f);
    }

    /// <summary>
    /// Brings the shredder's payouts in line with the latest request: back to the game's values
    /// whenever the request changes, then scaled to the new one. Each request is tried once, so a
    /// failure is logged rather than repeated every frame.
    /// </summary>
    private static void ApplyShredderMultiplier()
    {
        float wanted = shredderMultiplierRequest;
        if (wanted != shredderMultiplierHandled)
        {
            shredderMultiplierHandled = wanted;
            shredderMultiplierActive = 1f;
            RestoreLayerValues();
            RestoreTreasureValues();

            if (wanted != 1f)
            {
                try
                {
                    ScaleLayerValues(wanted);
                    shredderMultiplierActive = wanted;
                }
                catch (Exception ex)
                {
                    RestoreLayerValues();
                    LogException("Shredder Payouts Multiplier failed", ex);
                }
            }
        }

        if (shredderMultiplierActive != 1f)
            ScaleTreasureValues(shredderMultiplierActive);
    }

    /// <summary>
    /// Shredder.Pay() credits valuePerCube * VoxelWall.TierValue(tier) * 0.65 per cube, and TierValue
    /// is a lookup into this table and nothing else - nothing but the shredder reads it, so scaling it
    /// scales exactly the shredder's cube income, fractions included.
    /// </summary>
    private static void ScaleLayerValues(float multiplier)
    {
        VoxelWall.Layer[] layers = VoxelWall.Layers;
        if (layers == null)
            return;

        if (baseLayerValues == null)
        {
            baseLayerValues = new float[layers.Length];
            for (int i = 0; i < layers.Length; i++)
                baseLayerValues[i] = (layers[i] != null) ? layers[i].value : 0f;
        }

        for (int i = 0; i < layers.Length; i++)
        {
            if (layers[i] != null)
                layers[i].value = baseLayerValues[i] * multiplier;
        }
    }

    private static void RestoreLayerValues()
    {
        if (baseLayerValues == null)
            return;

        VoxelWall.Layer[] layers = VoxelWall.Layers;
        if (layers != null)
        {
            for (int i = 0; i < layers.Length && i < baseLayerValues.Length; i++)
            {
                if (layers[i] != null)
                    layers[i].value = baseLayerValues[i];
            }
        }

        baseLayerValues = null;
    }

    /// <summary>
    /// Shredder.PayTreasure() credits Valuable.value as-is. That field is read nowhere else and is
    /// never saved - a loaded world rebuilds it from the treasure's definition - so a treasure is
    /// scaled once when first seen, and new ones are picked up as the world spawns them.
    /// </summary>
    private static void ScaleTreasureValues(float multiplier)
    {
        List<Valuable> all = Valuable.All;
        if (all == null)
            return;

        for (int i = 0; i < all.Count; i++)
        {
            Valuable treasure = all[i];
            if (treasure == null || baseTreasureValues.ContainsKey(treasure))
                continue;

            baseTreasureValues[treasure] = treasure.value;

            // Plain arithmetic on purpose: the game's mscorlib is stripped, and Math.Min(double,
            // double) is one of the overloads that did not survive.
            double scaled = treasure.value * (double)multiplier;
            treasure.value = (scaled <= 0.0) ? 0 : (scaled >= int.MaxValue) ? int.MaxValue : (int)(scaled + 0.5);
        }

        // Shredded and unloaded treasures are destroyed; drop them so the map does not grow forever.
        if (baseTreasureValues.Count > all.Count)
        {
            deadTreasures.Clear();
            foreach (Valuable treasure in baseTreasureValues.Keys)
            {
                if (treasure == null)
                    deadTreasures.Add(treasure);
            }

            for (int i = 0; i < deadTreasures.Count; i++)
                baseTreasureValues.Remove(deadTreasures[i]);

            deadTreasures.Clear();
        }
    }

    private static void RestoreTreasureValues()
    {
        foreach (KeyValuePair<Valuable, int> entry in baseTreasureValues)
        {
            if (entry.Key != null)
                entry.Key.value = entry.Value;
        }

        baseTreasureValues.Clear();
    }

    // ============================================================
    // Helpers
    // ============================================================

    private static int NotNegative(int value)
    {
        return (value < 0) ? 0 : value;
    }

    /// <summary>
    /// Records a cheat and answers the trainer exactly once - null for success, or the reason it
    /// refused. The trainer blocks on that reply, so every path must answer.
    ///
    /// Unlike the reference payload, this answers on the trainer's own thread instead of queueing for
    /// the game's main thread. This game stops running frames whenever it loses focus - Unity's "Run
    /// In Background" is off, and its setter is stripped, so it cannot be switched on - and the
    /// trainer always has focus when it is clicked. A queued cheat therefore sat unanswered until the
    /// user went back to the game. So an entry point only writes the one field that holds the
    /// request, which is safe from any thread, and Tick() applies it on the main thread the next time
    /// the game runs a frame - which, with the game in its own pause menu, is when it matters anyway.
    /// </summary>
    private static void RunCheat(Func<string> cheat)
    {
        string message;

        try
        {
            message = MainThreadDispatcher.Alive ? cheat.Invoke() : "Please restart the game and the trainer.";
        }
        // The trainer shows this verbatim under its generic line, so it carries the exception type
        // too - "Object reference not set to an instance of an object." alone says little.
        catch (Exception ex)
        {
            LogException("Cheat failed", ex);
            message = ex.GetType().Name + ": " + ex.Message;
        }

        if (!string.IsNullOrEmpty(message))
            Log("[!] " + message);

        SendResponse(string.IsNullOrEmpty(message) ? ResponseOk : message);
    }
}
