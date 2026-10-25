#include "TrainerApp.h"

void register_cheats(TrainerUI &ui)
{
    ui.toggle(
        "God Mode",
        [](Trainer &t, bool enable, const Value &, std::string &)
        { return t.toggleGodMode(enable); }
    );

    ui.toggle(
        "Infinite Health",
        [](Trainer &t, bool enable, const Value &, std::string &)
        { return t.toggleInfiniteHealth(enable); }
    );

    ui.toggle(
        "Unlimited Jumps",
        [](Trainer &t, bool enable, const Value &, std::string &)
        { return t.toggleUnlimitedJumps(enable); }
    );

    ui.toggle(
        "Movement Speed Multiplier", FloatInput("2", "0.1", "10"),
        [](Trainer &t, bool enable, const Value &value, std::string &message)
        { return t.setMovementSpeedMultiplier(enable, value.f(), message); }
    );

    ui.toggle(
        "Set Bolts", IntInput("999999", "0", "999999999"),
        [](Trainer &t, bool enable, const Value &value, std::string &)
        { return t.setBolts(enable, value.i()); }
    );

    ui.toggle(
        "Set Raritanium", IntInput("999", "0", "999999999"),
        [](Trainer &t, bool enable, const Value &value, std::string &)
        { return t.setRaritanium(enable, value.i()); }
    );

    ui.toggle(
        "Set Ammo", IntInput("999", "0", "9999"),
        [](Trainer &t, bool enable, const Value &value, std::string &)
        { return t.setAmmo(enable, value.i()); }
    );
}
