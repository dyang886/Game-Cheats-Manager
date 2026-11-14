#include "TrainerApp.h"

void register_cheats(TrainerUI &ui)
{
    ui.toggle(
        "Set Coins", IntInput("999999", "0", "999999999"),
        [](Trainer &t, bool enable, const Value &value, std::string &message)
        { return t.setCoins(enable, value.i(), message); }
    );

    ui.toggle(
        "Set Dynamite", IntInput("99", "0", "99999"),
        [](Trainer &t, bool enable, const Value &value, std::string &message)
        { return t.setDynamite(enable, value.i(), message); }
    );

    ui.toggle(
        "Set C4", IntInput("99", "0", "99999"),
        [](Trainer &t, bool enable, const Value &value, std::string &message)
        { return t.setC4(enable, value.i(), message); }
    );

    ui.toggle(
        "Set Vacuum Capacity", IntInput("99999", "1", "999999999"),
        [](Trainer &t, bool enable, const Value &value, std::string &message)
        { return t.setVacuumCapacity(enable, value.i(), message); }
    );

    ui.apply(
        "Clear Vacuum",
        [](Trainer &t, const Value &, std::string &message)
        { return t.clearVacuum(message); }
    );

    ui.toggle(
        "No Debris",
        [](Trainer &t, bool enable, const Value &, std::string &message)
        { return t.toggleNoDebris(enable, message); }
    );

    ui.toggle(
        "No Charge Cooldown",
        [](Trainer &t, bool enable, const Value &, std::string &message)
        { return t.toggleNoChargeCooldown(enable, message); }
    );

    ui.toggle(
        "Shredder Payouts Multiplier", FloatInput("5", "1", "100"),
        [](Trainer &t, bool enable, const Value &value, std::string &message)
        { return t.setShredderPayoutsMultiplier(enable, value.f(), message); }
    );
}
