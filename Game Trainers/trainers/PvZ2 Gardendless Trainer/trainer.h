// trainer.h
// PvZ2 Gardendless Trainer — Game-specific cheat functions.
// Uses CDPBase for CDP communication with the Tauri/WebView2 game.
#pragma once

#include "CDPBase.h"

class Trainer : public CDPBase
{
public:
    Trainer() : CDPBase("pvz2_gardenless_trainer", CDPLaunchMethod::WebView2EnvVar) {}
    virtual ~Trainer() {}

    // ============================================================
    // Cheat Functions — In-Level
    // ============================================================

    bool instantWin()
    {
        return executeJS(
            "(() => {"
            "  const cls = (n) => cc.js.getClassByName(n);"
            "  const fc = (n) => cc.director.getScene().getComponentInChildren(cls(n));"
            "  const lp = fc('levelController');"
            "  if (!lp) return 'fail: not in level';"
            "  lp.levelVictoryCountDown = 0;"
            "  lp.victory();"
            "  return 'ok';"
            "})()");
    }

    bool setSun(const std::string &amount)
    {
        return executeJS(
            "(() => {"
            "  const cls = (n) => cc.js.getClassByName(n);"
            "  const c = cls('SunCount').component;"
            "  if (!c) return 'fail: not in level';"
            "  c.setSunCount(" +
            amount +
            ");"
            "  return 'ok';"
            "})()");
    }

    bool setPlantFood(const std::string &count)
    {
        return executeJS(
            "(() => {"
            "  const cls = (n) => cc.js.getClassByName(n);"
            "  const pf = cls('PlantFoodCount').component;"
            "  if (!pf) return 'fail: not in level';"
            "  pf.setPlantFoodNum(" +
            count +
            ");"
            "  return 'ok';"
            "})()");
    }

    bool killAllZombies()
    {
        return executeJS(
            "(() => {"
            "  const cls = (n) => cc.js.getClassByName(n);"
            "  const cm = cc.find('CharacterManager');"
            "  if (!cm) return 'fail: not in level';"
            "  const comp = cm.getComponent(cls('CharacterManager'));"
            "  if (!comp || !comp.zombiePool) return 'fail: no pool';"
            "  let c = 0;"
            "  comp.zombiePool.concat().forEach(z => {"
            "    if (z.isAlive && z.isAlive()) { z.playDie(); c++; }"
            "  });"
            "  return 'killed ' + c;"
            "})()");
    }

    bool unlockAllPlants()
    {
        return executeJS(
            "(() => {"
            "  const cls = (n) => cc.js.getClassByName(n);"
            "  const fc = (n) => cc.director.getScene().getComponentInChildren(cls(n));"
            "  const lc = fc('levelController');"
            "  if (!lc) return 'fail: not in level';"
            "  if (lc.UnlockAll) return 'ok: already unlocked';"
            "  lc.UnlockAll = true;"
            "  const cards = cls('Cards').component;"
            "  if (!cards || !cards.seedChooser) return 'fail: no seed chooser';"
            "  const sc = cards.seedChooser;"
            "  cards.spliceAllCards();"
            "  cards.fromCards.length = 0;"
            "  const parent = sc.ca0.node.parent;"
            "  const toRemove = parent.children.filter(c => c !== sc.ca0.node);"
            "  toRemove.forEach(c => c.destroy());"
            "  sc.ca0.node.off('touch-end');"
            "  sc.CFs.length = 0;"
            "  cards.plantsCanBeChosen = null;"
            "  sc.currentPlantTypes = null;"
            "  sc.layCards();"
            "  sc.CFs.forEach(cf => {"
            "    cf.node.on('touch-end', () => {"
            "      if (cards.shown) cards.addCard(cf, false);"
            "    });"
            "  });"
            "  return 'ok: ' + sc.CFs.length + ' plants';"
            "})()");
    }

    // ============================================================
    // Cheat Functions — Currency (work from any screen)
    // ============================================================

    bool addCoins(const std::string &amount)
    {
        return executeJS(
            "(() => {"
            "  const cls = (n) => cc.js.getClassByName(n);"
            "  const c = cls('CoinCount');"
            "  if (!c || !c.component) return 'fail';"
            "  c.component.addCoinCount(" +
            amount +
            ");"
            "  return 'ok';"
            "})()");
    }

    bool addGems(const std::string &amount)
    {
        return executeJS(
            "(() => {"
            "  const cls = (n) => cc.js.getClassByName(n);"
            "  const c = cls('GemCount');"
            "  if (!c || !c.component) return 'fail';"
            "  c.component.addGemCount(" +
            amount +
            ");"
            "  return 'ok';"
            "})()");
    }

    bool addWorldKeys(const std::string &amount)
    {
        return executeJS(
            "(() => {"
            "  const cls = (n) => cc.js.getClassByName(n);"
            "  const c = cls('WorldKeyCount');"
            "  if (!c || !c.component) return 'fail';"
            "  c.component.addKeyCount(" +
            amount +
            ");"
            "  return 'ok';"
            "})()");
    }
};
