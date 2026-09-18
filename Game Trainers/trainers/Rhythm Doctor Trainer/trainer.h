// trainer.h
#pragma once

#include "MonoBase.h"

class Trainer : public MonoBase
{
public:
    Trainer() : MonoBase(L"Rhythm Doctor.exe") {} // x64
    virtual ~Trainer() {}

    static inline const wchar_t *moduleName = L"mono-2.0-bdwgc.dll";

    bool toggleInfiniteHealth(bool enable, std::string &message)
    {
        return invokeCheat("ToggleInfiniteHealth", {enable}, message);
    }

    bool toggleAutoPlay(bool enable, std::string &message)
    {
        return invokeCheat("ToggleAutoPlay", {enable}, message);
    }

    /// The game speeds the level up, not the whole game, so a disabled toggle is speed 1 rather than
    /// a separate "restore" call.
    bool setGameSpeedMultiplier(bool enable, float value, std::string &message)
    {
        return invokeCheat("SetGameSpeedMultiplier", {enable, value}, message);
    }

    /// The option takes the row number from the table, not the value the game stores ranks as - that
    /// one is sparse and signed (S is 5, S+ is 15, S- is -15), which is no use to type into a box.
    bool instantCompleteLevel(int row, std::string &message)
    {
        if (row < 1 || row > static_cast<int>(ranks().size()))
        {
            message = "Pick a rank from the list.";
            return false;
        }

        return invokeCheat("InstantCompleteLevel", {ranks()[row - 1].first}, message);
    }

    /// Feeds the rank lookup table: one row per line, '>'-separated. The ranks are fixed constants in
    /// the game, so there is nothing to ask the game for.
    std::string rankList()
    {
        std::string rows;
        for (size_t i = 0; i < ranks().size(); ++i)
            rows += std::to_string(i + 1) + ">" + ranks()[i].second + "\n";

        return rows;
    }

private:
    /// Every rank the game defines, best first, paired with the value it stores.
    static const std::vector<std::pair<int, std::string>> &ranks()
    {
        static const std::vector<std::pair<int, std::string>> table = {
            {15, "S+"}, {5, "S"}, {-15, "S-"},
            {14, "A+"}, {4, "A"}, {-14, "A-"},
            {13, "B+"}, {3, "B"}, {-13, "B-"},
            {12, "C+"}, {2, "C"}, {-12, "C-"},
            {11, "D+"}, {1, "D"}, {-11, "D-"},
            {10, "F+"}, {0, "F"}, {-10, "F-"}};
        return table;
    }

    /// Every cheat answers with "OK" or the reason it refused, which the caller passes on as-is.
    bool invokeCheat(const std::string &methodName, const std::vector<Param> &params, std::string &message)
    {
        message.clear();

        if (!initializeDllInjection())
            return false;

        const std::string response = invokeMethodReturn("", "GCMInjection", methodName, params);
        if (response == "OK")
            return true;

        message = response;
        return false;
    }
};
