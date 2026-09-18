using System;
using System.Collections.Generic;
using System.Reflection;
using RDLevelEditor;
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

    /// <summary>Success. Anything else is shown to the user under the trainer's generic line.</summary>
    private const string ResponseOk = "OK";

    // MistakesManager.mistakes has an [Obsolete] no-op setter, so the counters behind it are what the
    // rank steering actually writes.
    private static readonly FieldInfo mistakesP1Field =
        typeof(MistakesManager).GetField("mistakesCountP1", BindingFlags.NonPublic | BindingFlags.Instance);
    private static readonly FieldInfo mistakesP2Field =
        typeof(MistakesManager).GetField("mistakesCountP2", BindingFlags.NonPublic | BindingFlags.Instance);

    private static bool infiniteHealthEnabled;
    private static bool autoPlayEnabled;
    private static bool gameSpeedEnabled;
    private static float gameSpeed = 1f;
    private static bool speedHeld;

    // Infinite Health overwrites a flag the level owns, so its previous value is kept per level
    // instance and put back when the cheat is switched off.
    private static LevelBase guardedLevel;
    private static bool guardedLevelNoBossFail;

    // Auto Play overwrites two flags the level owns, so their previous values are kept per level
    // instance and put back when it lets go. exemptLevel is the one level that still records.
    private static LevelBase uncountedLevel;
    private static bool uncountedLevelDogMode;
    private static bool uncountedLevelUseScore;
    private static LevelBase exemptLevel;

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
            Log("Rhythm Doctor injection already initialized.");
            return;
        }

        GameObject go = new GameObject("GCM MainThreadDispatcher");
        go.AddComponent<MainThreadDispatcher>();
        Log("Initialized Rhythm Doctor injection.");
    }

    /// <summary>Once per frame: where a cheat re-asserts a value the game keeps overwriting.</summary>
    public static void Tick()
    {
        try
        {
            scnGame game = scnGame.instance;

            if (infiniteHealthEnabled)
                HoldNoBossFail(game);

            // scnBase.GoToScene() clears autoplay on every scene change, so the toggle has to hold it.
            if (autoPlayEnabled)
            {
                if (!DebugSettings.instance.Auto)
                    DebugSettings.instance.Auto = true;

                HoldNoProgress(game);
            }

            HoldLevelSpeed(game);
        }
        catch (Exception ex)
        {
            Log("Tick failed: " + ex.Message);
        }
    }

    // ============================================================
    // Trainer entry points
    // ============================================================

    public static void ToggleInfiniteHealth(bool enabled)
    {
        RunCheat(delegate
        {
            infiniteHealthEnabled = enabled;
            if (!enabled)
                ReleaseNoBossFail();

            return null;
        });
    }

    public static void ToggleAutoPlay(bool enabled)
    {
        RunCheat(delegate
        {
            autoPlayEnabled = enabled;
            DebugSettings.instance.Auto = enabled;
            if (!enabled)
            {
                ReleaseAutoInput();
                ReleaseNoProgress();
                exemptLevel = null;
            }

            return null;
        });
    }

    public static void SetGameSpeedMultiplier(bool enabled, float value)
    {
        RunCheat(delegate
        {
            gameSpeedEnabled = enabled;
            gameSpeed = enabled ? value : 1f;
            HoldLevelSpeed(scnGame.instance);
            return null;
        });
    }

    /// <summary>
    /// Ends the level through the game's own WinLevel(), after steering the three inputs its rank
    /// calculation reads so that it arrives at the rank the user asked for.
    /// </summary>
    public static void InstantCompleteLevel(int rank)
    {
        RunCheat(delegate
        {
            scnGame game = scnGame.instance;
            if (game == null || game.currentLevel == null)
                return "Please start a level first.";

            if (game.failedLevel)
                return "The level has already ended.";

            // The user asked for this result, so it records normally even with Auto Play on.
            exemptLevel = game.currentLevel;
            ReleaseNoProgress();

            // A level normally reaches its rank screen having already run out of chart: the song is
            // over and the conductor has passed the last bar. WinLevel() assumes all of that and only
            // shows the screen, which is why firing it mid-level left everything else running.
            //
            // So put the level in that state first. FailLevelLite() is the game's own helper for the
            // audible half - pending executes, the beats, the song, the beat loops.
            game.FailLevelLite();

            // The other half is the level's own script, which runs off the conductor's bar number.
            // Moving that past the end of the chart is exactly how a finished level stops running it:
            // basePrebarActions() and baseActions() are both already guarded on this very comparison,
            // so nothing has to be overridden to make them stand down.
            List<LevelEvent_Base>[] chart = game.currentLevel.levelEventsPerBar;
            if (chart != null && scrConductor.instance != null)
                scrConductor.instance.barNumber = chart.Length;

            // Only now are the rank's inputs the last word - stopping the level above can still count
            // beats that were in flight, and those would have moved the rank off what was asked for.
            SteerRank(game, rank);

            game.WinLevel();
            return null;
        });
    }

    // ============================================================
    // Cheats
    // ============================================================

    /// <summary>
    /// noBossFail is the game's own "this level cannot kill you" switch, checked after the heart has
    /// already cracked and the health bars have been updated - so misses still land and the heart
    /// still takes damage, only FailLevel() never fires.
    /// </summary>
    private static void HoldNoBossFail(scnGame game)
    {
        LevelBase level = (game != null) ? game.currentLevel : null;
        if (level == null)
            return;

        if (level != guardedLevel)
        {
            ReleaseNoBossFail();
            guardedLevel = level;
            guardedLevelNoBossFail = level.noBossFail;
        }

        level.noBossFail = true;
    }

    private static void ReleaseNoBossFail()
    {
        if (guardedLevel != null)
            guardedLevel.noBossFail = guardedLevelNoBossFail;

        guardedLevel = null;
    }

    /// <summary>
    /// Beat.LateUpdate() raises autoHeldBeatCounter when it starts a held clap and lowers it when that
    /// clap ends - both inside the same "is autoplay on" branch. Switching autoplay off between the
    /// two strands the counter above zero, ReleaseIfClear() then never lifts the emulated key, and the
    /// game goes on seeing a held input for the rest of the level. Put both back.
    /// </summary>
    private static void ReleaseAutoInput()
    {
        scnGame game = scnGame.instance;
        if (game != null)
            game.autoHeldBeatCounter = 0;

        if (RDInput.emuStates == null)
            return;

        for (int i = 0; i < RDInput.emuStates.Length; i++)
        {
            if (RDInput.emuStates[i] != null)
                RDInput.emuStates[i].SetKey(RDInput.PlayerEmuKey.Up);
        }
    }

    /// <summary>
    /// Only the seed is written, never RDTime.speed itself. scnGame reads levelSpeed into RDTime.speed
    /// as a level loads, and scrConductor then bakes that into the song's pitch and the chart's BPM -
    /// so a speed change after the song has started moves the chart but not the audio, which is the
    /// desync. Writing the seed alone means the speed lands on the next level start, intact.
    ///
    /// It is also only written inside the game scene: levelSpeed is what the level select's own Ice /
    /// Chili control sets, and holding it out there is what disturbed the level select.
    /// </summary>
    private static void HoldLevelSpeed(scnGame game)
    {
        if (game == null)
        {
            speedHeld = false;
            return;
        }

        if (gameSpeedEnabled)
        {
            scnGame.levelSpeed = gameSpeed;
            speedHeld = true;
        }
        else if (speedHeld)
        {
            scnGame.levelSpeed = 1f;
            speedHeld = false;
        }
    }

    /// <summary>
    /// dogMode is the game's own "this run does not count" switch, and it is the whole cheat: it is
    /// what stops ShowAndSaveRank() and SaveCurrentBossLevelAsPassed() from calling SetLevelRank(),
    /// and every level achievement plus the Steam stat flush hangs off the end of that one method.
    /// useScore closes the one remaining save call, and the achievement inside it.
    ///
    /// It also swaps some on-screen instructions for their Rhythm Dogtor variants while it is up.
    /// </summary>
    private static void HoldNoProgress(scnGame game)
    {
        LevelBase level = (game != null) ? game.currentLevel : null;

        // The exemption belongs to the one level it was granted for.
        if (exemptLevel != null && level != exemptLevel)
            exemptLevel = null;

        if (level == null || level == exemptLevel)
        {
            ReleaseNoProgress();
            return;
        }

        if (level != uncountedLevel)
        {
            ReleaseNoProgress();
            uncountedLevel = level;
            uncountedLevelDogMode = level.dogMode;
            uncountedLevelUseScore = level.useScore;
        }

        level.dogMode = true;
        level.useScore = false;
    }

    private static void ReleaseNoProgress()
    {
        if (uncountedLevel != null)
        {
            uncountedLevel.dogMode = uncountedLevelDogMode;
            uncountedLevel.useScore = uncountedLevelUseScore;
        }

        uncountedLevel = null;
    }

    /// <summary>
    /// LevelBase.GetRankFromMistakes() derives the rank from the mistake count against the level's own
    /// thresholds, then turns it into a + or - grade from how early the player's hits were. Setting
    /// all three makes it return an exact rank.
    /// </summary>
    private static void SteerRank(scnGame game, int rank)
    {
        int normal = ((Rank)rank).ToNormal();

        // S is the only rank a nonzero mistake count cannot produce, so it is the only one that needs
        // the counters cleared; every other rank comes from moving the thresholds around a count of 1.
        MistakesManager mistakes = game.mistakesManager;
        mistakesP1Field.SetValue(mistakes, (normal == Rank.S) ? 0f : 1f);
        mistakesP2Field.SetValue(mistakes, 0f);

        float[] bounds = game.currentLevel.rankLowerBounds;
        if (normal != Rank.S && bounds != null && bounds.Length >= 5)
        {
            bounds[4] = (normal == Rank.A) ? 1f : 0f;
            bounds[3] = (normal == Rank.B) ? 1f : 0f;
            bounds[2] = (normal == Rank.C) ? 1f : 0f;
            bounds[1] = (normal == Rank.D) ? 1f : 0f;
        }

        // An empty hit list reads as "every hit was early" and grades up; one hit inside the window
        // grades flat, one outside it grades down.
        scnGame.p1HitTimes.Clear();
        scnGame.p2HitTimes.Clear();
        if (rank == normal)
            scnGame.p1HitTimes.Add(0.06f);
        else if (rank < 0)
            scnGame.p1HitTimes.Add(0.1f);
    }

    // ============================================================
    // Helpers
    // ============================================================

    /// <summary>
    /// Runs a cheat on the main thread and answers the trainer exactly once - null for success, or the
    /// reason it refused. The trainer blocks on that reply, so every path must answer, unhandled
    /// exceptions included.
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
            // The trainer shows this verbatim under its generic line, so it carries the exception
            // type too - "Object reference not set to an instance of an object." alone says little.
            catch (Exception ex)
            {
                LogException("Cheat failed", ex);
                message = ex.GetType().Name + ": " + ex.Message;
            }

            if (!string.IsNullOrEmpty(message))
                Log("[!] " + message);

            SendResponse(string.IsNullOrEmpty(message) ? ResponseOk : message);
        });

        // Nothing will run the cheat, so answer here rather than let the trainer time out.
        if (!queued)
        {
            Log("[!] The dispatcher is not running; the cheat was dropped.");
            SendResponse("Please restart the game and the trainer.");
        }
    }
}
