using System;
using System.Collections.Generic;
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

    private static bool autoPlayEnabled;

    private static bool noDeathEnabled;

    private static bool gameSpeedMultiplierEnabled;
    private static float gameSpeedMultiplier = 1f;

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
            Log("A Dance of Fire and Ice injection already initialized.");
            return;
        }

        GameObject go = new GameObject("GCM MainThreadDispatcher");
        go.AddComponent<MainThreadDispatcher>();
        Log("Initialized A Dance of Fire and Ice injection.");
    }

    public static void Tick()
    {
        try
        {
            if (autoPlayEnabled)
                ApplyAutoPlay();

            if (noDeathEnabled)
                ApplyNoDeath();

            if (gameSpeedMultiplierEnabled)
                ApplyGameSpeedMultiplier();
        }
        catch (Exception ex)
        {
            Log("Tick failed: " + ex.Message);
        }
    }

    public static void LateTick()
    {
    }

    // ============================================================
    // Trainer entry points
    // ============================================================

    public static void ToggleAutoPlay(bool enabled)
    {
        RunOnMainThread(() =>
        {
            autoPlayEnabled = enabled;

            if (enabled)
                ApplyAutoPlay();
            else
                RestoreAutoPlay();

            Log("Auto Play " + (enabled ? "enabled" : "disabled") + ".");
        });
    }

    public static void ToggleNoDeath(bool enabled)
    {
        RunOnMainThread(() =>
        {
            noDeathEnabled = enabled;

            if (enabled)
                ApplyNoDeath();
            else
                RestoreNoDeath();

            Log("No Death " + (enabled ? "enabled" : "disabled") + ".");
        });
    }

    public static void SetGameSpeedMultiplier(bool enabled, float value)
    {
        RunOnMainThread(() =>
        {
            gameSpeedMultiplierEnabled = enabled;
            gameSpeedMultiplier = NormalizeMultiplier(value);

            if (enabled)
                ApplyGameSpeedMultiplier();
            else
                RestoreGameSpeedMultiplier();

            Log("Game Speed Multiplier " + (enabled ? "enabled at " + gameSpeedMultiplier : "disabled") + ".");
        });
    }

    public static void FinishLevelPerfectly()
    {
        RunOnMainThread(() =>
        {
            ApplyFinishLevelPerfectly();
            Log("Finish Level Perfectly applied.");
        });
    }

    // ============================================================
    // Cheat implementations
    // ============================================================

    // TODO: research - drive the game's built-in auto play / bot input so tiles are hit on beat.
    private static void ApplyAutoPlay()
    {
    }

    // TODO: research - restore whatever ApplyAutoPlay overrode.
    private static void RestoreAutoPlay()
    {
    }

    // TODO: research - suppress the fail/death path so a mistimed or missed hit does not restart the level.
    private static void ApplyNoDeath()
    {
    }

    // TODO: research - restore whatever ApplyNoDeath overrode.
    private static void RestoreNoDeath()
    {
    }

    // TODO: research - scale level playback speed (conductor BPM / audio pitch), not just Time.timeScale,
    // otherwise the chart and the music desync.
    private static void ApplyGameSpeedMultiplier()
    {
    }

    // TODO: research - restore the original playback speed.
    private static void RestoreGameSpeedMultiplier()
    {
    }

    // TODO: research - complete the current level and report every hit as a perfect one.
    private static void ApplyFinishLevelPerfectly()
    {
    }

    // ============================================================
    // Helpers
    // ============================================================

    private static void RunOnMainThread(Action action)
    {
        MainThreadDispatcher.Enqueue(action);
    }

    private static float NormalizeMultiplier(float value)
    {
        if (float.IsNaN(value) || float.IsInfinity(value))
            return 1f;

        return Math.Max(0f, value);
    }
}
