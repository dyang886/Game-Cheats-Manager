// trainer.h
// Zao Meng Xi You: Wu Shuang Trainer — Game-specific cheat functions.
// Uses CDPBase for CDP communication with the Electron/Cocos Creator game.
#pragma once

#include "CDPBase.h"

class Trainer : public CDPBase
{
public:
    // SteamLaunchUrl: fires steam://run/3867950//--remote-debugging-port=9222/ so Steam
    // passes the flag to the inner Electron process, bypassing the launcher wrapper exe
    // that rejects --remote-debugging-port as an unknown argument.
    Trainer() : CDPBase("zmws_trainer", CDPLaunchMethod::SteamLaunchUrl, "3867950") {}
    virtual ~Trainer() {}

    // Navigate to the fightScene's gameLogic and collect all party heroes.
    // Returns null/undefined if not currently in a battle.
    static constexpr const char *kGetHeroes =
        "  const cls = (n) => cc.js.getClassByName(n);"
        "  const fs = cc.director.getScene().getComponentInChildren(cls('fightScene'));"
        "  if (!fs) return 'fail: Please use this in battle.';"
        "  const gl = fs.mGameLogic;"
        "  if (!gl) return 'fail: Game logic not found.';"
        "  const heroes = [];"
        "  for (let i = 0; i < 8; i++) {"
        "    const h = gl.getMainPlayer(i);"
        "    if (!h || !h.mProp) break;"
        "    heroes.push(h);"
        "  }";

    // ============================================================
    // Cheat Functions
    // ============================================================

    // Toggles "always full" HP for all party heroes.
    // Uses mLockHp — the game's own flag that blocks setHp — to prevent the damage
    // system from reducing HP between interval ticks. The interval re-locks on every
    // tick in case the game resets the flag (e.g. on entering a new battle state).
    // kGetHeroes early-returns are scoped inside the inner `fill` arrow function so
    // they don't abort the outer IIFE when not in battle.
    bool toggleInfiniteHp(bool enable)
    {
        if (enable)
            return executeJS(
                "(() => {"
                "  if (window.__zmws_hpTimer) clearInterval(window.__zmws_hpTimer);"
                "  const fill = () => { try {"
                + std::string(kGetHeroes) +
                "    heroes.forEach(h => {"
                "      h.mProp.mLockHp = false;"
                "      h.mProp.setHp(h.mProp.getMaxHp());"
                "      h.mProp.mLockHp = true;"
                "    });"
                "  } catch(e) {} };"
                "  fill();"
                "  window.__zmws_hpTimer = setInterval(fill, 200);"
                "  return 'ok';"
                "})()");
        else
            return executeJS(
                "(() => {"
                "  if (window.__zmws_hpTimer) { clearInterval(window.__zmws_hpTimer); window.__zmws_hpTimer = null; }"
                "  (function() { try {"
                + std::string(kGetHeroes) +
                "    heroes.forEach(h => { h.mProp.mLockHp = false; });"
                "  } catch(e) {} })();"
                "  return 'ok';"
                "})()");
    }

    bool toggleInfiniteMp(bool enable)
    {
        if (enable)
            return executeJS(
                "(() => {"
                "  if (window.__zmws_mpTimer) clearInterval(window.__zmws_mpTimer);"
                "  const fill = () => { try {"
                + std::string(kGetHeroes) +
                "    heroes.forEach(h => {"
                "      h.mProp.mLockMp = false;"
                "      h.mProp.setMp(h.mProp.getMaxMp());"
                "      h.mProp.mLockMp = true;"
                "    });"
                "  } catch(e) {} };"
                "  fill();"
                "  window.__zmws_mpTimer = setInterval(fill, 200);"
                "  return 'ok';"
                "})()");
        else
            return executeJS(
                "(() => {"
                "  if (window.__zmws_mpTimer) { clearInterval(window.__zmws_mpTimer); window.__zmws_mpTimer = null; }"
                "  (function() { try {"
                + std::string(kGetHeroes) +
                "    heroes.forEach(h => { h.mProp.mLockMp = false; });"
                "  } catch(e) {} })();"
                "  return 'ok';"
                "})()");
    }

