using System;
using System.Collections.Generic;
using System.Reflection;
using System.Runtime.InteropServices;
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

    private void LateUpdate()
    {
        GCMInjection.LateTick();
    }

    public static void Enqueue(Action action)
    {
        if (instance == null)
            return;

        lock (instance.actionQueue)
        {
            instance.actionQueue.Enqueue(action);
        }
    }
}

public static class GCMInjection
{
    [DllImport("MonoBridge.dll")]
    private static extern void SendData(string message);

    private const BindingFlags PrivateInstance = BindingFlags.Instance | BindingFlags.NonPublic;
    private const BindingFlags PrivateStatic = BindingFlags.Static | BindingFlags.NonPublic;
    private static readonly FieldInfo DestroyableHpCurrField =
        typeof(Destroyable).GetField("<HpCurr>k__BackingField", PrivateInstance);
    private static readonly FieldInfo DestroyableHpChangedField =
        typeof(Destroyable).GetField("HpChanged", PrivateInstance);
    private static readonly FieldInfo PlayerEnergyCurrField =
        typeof(PlayerEnergy).GetField("curr", PrivateInstance);
    private static readonly FieldInfo PlayerEnergyRechargedField =
        typeof(PlayerEnergy).GetField("Recharged", PrivateInstance);
    private static readonly FieldInfo PlayerEnergyRechargedPartiallyField =
        typeof(PlayerEnergy).GetField("RechargedPartially", PrivateInstance);
    private static readonly FieldInfo PlayerEnergyHasGoneBelowThresholdField =
        typeof(PlayerEnergy).GetField("<HasGoneBellowTreshold>k__BackingField", PrivateStatic);
    private static readonly PropertyInfo PlayerEnergyMaxProperty =
        typeof(PlayerEnergy).GetProperty("max", PrivateInstance);
    private static readonly FieldInfo PlayerWeaponFirerateDelayField =
        typeof(PlayerWeapon).GetField("firerateDelay", PrivateInstance);
    private static readonly FieldInfo PlayerWeaponBulletDmgField =
        typeof(PlayerWeapon).GetField("bulletDmg", PrivateInstance);
    private static readonly FieldInfo PlayerWeaponBulletSuperDmgField =
        typeof(PlayerWeapon).GetField("bulletSuperDmg", PrivateInstance);
    private static readonly FieldInfo PlayerWeaponBulletPrimordialDmgField =
        typeof(PlayerWeapon).GetField("bulletPrimordialDmg", PrivateInstance);
    private static readonly FieldInfo PlayerWeaponBulletBurstDmgField =
        typeof(PlayerWeapon).GetField("bulletBurstDmg", PrivateInstance);
    private static readonly FieldInfo PlayerWeaponBulletDataField =
        typeof(PlayerWeapon).GetField("bulletData", PrivateInstance);
    private static readonly FieldInfo PlayerWeaponBulletBurstDataField =
        typeof(PlayerWeapon).GetField("bulletBurstData", PrivateInstance);
    private static readonly FieldInfo PlayerWeaponBulletSupershotDataField =
        typeof(PlayerWeapon).GetField("bulletSupershotData", PrivateInstance);
    private static readonly FieldInfo PlayerWeaponBulletPrimordialDataField =
        typeof(PlayerWeapon).GetField("bulletPrimordialData", PrivateInstance);
    private static readonly FieldInfo PlayerPowersReloadedField =
        typeof(PlayerPowers).GetField("Reloaded", PrivateStatic);
    private static readonly FieldInfo PlayerPowersIncreasedField =
        typeof(PlayerPowers).GetField("Increased", PrivateStatic);
    private static readonly FieldInfo PlayerControlClosestTargetField =
        typeof(PlayerControl).GetField("closestTarget", PrivateInstance);
    private static readonly FieldInfo PlayerPowersAlly1Field =
        typeof(PlayerPowers).GetField("ally1", PrivateInstance);
    private static readonly FieldInfo PlayerPowersAlly2Field =
        typeof(PlayerPowers).GetField("ally2", PrivateInstance);
    private static readonly FieldInfo PlayerPowersAlly3Field =
        typeof(PlayerPowers).GetField("ally3", PrivateInstance);
    private static readonly FieldInfo NpcTinyAllyFirerateIntervalField =
        typeof(NpcTinyAlly).GetField("firerateInterval", PrivateInstance);
    private static readonly FieldInfo NpcTinyAllyBulletDataField =
        typeof(NpcTinyAlly).GetField("bulletData", PrivateInstance);
    private static readonly FieldInfo ShipBehaviourCanOverHoleField =
        typeof(ShipBehaviour).GetField("canOverHole", PrivateInstance);
    private static readonly FieldInfo ShipBehaviourCanOverWaterField =
        typeof(ShipBehaviour).GetField("canOverWater", PrivateInstance);
    private static readonly FieldInfo ShipBehaviourCanOverWaterDeepField =
        typeof(ShipBehaviour).GetField("canOverWaterDeep", PrivateInstance);

