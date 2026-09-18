#include "TrainerApp.h"

void register_cheats(TrainerUI &ui)
{
    ui.toggle(
        "Infinite Health",
        [](Trainer &t, bool enable, const Value &, std::string &message)
        { return t.toggleInfiniteHealth(enable, message); }
    );

    ui.toggle(
        "Auto Play",
        [](Trainer &t, bool enable, const Value &, std::string &message)
        { return t.toggleAutoPlay(enable, message); },
        "Auto Play Info"
    );

    ui.toggle(
        "Game Speed Multiplier", FloatInput("1", "0.1", "10"),
        [](Trainer &t, bool enable, const Value &value, std::string &message)
        { return t.setGameSpeedMultiplier(enable, value.f(), message); },
        "Game Speed Info"
    );

    ui.apply(
        "Instant Complete Level", IntInput("2", "1", "18"),
        [](Trainer &t, const Value &value, std::string &message)
        { return t.instantCompleteLevel(value.i(), message); },
        InfoTable("Rank List", {"ID", "Rank"}, {100, 234}, &Trainer::rankList)
    );
}