    // Uses both mSkillFather (game-level invincibility flag) and mLockHp (blocks
    // setHp calls) for double protection. mSkillFather alone can be transiently
    // reset by attack animations, leaving a brief damage window; mLockHp closes
    // that window by preventing HP from changing at all.
    bool toggleInvincibilityMode(bool enable)
    {
        if (enable)
            return executeJS(
                "(() => {"
                "  if (window.__zmws_invTimer) clearInterval(window.__zmws_invTimer);"
                "  const apply = () => { try {"
                + std::string(kGetHeroes) +
                "    heroes.forEach(h => {"
                "      h.mProp.mLockHp = true;"
                "      h.mSkillFather = true;"
                "      if (h.updateFatherState) h.updateFatherState();"
                "    });"
                "  } catch(e) {} };"
                "  apply();"
                "  window.__zmws_invTimer = setInterval(apply, 200);"
                "  return 'ok';"
                "})()");
        else
            return executeJS(
                "(() => {"
                "  if (window.__zmws_invTimer) { clearInterval(window.__zmws_invTimer); window.__zmws_invTimer = null; }"
                "  (function() { try {"
                + std::string(kGetHeroes) +
                "    heroes.forEach(h => {"
                "      h.mProp.mLockHp = false;"
                "      h.mSkillFather = false;"
                "      if (h.updateFatherState) h.updateFatherState();"
                "    });"
                "  } catch(e) {} })();"
                "  return 'ok';"
                "})()");
    }

    // ---- Prototype helpers shared by damage-multiplier ----

    // Finds the entityProp prototype via a live hero instance (requires being in
    // battle on first call) and caches it on window.__zmws_entityPropProto.
    static constexpr const char *kGetOrFindProto =
        "  const proto = window.__zmws_entityPropProto || (() => {"
        "    const cls = (n) => cc.js.getClassByName(n);"
        "    const sc = cc.director.getScene();"
        "    if (!sc) return null;"
        "    const fs = sc.getComponentInChildren(cls('fightScene'));"
        "    if (!fs || !fs.mGameLogic) return null;"
        "    const h0 = fs.mGameLogic.getMainPlayer(0);"
        "    return (h0 && h0.mProp) ? h0.mProp.constructor.prototype : null;"
        "  })();"
        "  if (!proto) return 'fail: Please enable this in battle first.';"
        "  window.__zmws_entityPropProto = proto;";

    // Patches countHp to multiply incoming damage only for player hero entities.
    // Checks window.__zmws_heroSet (a WeakSet of hero mProp instances, maintained
    // by a separate interval) so enemies sharing the same prototype are unaffected.
    // Idempotent: no-op if the patch is already installed.
    static constexpr const char *kInstallCountHpPatch =
        "  if (!proto.__zmws_origCountHp) {"
        "    proto.__zmws_origCountHp = proto.countHp;"
        "    proto.countHp = function(e) {"
        "      if (e < 0) {"
        "        const m = window.__zmws_dmgMult;"
        "        if (m && m !== 1 && window.__zmws_heroSet && window.__zmws_heroSet.has(this))"
        "          e *= m;"
        "      }"
        "      return this.__zmws_origCountHp(e);"
        "    };"
        "  }";

    // Restores the original countHp when the multiplier var is cleared.
    static constexpr const char *kMaybeUninstallPatch =
        "  if (!window.__zmws_dmgMult) {"
        "    const _p = window.__zmws_entityPropProto;"
        "    if (_p && _p.__zmws_origCountHp) {"
        "      _p.countHp = _p.__zmws_origCountHp;"
        "      delete _p.__zmws_origCountHp;"
        "    }"
        "    delete window.__zmws_entityPropProto;"
        "  }";

    // ---- Cheat functions ----