    private static bool godModeEnabled;
    private static Destroyable godModeDestroyable;
    private static bool godModeOriginalInvincible;

    private static bool healthLockEnabled;
    private static int healthLockValue;

    private static bool infiniteEnergyEnabled;

    private static bool movementSpeedMultiplierEnabled;
    private static float movementSpeedMultiplier = 1f;

    private static bool damageMultiplierEnabled;
    private static float damageMultiplier = 1f;

    private static bool fireRateMultiplierEnabled;
    private static float fireRateMultiplier = 1f;

    private static bool bulletSpeedMultiplierEnabled;
    private static float bulletSpeedMultiplier = 1f;

    private static bool fireRangeMultiplierEnabled;
    private static float fireRangeMultiplier = 1f;

    private static bool spiritPowerMultiplierEnabled;
    private static float spiritPowerMultiplier = 1f;

    private static bool floatOverVoidWaterEnabled;
    private static ShipBehaviour floatOriginalShip;
    private static bool floatOriginalCanOverHole;
    private static bool floatOriginalCanOverWater;
    private static bool floatOriginalCanOverWaterDeep;
    private static readonly Dictionary<Surface, bool> floatOriginalSurfaceValues = new Dictionary<Surface, bool>();

    private static bool noPowerCooldownEnabled;
    private static bool noPowerCooldownForceNotify;

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
            Log("Minishoot' Adventures injection already initialized.");
            return;
        }

        GameObject go = new GameObject("GCM MainThreadDispatcher");
        go.AddComponent<MainThreadDispatcher>();
        Log("Initialized Minishoot' Adventures injection.");
    }

    public static void Tick()
    {
        try
        {
            ApplyGodMode();

            if (healthLockEnabled)
                ApplyHealth(healthLockValue);

            if (infiniteEnergyEnabled)
                ApplyInfiniteEnergy();

            if (movementSpeedMultiplierEnabled)
                ApplyMovementSpeedMultiplier();

            if (HasWeaponMultiplierEnabled())
                ApplyWeaponMultipliers();

            if (spiritPowerMultiplierEnabled)
                ApplySpiritPowerMultiplier();

            if (floatOverVoidWaterEnabled)
                ApplyFloatOverVoidWater();

            if (noPowerCooldownEnabled)
                ApplyNoPowerCooldown();
        }
        catch (Exception ex)
        {
            Log("Tick failed: " + ex.Message);
        }
    }

    public static void LateTick()
    {
        try
        {
            if (fireRangeMultiplierEnabled)
                ApplyAutoFireRangeMultiplier();

        }
        catch (Exception ex)
        {
            Log("LateTick failed: " + ex.Message);
        }
    }

    public static void ToggleGodMode(bool enabled)
    {
        RunOnMainThread(() =>
        {
            godModeEnabled = enabled;

            if (enabled)
                ApplyGodMode();
            else
                RestoreGodModeOriginal();

            Log("God Mode " + (enabled ? "enabled" : "disabled") + ".");
        });
    }

    public static void SetHealth(bool enabled, int value)
    {
        RunOnMainThread(() =>
        {
            healthLockEnabled = enabled;
            healthLockValue = NormalizeAmount(value);

            if (enabled)
                ApplyHealth(healthLockValue);

            Log("Health lock " + (enabled ? "enabled at " + healthLockValue : "disabled") + ".");
        });
    }

    public static void ToggleInfiniteEnergy(bool enabled)
    {
        RunOnMainThread(() =>
        {
            infiniteEnergyEnabled = enabled;

            if (enabled)
                ApplyInfiniteEnergy();

            Log("Infinite Energy " + (enabled ? "enabled" : "disabled") + ".");
        });
    }

    public static void SetMovementSpeedMultiplier(bool enabled, float value)
    {
        RunOnMainThread(() =>
        {
            movementSpeedMultiplierEnabled = enabled;
            movementSpeedMultiplier = NormalizeMultiplier(value);

            ApplyMovementSpeedMultiplier();

            Log("Movement Speed Multiplier " + (enabled ? "enabled at " + movementSpeedMultiplier : "disabled") + ".");
        });
    }

    public static void SetDamageMultiplier(bool enabled, float value)
    {
        RunOnMainThread(() =>
        {
            damageMultiplierEnabled = enabled;
            damageMultiplier = NormalizeMultiplier(value);

            ApplyWeaponMultipliers();

            Log("Damage Multiplier " + (enabled ? "enabled at " + damageMultiplier : "disabled") + ".");
        });
    }

    public static void SetFireRateMultiplier(bool enabled, float value)
    {
        RunOnMainThread(() =>
        {
            fireRateMultiplierEnabled = enabled;
            fireRateMultiplier = NormalizeMultiplier(value);

            ApplyWeaponMultipliers();

            Log("Fire Rate Multiplier " + (enabled ? "enabled at " + fireRateMultiplier : "disabled") + ".");
        });
    }

    public static void SetBulletSpeedMultiplier(bool enabled, float value)
    {
        RunOnMainThread(() =>
        {
            bulletSpeedMultiplierEnabled = enabled;
            bulletSpeedMultiplier = NormalizeMultiplier(value);

            ApplyWeaponMultipliers();

            Log("Bullet Speed Multiplier " + (enabled ? "enabled at " + bulletSpeedMultiplier : "disabled") + ".");
        });
    }

    public static void SetFireRangeMultiplier(bool enabled, float value)
    {
        RunOnMainThread(() =>
        {
            fireRangeMultiplierEnabled = enabled;
            fireRangeMultiplier = NormalizeMultiplier(value);

            ApplyWeaponMultipliers();

            Log("Fire Range Multiplier " + (enabled ? "enabled at " + fireRangeMultiplier : "disabled") + ".");
        });
    }

    public static void SetSpiritPowerMultiplier(bool enabled, float value)
    {
        RunOnMainThread(() =>
        {
            spiritPowerMultiplierEnabled = enabled;
            spiritPowerMultiplier = NormalizeMultiplier(value);

            ApplySpiritPowerMultiplier();

            Log("Spirit Power Multiplier " + (enabled ? "enabled at " + spiritPowerMultiplier : "disabled") + ".");
        });
    }

    public static void ToggleFloatOverVoidWater(bool enabled)
    {
        RunOnMainThread(() =>
        {
            floatOverVoidWaterEnabled = enabled;

            if (enabled)
                ApplyFloatOverVoidWater();
            else
                RestoreFloatOverVoidWater();

            Log("Float Over Void/Water " + (enabled ? "enabled" : "disabled") + ".");
        });
    }

    public static void ToggleNoPowerCooldown(bool enabled)
    {
        RunOnMainThread(() =>
        {
            noPowerCooldownEnabled = enabled;
            noPowerCooldownForceNotify = enabled;

            if (enabled)
                ApplyNoPowerCooldown();

            Log("No Power Cooldown " + (enabled ? "enabled" : "disabled") + ".");
        });
    }

    public static void SetStatsPoints(int value)
    {
        RunOnMainThread(() =>
        {
            value = NormalizeAmount(value);
            PlayerState.SetStatsPoint(value - PlayerState.StatsPoints);
            Log("Stats Points set to " + value + ".");
        });
    }

    public static void SetSuperCrystals(int value)
    {
        RunOnMainThread(() =>
        {
            value = NormalizeAmount(value);
            EnsureCurrencyState();

            int current = PlayerState.Currency[Currency.SuperCrystal];
            PlayerState.SetCurrency(Currency.SuperCrystal, value - current);
            Log("Super Crystals set to " + value + ".");
        });
    }

    private static void RunOnMainThread(Action action)
    {
        MainThreadDispatcher.Enqueue(action);
    }

    private static void ApplyGodMode()
    {
        if (!godModeEnabled)
            return;

        Destroyable destroyable = GetPlayerDestroyable();
        if (destroyable == null)
            return;

        CaptureGodModeOriginal(destroyable);
        destroyable.Invincible = true;
    }

    private static void CaptureGodModeOriginal(Destroyable destroyable)
    {
        if (destroyable == null || destroyable == godModeDestroyable)
            return;

        RestoreGodModeOriginal();
        godModeDestroyable = destroyable;
        godModeOriginalInvincible = destroyable.Invincible;
    }

    private static void RestoreGodModeOriginal()
    {
        if (godModeDestroyable != null)
            godModeDestroyable.Invincible = godModeOriginalInvincible;

        godModeDestroyable = null;
        godModeOriginalInvincible = false;
    }

    private static void ApplyHealth(int value)
    {
        Destroyable destroyable = GetPlayerDestroyable();
        if (destroyable == null || DestroyableHpCurrField == null)
            return;

        int target = NormalizeAmount(value);
        if (target > destroyable.HpMax)
            destroyable.SetHpMax(target, false);

        float previous = destroyable.HpCurr;
        if (Math.Abs(previous - target) < 0.001f)
            return;

        DestroyableHpCurrField.SetValue(destroyable, (float)target);
        NotifyHpChanged(destroyable, previous - target, target >= previous);
    }

    private static void ApplyInfiniteEnergy()
    {
        PlayerEnergy energy = GetPlayerEnergy();
        if (energy == null || PlayerEnergyCurrField == null)
            return;

        float max = Math.Max(0f, GetEnergyMax(energy));
        float previous = (float)PlayerEnergyCurrField.GetValue(energy);

        if (Math.Abs(previous - max) < 0.001f)
            return;

        PlayerEnergyCurrField.SetValue(energy, max);
        NotifyEnergyChanged(energy, previous, max, max);
    }

    private static void ApplyMovementSpeedMultiplier()
    {
        Player player = Player.Instance;
        if (player == null || player.Movable == null)
            return;

        float multiplier = movementSpeedMultiplierEnabled ? movementSpeedMultiplier : 1f;
        float moveSpeed = PlayerStats.MoveSpeed * multiplier;
        float boostSpeed = PlayerStats.BoostSpeed * multiplier;

        if (player.Movable.BaseMove != null)
            player.Movable.BaseMove.speedMax = moveSpeed;

        if (player.Movable.WaterMove != null)
            player.Movable.WaterMove.speedMax = moveSpeed;

        if (player.Movable.BoostMove != null)
            player.Movable.BoostMove.speedMax = boostSpeed;
    }

    private static void ApplyWeaponMultipliers()
    {
        PlayerWeapon weapon = Player.Weapon;
        if (weapon == null)
            return;

        float damageMul = damageMultiplierEnabled ? damageMultiplier : 1f;
        float fireRateMul = fireRateMultiplierEnabled ? fireRateMultiplier : 1f;
        float bulletSpeedMul = bulletSpeedMultiplierEnabled ? bulletSpeedMultiplier : 1f;
        float fireRangeMul = fireRangeMultiplierEnabled ? fireRangeMultiplier : 1f;

        SetFieldValue(PlayerWeaponFirerateDelayField, weapon, 1f / Math.Max(0.001f, PlayerStats.FireRate * fireRateMul));

        float bulletDamage = PlayerStats.BulletDamage * damageMul;
        SetFieldValue(PlayerWeaponBulletDmgField, weapon, bulletDamage);
        SetFieldValue(PlayerWeaponBulletSuperDmgField, weapon, bulletDamage);
        SetFieldValue(PlayerWeaponBulletPrimordialDmgField, weapon, bulletDamage * PlayerData.PrimordialDmgMod);
        SetFieldValue(PlayerWeaponBulletBurstDmgField, weapon, bulletDamage * PlayerStats.BulletNumber * PlayerStats.FireRate * PlayerData.BurstDmgMod);

        ApplyBulletData(GetFieldValue<BulletData>(PlayerWeaponBulletDataField, weapon),
            PlayerStats.BulletSpeed * bulletSpeedMul,
            PlayerStats.FireRange * fireRangeMul);
        ApplyBulletData(GetFieldValue<BulletData>(PlayerWeaponBulletSupershotDataField, weapon),
            PlayerStats.BulletSpeed * bulletSpeedMul,
            PlayerStats.FireRange * PlayerData.SupershotRangeMod * fireRangeMul);
        ApplyBulletData(GetFieldValue<BulletData>(PlayerWeaponBulletPrimordialDataField, weapon),
            PlayerStats.BulletSpeed * bulletSpeedMul,
            PlayerStats.FireRange * fireRangeMul);
        ApplyBulletData(GetFieldValue<BulletData>(PlayerWeaponBulletBurstDataField, weapon),
            PlayerData.BurstBulletSpeed * bulletSpeedMul,
            PlayerData.BurstRange * fireRangeMul);
    }

    private static void ApplyAutoFireRangeMultiplier()
    {
        PlayerControl control = Player.Control;
        if (control == null || PlayerControlClosestTargetField == null)
            return;

        float range = PlayerStats.FireRange * 1.5f * fireRangeMultiplier;
        Transform target = PlayerHelper.GetClosestTarget(
            Player.Position,
            12288,
            range,
            true,
            Vector2.zero,
            0f,
            new[] { "Enemy", "Boss", "CrystalEmptyBoss" });

        PlayerControlClosestTargetField.SetValue(control, target);
    }

    private static void ApplyBulletData(BulletData data, float speed, float range)
    {
        if (data == null)
            return;

        data.Speed = speed;
        data.Range = range;
    }

    private static void ApplySpiritPowerMultiplier()
    {
        float multiplier = spiritPowerMultiplierEnabled ? spiritPowerMultiplier : 1f;
        float fireRate = Math.Max(0.001f, PlayerStatsDerived.AllyFirerate * multiplier);

        foreach (NpcTinyAlly ally in GetSpiritAllies())
        {
            if (ally == null)
                continue;

            SetFieldValue(NpcTinyAllyFirerateIntervalField, ally, 1f / fireRate);

            BulletData data = GetFieldValue<BulletData>(NpcTinyAllyBulletDataField, ally);
            if (data == null)
                continue;

            data.Speed = PlayerStats.BulletSpeed * multiplier;
            data.Range = PlayerStatsDerived.AllyFireRange;
        }
    }

    private static IEnumerable<NpcTinyAlly> GetSpiritAllies()
    {
        PlayerPowers powers = Player.Powers;
        if (powers == null)
            yield break;

        NpcTinyAlly ally1 = GetFieldValue<NpcTinyAlly>(PlayerPowersAlly1Field, powers);
        NpcTinyAlly ally2 = GetFieldValue<NpcTinyAlly>(PlayerPowersAlly2Field, powers);
        NpcTinyAlly ally3 = GetFieldValue<NpcTinyAlly>(PlayerPowersAlly3Field, powers);

        if (ally1 != null)
            yield return ally1;
        if (ally2 != null)
            yield return ally2;
        if (ally3 != null)
            yield return ally3;
    }

    private static void ApplyFloatOverVoidWater()
    {
        Player player = Player.Instance;
        if (player == null || player.SurfaceHandler == null)
            return;

        CaptureFloatOriginals(player);

        SetFieldValue(ShipBehaviourCanOverHoleField, player, true);
        SetFieldValue(ShipBehaviourCanOverWaterField, player, true);
        SetFieldValue(ShipBehaviourCanOverWaterDeepField, player, true);

        ForceSurfaceCanOver(player.SurfaceHandler.Hole);
        ForceSurfaceCanOver(player.SurfaceHandler.Water);
        ForceSurfaceCanOver(player.SurfaceHandler.WaterDeep);
    }

    private static void CaptureFloatOriginals(Player player)
    {
        if (floatOriginalShip == player)
            return;

        RestoreFloatOverVoidWater();

        floatOriginalShip = player;
        floatOriginalCanOverHole = GetFieldStructValue<bool>(ShipBehaviourCanOverHoleField, player);
        floatOriginalCanOverWater = GetFieldStructValue<bool>(ShipBehaviourCanOverWaterField, player);
        floatOriginalCanOverWaterDeep = GetFieldStructValue<bool>(ShipBehaviourCanOverWaterDeepField, player);

        CaptureSurfaceOriginal(player.SurfaceHandler.Hole);
        CaptureSurfaceOriginal(player.SurfaceHandler.Water);
        CaptureSurfaceOriginal(player.SurfaceHandler.WaterDeep);
    }

    private static void CaptureSurfaceOriginal(Surface surface)
    {
        if (surface == null || floatOriginalSurfaceValues.ContainsKey(surface))
            return;

        floatOriginalSurfaceValues[surface] = surface.CanOver;
    }

    private static void ForceSurfaceCanOver(Surface surface)
    {
        if (surface == null)
            return;

        CaptureSurfaceOriginal(surface);
        surface.CanOver = true;
    }

    private static void RestoreFloatOverVoidWater()
    {
        if (floatOriginalShip != null)
        {
            SetFieldValue(ShipBehaviourCanOverHoleField, floatOriginalShip, floatOriginalCanOverHole);
            SetFieldValue(ShipBehaviourCanOverWaterField, floatOriginalShip, floatOriginalCanOverWater);
            SetFieldValue(ShipBehaviourCanOverWaterDeepField, floatOriginalShip, floatOriginalCanOverWaterDeep);
        }

        foreach (KeyValuePair<Surface, bool> entry in floatOriginalSurfaceValues)
        {
            if (entry.Key != null)
                entry.Key.CanOver = entry.Value;
        }

        floatOriginalShip = null;
        floatOriginalCanOverHole = false;
        floatOriginalCanOverWater = false;
        floatOriginalCanOverWaterDeep = false;
        floatOriginalSurfaceValues.Clear();
    }

    private static void ApplyNoPowerCooldown()
    {
        PlayerPowers powers = Player.Powers;
        if (powers == null || powers.LoadRatio == null)
            return;

        foreach (Power power in Enum.GetValues(typeof(Power)))
        {
            if (!powers.LoadRatio.ContainsKey(power))
                continue;

            float previous = powers.LoadRatio[power];
            powers.LoadRatio[power] = 1f;

            if (previous < 1f || noPowerCooldownForceNotify)
                InvokeDelegate(PlayerPowersIncreasedField, null, power);

            if (previous < 1f)
                InvokeDelegate(PlayerPowersReloadedField, null, power);
        }

        noPowerCooldownForceNotify = false;
    }

    private static Destroyable GetPlayerDestroyable()
    {
        Player player = Player.Instance;
        if (player == null)
            return null;

        return player.Destroyable;
    }

    private static PlayerEnergy GetPlayerEnergy()
    {
        return Player.Energy;
    }

    private static void EnsureCurrencyState()
    {
        if (PlayerState.Currency == null)
            PlayerState.Currency = new Dictionary<Currency, int>();

        if (!PlayerState.Currency.ContainsKey(Currency.SuperCrystal))
            PlayerState.Currency[Currency.SuperCrystal] = 0;
    }

    private static int NormalizeAmount(int value)
    {
        return Math.Max(0, value);
    }

    private static float NormalizeMultiplier(float value)
    {
        if (float.IsNaN(value) || float.IsInfinity(value))
            return 1f;

        return Math.Max(0f, value);
    }

    private static bool HasWeaponMultiplierEnabled()
    {
        return damageMultiplierEnabled ||
               fireRateMultiplierEnabled ||
               bulletSpeedMultiplierEnabled ||
               fireRangeMultiplierEnabled;
    }

    private static float GetEnergyMax(PlayerEnergy energy)
    {
        if (PlayerEnergyMaxProperty == null)
            return 0f;

        object value = PlayerEnergyMaxProperty.GetValue(energy, null);
        return Convert.ToSingle(value);
    }

    private static T GetFieldValue<T>(FieldInfo field, object instance) where T : class
    {
        if (field == null)
            return null;

        return field.GetValue(instance) as T;
    }

    private static T GetFieldStructValue<T>(FieldInfo field, object instance) where T : struct
    {
        if (field == null)
            return default(T);

        object value = field.GetValue(instance);
        if (value == null)
            return default(T);

        return (T)value;
    }

    private static void SetFieldValue(FieldInfo field, object instance, object value)
    {
        if (field == null)
            return;

        field.SetValue(instance, value);
    }

    private static void NotifyHpChanged(Destroyable destroyable, float damage, bool restored)
    {
        InvokeDelegate(DestroyableHpChangedField, destroyable, damage, restored);
    }

    private static void NotifyEnergyChanged(PlayerEnergy energy, float previous, float current, float max)
    {
        if (current > previous)
            InvokeDelegate(PlayerEnergyRechargedPartiallyField, energy);

        if (current >= max && previous < max)
        {
            if (PlayerEnergyHasGoneBelowThresholdField != null)
                PlayerEnergyHasGoneBelowThresholdField.SetValue(null, false);

            InvokeDelegate(PlayerEnergyRechargedField, energy);
        }
    }

    private static void InvokeDelegate(FieldInfo eventField, object instance, params object[] args)
    {
        if (eventField == null)
            return;

        Delegate handler = eventField.GetValue(instance) as Delegate;
        if (handler == null)
            return;

        handler.DynamicInvoke(args);
    }
}
