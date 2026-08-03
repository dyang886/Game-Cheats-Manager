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

    [DllImport("MonoBridge.dll")]
    private static extern void SendResponse(string message);

    /// <summary>
    /// Reply for an apply action that succeeded. Anything else the trainer receives is a message to
    /// show the user, run through its own translation table - so the strings below are the keys.
    /// </summary>
    private const string ResponseOk = "OK";

    private const float SpikeImmunitySeconds = 1f;

    private static bool godModeEnabled;
    private static bool godModeOriginalCaptured;
    private static bool godModeOriginalUseNoFail;

    private static bool gameSpeedEnabled;
    private static float gameSpeed = 1f;
    private static bool gameSpeedOriginalCaptured;
    private static float gameSpeedOriginalCurrent = 1f;
    private static float gameSpeedOriginalNext = 1f;

    private static bool autoPlayEnabled;
    private static scrController autoPlayGuardedController;
    private static bool autoPlayOriginalKeyLimiter;
    private static int autoPlayOriginalMaxKeys;

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
            ApplyAutoPlay();

            if (godModeEnabled)
                ApplyGodMode();

            if (gameSpeedEnabled)
                ApplyGameSpeedMultiplier();
        }
        catch (Exception ex)
        {
            Log("Tick failed: " + ex.Message);
        }
    }

    // ============================================================
    // Trainer entry points
    // ============================================================

    public static void ToggleAutoPlay(bool enabled)
    {
        RunOnMainThread(delegate
        {
            autoPlayEnabled = enabled;
            ApplyAutoPlay();

            Log("Auto Play " + (enabled ? "enabled" : "disabled") + ".");
        });
    }

    public static void ToggleGodMode(bool enabled)
    {
        RunOnMainThread(delegate
        {
            godModeEnabled = enabled;

            if (enabled)
            {
                CaptureGodModeOriginal();
                ApplyGodMode();
            }
            else
            {
                RestoreGodMode();
            }

            Log("God Mode " + (enabled ? "enabled" : "disabled") + ".");
        });
    }

    public static void SetGameSpeedMultiplier(bool enabled, float value)
    {
        RunOnMainThread(delegate
        {
            gameSpeedEnabled = enabled;
            gameSpeed = NormalizeMultiplier(value);

            if (enabled)
            {
                CaptureGameSpeedOriginal();
                ApplyGameSpeedMultiplier();
            }
            else
            {
                RestoreGameSpeedMultiplier();
            }

            Log("Game Speed Multiplier " + (enabled ? "set to " + gameSpeed + " (applies when the level starts)" : "disabled") + ".");
        });
    }

    public static void FinishLevelPerfectly()
    {
        RunOnMainThread(delegate
        {
            scrController controller = ADOBase.controller;
            if (controller == null || !controller.gameworld)
            {
                Fail("Please activate this inside a level.");
                return;
            }

            if (controller.currentState == States.Won || controller.currentState == States.Fail ||
                controller.currentState == States.Fail2)
            {
                Fail("The current level is already over.");
                return;
            }

            CancelPressToStartWait(controller);
            TeleportToEndPortal(controller);
            MarkRunAsPurePerfect(controller);

            // Deliberately not scrController.BeatLevel(): that calls OnLandOnPortal and then
            // PortalTravelAction back to back, which skips straight past the results screen into the
            // level transition. Landing on the end portal normally only calls OnLandOnPortal, and
            // Won_Update fires the transition once the player presses a key.
            controller.OnLandOnPortal(controller.planetRed, Portal.EndOfLevel, null);
            GrantTaroMedals(controller);
            SavePurePerfectResult(controller);

            Log("Finished the current level as a pure perfect run.");
            Succeed();
        });
    }

    // ============================================================
    // Cheat implementations
    // ============================================================

    /// <summary>
    /// Drives the game's own autoplay, and keeps the resulting run out of the save file.
    ///
    /// scrController.Awake clears RDC.auto on every level load and restart, so holding the toggle on
    /// means re-asserting it each frame. It is only asserted while gameworld is true: menus and the
    /// hub run a scrController and let you move over tiles too, but leave that flag false.
    ///
    /// The game has no autoplay guard of its own - RDC.auto never reaches Save or SaveCustom - so
    /// this borrows the flag it does use to disqualify a run: scrController.unlockKeyLimiter, which
    /// makes scrMistakesManager.Save return before it writes anything. SaveCustom pairs that flag
    /// with a key count, so CLS levels need both. Awake re-seeds unlockKeyLimiter from
    /// GCS.useUnlockKeyLimiter alongside noFail, so neither flag can leak into a later level.
    /// </summary>
    private static void ApplyAutoPlay()
    {
        scrController controller = ADOBase.controller;

        if (autoPlayEnabled && controller != null && controller.gameworld)
        {
            if (autoPlayGuardedController != controller)
            {
                autoPlayGuardedController = controller;
                autoPlayOriginalKeyLimiter = controller.unlockKeyLimiter;
                autoPlayOriginalMaxKeys = controller.maximumUsedKeys;
            }

            RDC.auto = true;
            controller.unlockKeyLimiter = true;

            // Only custom levels need the key count, and it shows up on the results screen once it
            // passes 10 - so leave it alone everywhere it is not load-bearing.
            if (ADOBase.isCLSLevel && controller.maximumUsedKeys <= 1000)
                controller.maximumUsedKeys = 1001;

            ClearPendingSectionMedals();
            return;
        }

        if (autoPlayGuardedController == null)
            return;

        // Switched off inside the level we were driving; hand everything back. A level change needs
        // no undo - that controller is already gone and its replacement re-seeds both flags.
        if (autoPlayGuardedController == controller)
        {
            RDC.auto = false;
            controller.unlockKeyLimiter = autoPlayOriginalKeyLimiter;
            controller.maximumUsedKeys = autoPlayOriginalMaxKeys;
        }

        autoPlayGuardedController = null;
    }

    /// <summary>
    /// Keeps the DLC section medals out of an autoplayed run. They never go through
    /// scrMistakesManager.Save, so unlockKeyLimiter does not cover them: each Taro level's script
    /// judges a section into its own *_Stats.sectionStats list, and TaroBGScript.SaveMedals writes
    /// that list to Persistence at the results screen and from the pause menu.
    ///
    /// Autoplay actually makes this worse on its own - the mistake trackers those scripts keep are
    /// all written behind "&amp;&amp; !RDC.auto", so with autoplay on nothing ever marks a section as
    /// flawed and every section is judged a gold 3. Since SaveMedals only ever raises a medal
    /// (newMedals[i] &gt; stored[i]), holding the list at zero makes the write a no-op.
    ///
    /// GCS.pauseMedalStatsCurrent is the same List instance the running level's script judges into -
    /// each level's Awake points it at its own stats - so clearing it covers both save paths.
    /// </summary>
    private static void ClearPendingSectionMedals()
    {
        List<int> sectionMedals = GCS.pauseMedalStatsCurrent;
        if (sectionMedals == null)
            return;

        for (int i = 0; i < sectionMedals.Count; i++)
        {
            if (sectionMedals[i] != 0)
                sectionMedals[i] = 0;
        }
    }

    private static void ApplyGodMode()
    {
        // scrController.Awake copies GCS.useNoFail into scrController.noFail, so setting both keeps
        // God Mode alive across level loads and restarts.
        GCS.useNoFail = true;

        scrController controller = ADOBase.controller;
        if (controller == null)
            return;

        controller.noFail = true;

        // scrPlayer.Die skips the whole noFail branch when it is called with hitbox: true, so spikes
        // and kill tiles still kill. Every one of those call sites bails out while the planet has
        // i-frames left, which is a plain countdown field we can keep topped up.
        foreach (scrPlayer player in ADOBase.playerManager)
        {
            if (player == null || player.planetarySystem == null || player.planetarySystem.planetList == null)
                continue;

            foreach (scrPlanet planet in player.planetarySystem.planetList)
            {
                if (planet != null && planet.iFrames < SpikeImmunitySeconds)
                    planet.iFrames = SpikeImmunitySeconds;
            }
        }
    }

    private static void CaptureGodModeOriginal()
    {
        if (godModeOriginalCaptured)
            return;

        godModeOriginalUseNoFail = GCS.useNoFail;
        godModeOriginalCaptured = true;
    }

    private static void RestoreGodMode()
    {
        if (godModeOriginalCaptured)
        {
            GCS.useNoFail = godModeOriginalUseNoFail;
            godModeOriginalCaptured = false;
        }

        scrController controller = ADOBase.controller;
        if (controller != null)
            controller.noFail = GCS.useNoFail;

        // Leftover i-frames run themselves out within a second.
    }

    /// <summary>
    /// Drives the game's Speed Trial multiplier. The conductor bakes it into song.pitch once, when
    /// the level is set up, and every tile's entry time is precomputed from that pitch - so this
    /// takes effect on the next level start or restart rather than mid-run.
    /// </summary>
    private static void ApplyGameSpeedMultiplier()
    {
        GCS.currentSpeedTrial = gameSpeed;
        GCS.nextSpeedRun = gameSpeed;
    }

    private static void CaptureGameSpeedOriginal()
    {
        if (gameSpeedOriginalCaptured)
            return;

        gameSpeedOriginalCurrent = GCS.currentSpeedTrial;
        gameSpeedOriginalNext = GCS.nextSpeedRun;
        gameSpeedOriginalCaptured = true;
    }

    private static void RestoreGameSpeedMultiplier()
    {
        if (!gameSpeedOriginalCaptured)
            return;

        GCS.currentSpeedTrial = gameSpeedOriginalCurrent;
        GCS.nextSpeedRun = gameSpeedOriginalNext;
        gameSpeedOriginalCaptured = false;
    }

    /// <summary>
    /// Rewrites the accuracy trackers so the run reads as a full pure perfect clear: every logged
    /// hit becomes HitMargin.Perfect, the list is padded out to the level's hittable tile count so
    /// the stored accuracy matches a real full run, and death / checkpoint bookkeeping is cleared.
    /// scrMarginTracker.IsAllPurePerfect checks exactly these, and DetailedResults prints the
    /// accuracy in gold when it passes.
    /// </summary>
    private static void MarkRunAsPurePerfect(scrController controller)
    {
        int hittableTiles = CountHittableTiles();

        // percentComplete is derived from currentSeqID inside CalculatePercentAcc, and
        // EndscreenLanterns only lights the completion lantern at percentComplete >= 1, so the run
        // has to read as having reached the last tile. The level is over right after this, and
        // scrController.Awake resets currentSeqID on the next load.
        if (ADOBase.lm != null && ADOBase.lm.listFloors != null && ADOBase.lm.listFloors.Count > 0)
            controller.currentSeqID = ADOBase.lm.listFloors.Count - 1;

        foreach (scrMarginTracker tracker in scrMistakesManager.marginTrackers)
        {
            if (tracker == null || tracker.hitMargins == null)
                continue;

            for (int i = 0; i < tracker.hitMargins.Count; i++)
                tracker.hitMargins[i] = HitMargin.Perfect;

            while (tracker.hitMargins.Count < hittableTiles)
                tracker.hitMargins.Add(HitMargin.Perfect);

            Array.Clear(tracker.hitMarginsCount, 0, tracker.hitMarginsCount.Length);
            tracker.hitMarginsCount[(int)HitMargin.Perfect] = tracker.hitMargins.Count;

            tracker.lastHitMarginsSize = 0;
            tracker.deadTiles = 0;
            tracker.deadTilesBeforeCheckpoint = 0;
            tracker.deaths = 0;
            tracker.deathsBeforeCheckpoint = 0;

            // OnLandOnPortal calls RegisterDeadTiles(currentSeqID) for any player that is not alive,
            // which would count everything from here to the last tile as dead. Parking the marker on
            // the final tile makes that a no-op.
            tracker.deadTilesStartFloor = controller.currentSeqID;

            tracker.CalculatePercentAcc();
        }

        // IsAllPurePerfect rejects runs started from a checkpoint, and percentXAcc is scaled down by
        // every checkpoint used.
        controller.startedFromCheckpoint = false;
        scrController.checkpointsUsed = 0;

        if (ADOBase.customLevel != null)
            ADOBase.customLevel.checkpointsUsed = 0;
    }

    /// <summary>
    /// Counts the tiles a real playthrough would register a hit on. scrMarginTracker treats
    /// hitMargins.Count + deadTiles as the level's total countable tiles, and RegisterDeadTiles
    /// fills deadTiles from the half-open range [startFloor, currentFloor) skipping auto and
    /// midspin tiles - so a full clear ends with one hit per such tile, final tile excluded.
    /// </summary>
    private static int CountHittableTiles()
    {
        scrLevelMaker lm = ADOBase.lm;
        if (lm == null || lm.listFloors == null)
            return 0;

        int count = 0;
        for (int i = 0; i < lm.listFloors.Count - 1; i++)
        {
            scrFloor floor = lm.listFloors[i];
            if (floor != null && !floor.auto && !floor.midSpin)
                count++;
        }

        return count;
    }

    /// <summary>
    /// Disarms the "press a key to begin" wait. scrController.WaitForStartCo parks on that prompt
    /// and, once it finally sees an input, runs the whole level start sequence - ShowGetReady,
    /// conductor.Rewind and Start, Start_Rewind. Finishing the level while it is still parked leaves
    /// it armed, so the next click starts the level underneath the results screen with the planet
    /// stranded on the end tile and no way out but quitting.
    ///
    /// The coroutine cancels itself when waitForStartCoCallCount moves under it - it yield breaks and
    /// hides the prompt on its way out - which is how a second WaitForStartCo call retires the first.
    /// Bumping that counter is the game's own cancel signal, so use it rather than reimplementing the
    /// teardown.
    /// </summary>
    private static void CancelPressToStartWait(scrController controller)
    {
        // scrController.Start parks in States.Start until the level actually begins.
        if (controller.currentState != States.Start)
            return;

        FieldInfo callCount = typeof(scrController).GetField("waitForStartCoCallCount",
            BindingFlags.NonPublic | BindingFlags.Instance);

        if (callCount == null)
        {
            // Renamed by a game update - fall back to killing the coroutine outright. scrController
            // only ever runs WaitForStartCo and ResetCustomLevel, so nothing else is lost.
            Log("[!] scrController.waitForStartCoCallCount not found; stopping coroutines instead.");
            controller.StopAllCoroutines();
            return;
        }

        callCount.SetValue(controller, (int)callCount.GetValue(controller) + 1);
    }

    /// <summary>
    /// Parks every player on the level's final tile so the results screen is not sitting on top of a
    /// level that is still running. scrController.Scrub is the game's own jump-to-tile, used by
    /// practice mode and the editor; it walks the planetary system to the tile and scrubs the music
    /// with it.
    /// </summary>
    private static void TeleportToEndPortal(scrController controller)
    {
        scrLevelMaker lm = ADOBase.lm;
        if (lm == null || lm.listFloors == null || lm.listFloors.Count == 0)
            return;

        int lastFloor = lm.listFloors.Count - 1;

        // Scrub walks floorNum backwards while the tile is freeroam, but never re-reads the tile, so
        // a freeroam final tile would send it all the way down to tile 1. Skip the teleport there.
        if (lm.listFloors[lastFloor] != null && lm.listFloors[lastFloor].freeroam)
        {
            Log("Level ends on a freeroam tile - skipping the teleport to the end portal.");
            return;
        }

        try
        {
            controller.Scrub(lastFloor, forceDontStartMusicFourTilesBefore: true);
            PlacePlayersOnFloor(controller, lm.listFloors[lastFloor]);
        }
        catch (Exception ex)
        {
            LogException("Teleport to the end portal failed", ex);
        }
        finally
        {
            // Scrub pauses the AudioListener before scrubbing the music and only unpauses it inside
            // its "Vfx" object branch, so it can leave the game muted. Undo that unconditionally.
            AudioListener.pause = false;
        }
    }

    /// <summary>
    /// Scrub hands scrPlanet.ScrubToFloorNumber a movePos of "(bool)ADOBase.customLevel ||
    /// RDC.debug", so on an official level it updates the planet's tile without ever moving its
    /// transform - the running game only looks right because the next frames drag the planet along.
    /// Nothing drags it before the level has started, leaving the player sitting on tile 0, so do
    /// the move (and the camera snap Scrub also skips outside debug) ourselves.
    /// </summary>
    private static void PlacePlayersOnFloor(scrController controller, scrFloor floor)
    {
        if (floor == null)
            return;

        Vector3 position = floor.stickToFloor ? floor.transform.position : floor.startPos;

        foreach (scrPlayer player in ADOBase.playerManager)
        {
            if (player == null || player.planetarySystem == null)
                continue;

            scrPlanet chosen = player.planetarySystem.chosenPlanet;
            if (chosen == null)
                continue;

            chosen.transform.position = position;
            chosen.Update_RefreshAngles();
        }

        scrPlanet cameraTarget = controller.playerOne != null && controller.playerOne.planetarySystem != null
            ? controller.playerOne.planetarySystem.chosenPlanet
            : null;

        if (controller.camy != null && cameraTarget != null)
            controller.camy.ViewObjectInstant(cameraTarget.transform);
    }

    /// <summary>
    /// Writes the run through the same Persistence setters scrMistakesManager.Save and SaveCustom
    /// use, but without their "only if it beats the stored value" guards. Those guards mean a stored
    /// accuracy that is higher than the level's true maximum - Persistence.CompleteWorld parks a flat
    /// 1.1f there, and RecoverSaveDataFromAchievements a flat 1f - can never be corrected downwards,
    /// so the hub would keep showing a placeholder next to a gold label. A pure perfect clear is by
    /// definition the level's ceiling, so we assert the values instead of comparing them.
    /// </summary>
    private static void SavePurePerfectResult(scrController controller)
    {
        scrMistakesManager mistakesManager = ADOBase.playerManager.mistakesManager;
        if (mistakesManager == null)
            return;

        if (ADOBase.isCLSLevel)
        {
            if (ADOBase.customLevel == null || ADOBase.customLevel.levelData == null)
                return;

            string hash = ADOBase.customLevel.levelData.Hash;
            Persistence.SetCustomWorldCompletion(hash, 1f);
            Persistence.SetCustomWorldAccuracy(hash, mistakesManager.percentAcc);
            Persistence.SetCustomWorldXAccuracy(hash, mistakesManager.percentXAcc);
            Persistence.SetCustomWorldIsHighestPossibleAcc(hash, isHighest: true);
        }
        else
        {
            // Only boss levels reach Save at all; the numbered levels of a world just bump tutorial
            // progress, and there is no per-level accuracy to write.
            if (!ADOBase.isOfficialLevel || !controller.isbosslevel || controller.isPuzzleRoom ||
                GCS.practiceMode || GCS.d_booth)
                return;

            bool coop = scrController.coopMode;
            int world = scrController.currentWorld;
            Persistence.SetPercentCompletion(world, 1f, coop);
            Persistence.SetBestPercentAccuracy(world, mistakesManager.percentAcc, coop);
            Persistence.SetBestPercentXAccuracy(world, mistakesManager.percentXAcc, coop);
            Persistence.SetIsHighestPossibleAcc(world, isHighest: true, coop);
        }

        // Persistence.Save() normally defers behind a 0.5s coroutine that the level transition can
        // outlive; flush it here so the result is on disk before the scene changes.
        Persistence.Save(instant: true);

        Log("Saved pure perfect: accuracy " + mistakesManager.percentAcc.ToString("0.0000") +
            ", xAccuracy " + mistakesManager.percentXAcc.ToString("0.0000") + ".");
    }

    /// <summary>
    /// The one piece of scrController.BeatLevel worth keeping now that we call OnLandOnPortal
    /// directly: clearing a Taro DLC boss level awards its medals.
    /// </summary>
    private static void GrantTaroMedals(scrController controller)
    {
        if (!scrController.currentWorldString.IsTaro() || !controller.isbosslevel || controller.isPuzzleRoom)
            return;

        int medalCount = WorldData.dict[scrController.currentWorldString].medalCount;
        int[] medals = new int[medalCount];
        for (int i = 0; i < medalCount; i++)
            medals[i] = 3;

        Persistence.SetMedalsForDLCLevel(scrController.currentWorldString, medals);
    }

    // ============================================================
    // Helpers
    // ============================================================

    private static void RunOnMainThread(Action action)
    {
        MainThreadDispatcher.Enqueue(action);
    }

    /// <summary>Tells the waiting trainer that the apply action went through.</summary>
    private static void Succeed()
    {
        SendResponse(ResponseOk);
    }

    /// <summary>
    /// Aborts an apply action and hands the trainer a message to show. The trainer looks the string
    /// up in its own translation table, so it has to match a key in translations.json.
    /// </summary>
    private static void Fail(string message)
    {
        Log("[!] " + message);
        SendResponse(message);
    }

    private static float NormalizeMultiplier(float value)
    {
        if (float.IsNaN(value) || float.IsInfinity(value))
            return 1f;

        return Math.Max(0.01f, value);
    }
}
