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
            "  if (!lp) return 'fail: Please use this in a level.';"
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
            "  if (!c) return 'fail: Please use this in a level.';"
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
            "  if (!pf) return 'fail: Please use this in a level.';"
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
            "  if (!cm) return 'fail: Please use this in a level.';"
            "  const comp = cm.getComponent(cls('CharacterManager'));"
            "  if (!comp || !comp.zombiePool) return 'fail: Please use this in a level.';"
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
            "  if (!lc) return 'fail: Please use this on the plant selection screen.';"
            "  if (lc.UnlockAll) return 'ok: already unlocked';"
            "  lc.UnlockAll = true;"
            "  const cards = cls('Cards').component;"
            "  if (!cards || !cards.seedChooser) return 'fail: Please use this on the plant selection screen.';"
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

    bool setPlantBoost(const std::string &amount)
    {
        return executeJS(
            "(() => {"
            "  try {"
            "  const cls = n => cc.js.getClassByName(n);"
            "  const APP = window.__APP || (window.__APP = System.get("
            "    'chunks:///_virtual/PlayerProperties.ts').AllPlayerProperties);"
            "  const Plants = System.get('chunks:///_virtual/Plants.ts').plants;"
            "  const val = parseInt(" +
            amount +
            ");"
            "  if (isNaN(val) || val < 0) return 'fail: Invalid amount.';"
            "  const cards = cls('Cards').component;"
            "  if (!cards || !cards.seedChooser) return 'fail: Please use this on the plant selection screen.';"
            "  const allCFs = cc.director.getScene().getComponentsInChildren(cls('CardFeatureTotal'));"
            "  if (!allCFs || !allCFs.length) return 'fail: No plant cards found.';"
            "  let c = 0;"
            "  allCFs.forEach(cf => {"
            "    if (!cf.pID && cf.pID !== 0) return;"
            "    const p = APP.getPlantProgressByID(cf.pID);"
            "    if (p) {"
            "      p.boost = val;"
            "      cf._PP_BOOSTED = val > 0;"
            "      if (cf.ca && cf.ca._bgDB) {"
            "        const bg = Plants.getBGAnm(cf.PF, cf.BOOSTED);"
            "        cf.ca.golden = bg.golden;"
            "        cf.ca._bgDB.playAnimation(bg.bg);"
            "      }"
            "      c++;"
            "    }"
            "  });"
            "  APP.savePP();"
            "  return 'ok: set boost to ' + val + ' for ' + c + ' plants';"
            "  } catch(e) { return 'fail: ' + e.message; }"
            "})()");
    }

    // ============================================================
    // Cheat Functions — Zen Garden
    // ============================================================

    bool plantAllGardenPlants()
    {
        return executeJS(
            "(() => {"
            "  try {"
            "  const cls = n => cc.js.getClassByName(n);"
            "  const APP = window.__APP || (window.__APP = System.get("
            "    'chunks:///_virtual/PlayerProperties.ts').AllPlayerProperties);"
            "  const zgs = cc.director.getScene().getComponentInChildren(cls('zenGardenScene'));"
            "  if (!zgs) return 'fail: Please use this in the Zen Garden.';"
            "  const ZG = cls('ZenGarden');"
            "  const obtained = APP.getObtainedPlantIDs();"
            "  if (!obtained || !obtained.IDs.length) return 'fail: No plants obtained yet.';"
            "  const gardens = zgs.node.getComponentsInChildren(ZG);"
            "  let c = 0;"
            "  gardens.forEach(g => g.slots.forEach(s => {"
            "    if (!s.plant && !s.plantAnmControl) {"
            "      const valid = obtained.IDs.filter(id => ZG.judgePlantableInSlot(s, id));"
            "      if (valid.length) {"
            "        s.addPlant(valid[Math.floor(Math.random() * valid.length)], g.ID);"
            "        c++;"
            "      }"
            "    }"
            "  }));"
            "  return 'ok: planted ' + c;"
            "  } catch(e) { return 'fail: ' + e.message; }"
            "})()");
    }

    bool matureAllGardenPlants()
    {
        return executeJS(
            "(() => {"
            "  try {"
            "  const cls = n => cc.js.getClassByName(n);"
            "  const APP = window.__APP || (window.__APP = System.get("
            "    'chunks:///_virtual/PlayerProperties.ts').AllPlayerProperties);"
            "  const zgs = cc.director.getScene().getComponentInChildren(cls('zenGardenScene'));"
            "  if (!zgs) return 'fail: Please use this in the Zen Garden.';"
            "  const FULL = 86400000;"
            "  const gardens = zgs.node.getComponentsInChildren(cls('ZenGarden'));"
            "  let c = 0;"
            "  gardens.forEach(g => {"
            "    g.slots.forEach(s => {"
            "      if (!s.plant || s.plant.grownTime >= FULL) return;"
            "      s.plant.grownTime = FULL + 1;"
            "      s.plant.stuck = false;"
            "      if (s.requirePar) { s.requirePar.node.destroy(); s.requirePar = null; }"
            "      if (s.sprout) { s.sprout.node.destroy(); s.sprout = null; }"
            "      if (s.plantAnmControl) { s.plantAnmControl.node.destroy(); s.plantAnmControl = null; }"
            "      s.showPlant(g.PlantLayer);"
            "      c++;"
            "    });"
            "    g.clearSibling();"
            "  });"
            "  APP.savePP();"
            "  return 'ok: matured ' + c;"
            "  } catch(e) { return 'fail: ' + e.message; }"
            "})()");
    }

    bool shovelAllGardenPlants()
    {
        return executeJS(
            "(() => {"
            "  try {"
            "  const cls = n => cc.js.getClassByName(n);"
            "  const APP = window.__APP || (window.__APP = System.get("
            "    'chunks:///_virtual/PlayerProperties.ts').AllPlayerProperties);"
            "  const zgs = cc.director.getScene().getComponentInChildren(cls('zenGardenScene'));"
            "  if (!zgs) return 'fail: Please use this in the Zen Garden.';"
            "  const FULL = 86400000;"
            "  const gardens = zgs.node.getComponentsInChildren(cls('ZenGarden'));"
            "  let c = 0;"
            "  gardens.forEach(g => g.slots.forEach(s => {"
            "    if (s.plant) {"
            "      const stage = Math.floor(4 * s.plant.grownTime / FULL);"
            "      if (!s.plant.stuck && stage >= 3) APP.getPlantProgressByID(s.plant.ID).boost++;"
            "      s.removePlant();"
            "      c++;"
            "    }"
            "  }));"
            "  return 'ok: shoveled ' + c;"
            "  } catch(e) { return 'fail: ' + e.message; }"
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
            "  if (!c || !c.component) return 'fail: Coin counter not available.';"
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
            "  if (!c || !c.component) return 'fail: Gem counter not available.';"
            "  c.component.addGemCount(" +
            amount +
            ");"
            "  return 'ok';"
            "})()");
    }

    bool addSprouts(const std::string &amount)
    {
        return executeJS(
            "(() => {"
            "  const cls = (n) => cc.js.getClassByName(n);"
            "  const c = cls('SproutCount');"
            "  if (!c || !c.component) return 'fail: Sprout counter not available.';"
            "  c.component.addSproutCount(" +
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
            "  if (!c || !c.component) return 'fail: World key counter not available.';"
            "  c.component.addKeyCount(" +
            amount +
            ");"
            "  return 'ok';"
            "})()");
    }
};
