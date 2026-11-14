#include "TrainerApp.h"

void register_cheats(TrainerUI &ui)
{
    {
        static SubToggle coin1, coin2, coin3;

        ui.toggle(
            "Auto Pickup Coins",
            [](Trainer &t, bool enable, const Value &, std::string &message)
            { return t.toggleAutoPickupCoins(enable, coin1.on(), coin2.on(), coin3.on(), message); }
        );

        coin1 = ui.subToggle("Coin 1", true);
        coin2 = ui.subToggle("Coin 2", true);
        coin3 = ui.subToggle("Coin 3", true);
    }

    ui.toggle(
        "Instant Complete",
        [](Trainer &t, bool enable, const Value &, std::string &)
        { return t.toggleInstantComplete(enable); }
    );

    ui.toggle(
        "Jump Hack",
        [](Trainer &t, bool enable, const Value &, std::string &)
        { return t.toggleJumpHack(enable); }
    );

    ui.toggle(
        "Noclip",
        [](Trainer &t, bool enable, const Value &, std::string &)
        { return t.toggleNoclip(enable); }
    );

    {
        static SubToggle songs, sfx;

        ui.toggle(
            "Song Trash Can",
            [](Trainer &t, bool enable, const Value &, std::string &message)
            { return t.toggleSongTrashCan(enable, songs.on(), sfx.on(), message); }
        );

        songs = ui.subToggle("Songs", true);
        sfx = ui.subToggle("SFX", true);
    }
}