    // Multiplies damage received by player heroes only.
    // A 500ms interval keeps window.__zmws_heroSet current so the countHp patch
    // can distinguish hero mProp instances from enemy ones. First enable must be
    // done in battle (proto discovery requires a live hero); subsequent
    // enables/disables work from any scene.
    bool toggleDamageReceivedMultiplier(bool enable, const std::string &multiplier = "1")
    {
        if (enable)
            return executeJS(
                "(() => {"
                "  const val = parseFloat(" + multiplier + ");"
                "  if (isNaN(val) || val < 1) return 'fail: Invalid multiplier.';"
                + std::string(kGetOrFindProto)
                + std::string(kInstallCountHpPatch) +
                "  window.__zmws_dmgMult = val;"
                "  if (window.__zmws_dmgHeroTimer) clearInterval(window.__zmws_dmgHeroTimer);"
                "  const updateHeroSet = () => { try {"
                "    const _cls = (n) => cc.js.getClassByName(n);"
                "    const _sc = cc.director.getScene();"
                "    if (!_sc) return;"
                "    const _fs = _sc.getComponentInChildren(_cls('fightScene'));"
                "    if (!_fs || !_fs.mGameLogic) return;"
                "    const hs = new WeakSet();"
                "    for (let i = 0; i < 8; i++) {"
                "      const h = _fs.mGameLogic.getMainPlayer(i);"
                "      if (!h || !h.mProp) break;"
                "      hs.add(h.mProp);"
                "    }"
                "    window.__zmws_heroSet = hs;"
                "  } catch(e) {} };"
                "  updateHeroSet();"
                "  window.__zmws_dmgHeroTimer = setInterval(updateHeroSet, 500);"
                "  return 'ok';"
                "})()");
        else
            return executeJS(
                "(() => {"
                "  delete window.__zmws_dmgMult;"
                "  if (window.__zmws_dmgHeroTimer) { clearInterval(window.__zmws_dmgHeroTimer); window.__zmws_dmgHeroTimer = null; }"
                "  delete window.__zmws_heroSet;"
                + std::string(kMaybeUninstallPatch) +
                "  return 'ok';"
                "})()");
    }

    // Boosts each hero's ATK stat by the given multiplier via a 200ms interval.
    // Stores the original ATK in a WeakMap on first encounter so it can be
    // restored when the toggle is turned off. Enemies use a separate damage class
    // (entityDamage), so boosting hero ATK is the correct way to increase player
    // outgoing damage.
    bool toggleAttackDamageMultiplier(bool enable, const std::string &multiplier = "1")
    {
        if (enable)
            return executeJS(
                "(() => {"
                "  const mult = parseFloat(" + multiplier + ");"
                "  if (isNaN(mult) || mult < 1) return 'fail: Invalid multiplier.';"
                "  if (window.__zmws_atkTimer) clearInterval(window.__zmws_atkTimer);"
                "  if (!window.__zmws_heroBaseAtk) window.__zmws_heroBaseAtk = new WeakMap();"
                "  window.__zmws_atkMult = mult;"
                "  const apply = () => { try {"
                + std::string(kGetHeroes) +
                "    heroes.forEach(h => {"
                "      const mp = h.mProp;"
                "      if (!window.__zmws_heroBaseAtk.has(mp)) {"
                "        const base = typeof mp.getAtk === 'function' ? mp.getAtk() : mp.mData.atk;"
                "        window.__zmws_heroBaseAtk.set(mp, base);"
                "      }"
                "      const base = window.__zmws_heroBaseAtk.get(mp);"
                "      if (base != null) {"
                "        const boosted = base * window.__zmws_atkMult;"
                "        if (typeof mp.setAtk === 'function') mp.setAtk(boosted);"
                "        else mp.mData.atk = boosted;"
                "      }"
                "    });"
                "  } catch(e) {} };"
                "  apply();"
                "  window.__zmws_atkTimer = setInterval(apply, 200);"
                "  return 'ok';"
                "})()");
        else
            return executeJS(
                "(() => {"
                "  if (window.__zmws_atkTimer) { clearInterval(window.__zmws_atkTimer); window.__zmws_atkTimer = null; }"
                "  (function() { try {"
                + std::string(kGetHeroes) +
                "    if (window.__zmws_heroBaseAtk) {"
                "      heroes.forEach(h => {"
                "        const mp = h.mProp;"
                "        const base = window.__zmws_heroBaseAtk.get(mp);"
                "        if (base != null) {"
                "          if (typeof mp.setAtk === 'function') mp.setAtk(base);"
                "          else mp.mData.atk = base;"
                "        }"
                "      });"
                "    }"
                "  } catch(e) {} })();"
                "  delete window.__zmws_heroBaseAtk;"
                "  delete window.__zmws_atkMult;"
                "  return 'ok';"
                "})()");
    }
};
