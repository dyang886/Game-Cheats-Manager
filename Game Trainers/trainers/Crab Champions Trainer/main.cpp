// Crab Champions trainer UI.

#include <FL/Fl.H>
#include <FL/Fl_Flex.H>
#include <FL/Fl_PNG_Image.H>
#include <FL/Fl_Radio_Round_Button.H>
#include <FL/Fl_Table.H>
#include <FL/forms.H>
#include <sstream>
#include "trainer.h"
#include "FLTKUtils.h"

// Version info
static constexpr const char *TRAINER_NAME = "Crab Champions Trainer";
static constexpr const char *GAME_VERSION = "V2347";
static constexpr const char *TRAINER_VERSION = "1.0";

static Fl_Window *g_main_window = nullptr;
static Fl_Window *g_weapon_ability_melee_list_window = nullptr;
static Fl_Window *g_rank_list_window = nullptr;
static Fl_Input *g_rank_input = nullptr;
static Fl_Check_Button *g_apply_rank_to_all_button = nullptr;

// ============================================================
// Callbacks
// ============================================================

void toggle_callback(Fl_Widget *, void *data)
{
    ToggleData *toggleData = static_cast<ToggleData *>(data);
    Trainer *trainer = toggleData->trainer;
    Fl_Check_Button *button = toggleData->button;
    const std::string optionName = toggleData->optionName;
    const bool isEnabled = button->value() != 0;

    if (!trainer->isProcessRunning())
    {
        trainer_alert(t("Please run the game first."));
        button->value(0);
        return;
    }

    std::ostringstream errCapture;
    auto *oldBuf = std::cerr.rdbuf(errCapture.rdbuf());

    bool status = true;
    try
    {
        const std::string val = (toggleData->input && toggleData->input->value()[0]) ? toggleData->input->value() : "0";

        if (optionName == "SetGodMode")
            status = trainer->setGodMode(isEnabled);
        else if (optionName == "SetHealth")
            status = trainer->setHealth(isEnabled, std::stoi(val));
        else if (optionName == "SetMovementSpeedMultiplier")
            status = trainer->setMovementSpeedMultiplier(isEnabled, std::stof(val));
        else if (optionName == "SetWeaponDamageMultiplier")
            status = trainer->setWeaponDamageMultiplier(isEnabled, std::stof(val));
        else if (optionName == "SetAbilityDamageMultiplier")
            status = trainer->setAbilityDamageMultiplier(isEnabled, std::stof(val));
        else if (optionName == "SetMeleeDamageMultiplier")
            status = trainer->setMeleeDamageMultiplier(isEnabled, std::stof(val));
        else if (optionName == "SetCriticalHitChanceMultiplier")
            status = trainer->setCriticalHitChanceMultiplier(isEnabled, std::stof(val));
        else if (optionName == "SetCriticalHitDamageMultiplier")
            status = trainer->setCriticalHitDamageMultiplier(isEnabled, std::stof(val));
        else if (optionName == "SetFiringRateMultiplier")
            status = trainer->setFiringRateMultiplier(isEnabled, std::stof(val));
        else if (optionName == "SetReloadTime")
            status = trainer->setReloadTime(isEnabled, std::stof(val));
        else if (optionName == "SetAmmo")
            status = trainer->setAmmo(isEnabled, std::stoi(val));
        else if (optionName == "SetNoAbilityMeleeCooldown")
            status = trainer->setNoAbilityMeleeCooldown(isEnabled);
        else
            status = false;
    }
    catch (...)
    {
        status = false;
    }

    std::cerr.rdbuf(oldBuf);

    if (!status)
    {
        std::string msg = t("Failed to activate / deactivate.");
        const std::string details = errCapture.str();
        if (!details.empty())
            msg += std::string("\n") + details;
        trainer_alert(msg);
        button->value(isEnabled ? 0 : 1);
    }
    else if (toggleData->input)
    {
        toggleData->input->readonly(isEnabled ? 1 : 0);
    }
}

void apply_callback(Fl_Widget *widget, void *data)
{
    ApplyData *applyData = static_cast<ApplyData *>(data);
    Trainer *trainer = applyData->trainer;
    const std::string optionName = applyData->optionName;

    if (!trainer->isProcessRunning())
    {
        trainer_alert(t("Please run the game first."));
        return;
    }

    std::ostringstream errCapture;
    auto *oldBuf = std::cerr.rdbuf(errCapture.rdbuf());

    bool status = true;

    try
    {
        std::string val = (applyData->input && applyData->input->value()[0]) ? applyData->input->value() : "0";

        if (optionName == "SetCrystals")
            status = trainer->setCrystals(std::stoi(val));
        else if (optionName == "SetKeys")
            status = trainer->setKeys(std::stoi(val));
        else if (optionName == "SetMaxHealth")
            status = trainer->setMaxHealth(std::stoi(val));
        else if (optionName == "SetWeaponAbilityMeleeRank")
        {
            const bool applyToAll =
                g_apply_rank_to_all_button && g_apply_rank_to_all_button->value() != 0;
            const int itemId = applyToAll ? 0 : std::stoi(val);
            const int rank = std::stoi(g_rank_input->value());
            status = trainer->setWeaponAbilityMeleeRank(itemId, rank, applyToAll);
        }
        else if (optionName == "UnlockAllWeapons")
            status = trainer->unlockAllWeapons();
        else if (optionName == "UnlockAllMelee")
            status = trainer->unlockAllMelee();
        else if (optionName == "UnlockAllAbilities")
            status = trainer->unlockAllAbilities();
        else if (optionName == "UnlockAllCosmetics")
            status = trainer->unlockAllCosmetics();
        else
            status = false;
    }
    catch (...)
    {
        status = false;
    }

    std::cerr.rdbuf(oldBuf);

    if (!status)
    {
        std::string msg = t("Failed to activate.");
        std::string details = errCapture.str();
        if (!details.empty())
            msg += std::string("\n") + details;
        trainer_alert(msg);
    }
}

static void check_process_status_wrapper(void *data)
{
    check_process_status(data);
    Fl::remove_timeout(check_process_status, data);
    Fl::repeat_timeout(1.0, check_process_status_wrapper, data);
}

static void update_window_title()
{
    if (!g_main_window)
        return;

    std::string title = std::string(t(TRAINER_NAME)) + " | Game ver." + GAME_VERSION + " | Trainer ver." + TRAINER_VERSION;
    g_main_window->copy_label(title.c_str());
}

static void lang_title_callback(Fl_Widget *widget, void *data)
{
    change_language_callback(widget, data);
    update_window_title();
}

static void main_window_close_callback(Fl_Widget *w, void *)
{
    turn_off_all_toggles();
    if (g_weapon_ability_melee_list_window)
    {
        Fl::delete_widget(g_weapon_ability_melee_list_window);
        g_weapon_ability_melee_list_window = nullptr;
    }
    if (g_rank_list_window)
    {
        Fl::delete_widget(g_rank_list_window);
        g_rank_list_window = nullptr;
    }
    if (font_handle)
        RemoveFontMemResourceEx(font_handle);
    Fl::delete_widget(w);
}

// ============================================================
// Main
// ============================================================
int main(int argc, char **argv)
{
    Trainer trainer;
    setupLanguage();
    load_translations("TRANSLATION_JSON");

    int win_w = 1300;
    int win_h = 600;
    int screen_w = Fl::w();
    int screen_h = Fl::h();
    int win_x = (screen_w - win_w) / 2;
    int win_y = (screen_h - win_h) / 2;

    g_main_window = new Fl_Window(win_x, win_y, win_w, win_h);
    Fl::scheme("gtk+");
    Fl::set_color(FL_FREE_COLOR, 0x1c1c1c00);
    g_main_window->color(FL_FREE_COLOR);
    g_main_window->icon((char *)LoadIconA(GetModuleHandle(NULL), "APP_ICON"));
    g_main_window->callback(main_window_close_callback);

    DWORD font_mem_size = 0;
    DWORD num_fonts = 0;
    const unsigned char *font_data = load_resource("FONT_TTF", font_mem_size);
    font_handle = AddFontMemResourceEx((void *)font_data, font_mem_size, nullptr, &num_fonts);
    Fl::set_font(FL_FREE_FONT, "Noto Sans SC");
    fl_font(FL_FREE_FONT, font_size);

    DWORD info_img_size = 0;
    const unsigned char *info_img_data = load_resource("INFO_IMG", info_img_size);
    Fl_PNG_Image *info_img = new Fl_PNG_Image(
        nullptr,
        info_img_data,
        static_cast<int>(info_img_size));
    info_img->scale(20, 20, 1, 0);

    // ------------------------------------------------------------------
    // Top Row: Language Selection
    // ------------------------------------------------------------------
    int lang_flex_height = 30;
    int lang_flex_width = 200;

    Fl_Flex lang_flex(g_main_window->w() - lang_flex_width, 0, lang_flex_width, lang_flex_height, Fl_Flex::HORIZONTAL);
    lang_flex.color(FL_BLACK);

    Fl_Radio_Round_Button *lang_en = new Fl_Radio_Round_Button(0, 0, 0, 0, "English");
    if (language == "en_US")
        lang_en->set();
    lang_en->labelfont(FL_FREE_FONT);
    lang_en->labelsize(font_size);
    lang_en->labelcolor(FL_WHITE);
    ChangeLanguageData *changeLanguageDataEN = new ChangeLanguageData{"en_US", g_main_window};
    lang_en->callback(lang_title_callback, changeLanguageDataEN);

    Fl_Radio_Round_Button *lang_zh = new Fl_Radio_Round_Button(0, 0, 0, 0, "简体中文");
    if (language == "zh_CN")
        lang_zh->set();
    lang_zh->labelfont(FL_FREE_FONT);
    lang_zh->labelsize(font_size);
    lang_zh->labelcolor(FL_WHITE);
    ChangeLanguageData *changeLanguageDataSC = new ChangeLanguageData{"zh_CN", g_main_window};
    lang_zh->callback(lang_title_callback, changeLanguageDataSC);

    lang_flex.end();

    // ------------------------------------------------------------------
    // Left Column: Image and Process Status
    // ------------------------------------------------------------------
    std::pair<int, int> imageSize = std::make_pair(200, 300);

    DWORD img_size = 0;
    const unsigned char *img_data = load_resource("LOGO_IMG", img_size);
    if (img_data && img_size > 0)
    {
        Fl_PNG_Image *game_img = new Fl_PNG_Image(nullptr, img_data, (int)img_size);
        game_img->scale(imageSize.first, imageSize.second, 1, 0);
        Fl_Box *img_box = new Fl_Box(UI_LEFT_MARGIN, lang_flex_height, imageSize.first, imageSize.second);
        img_box->image(game_img);
    }

    Fl_Box *process_name = new Fl_Box(UI_LEFT_MARGIN, lang_flex_height + imageSize.second + 10, imageSize.first, font_size);
    process_name->align(FL_ALIGN_TOP_LEFT | FL_ALIGN_INSIDE);
    tr(process_name, "Process Name:");

    Fl_Box *process_exe = new Fl_Box(UI_LEFT_MARGIN, lang_flex_height + imageSize.second + font_size + 20, imageSize.first, font_size);
    process_exe->align(FL_ALIGN_TOP_LEFT | FL_ALIGN_INSIDE);

    Fl_Flex *process_id_flex = new Fl_Flex(UI_LEFT_MARGIN, lang_flex_height + imageSize.second + font_size + 55, imageSize.first, font_size, Fl_Flex::HORIZONTAL);
    process_id_flex->gap(5);

    Fl_Box *process_id_label = new Fl_Box(0, 0, 0, 0);
    process_id_label->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    tr(process_id_label, "Process ID:");

    Fl_Box *process_id = new Fl_Box(0, 0, 0, 0);
    process_id->align(FL_ALIGN_TOP_LEFT | FL_ALIGN_INSIDE);

    process_id_flex->end();

    TimeoutData *timeoutData = new TimeoutData{&trainer, process_exe, process_id};
    Fl::add_timeout(0, check_process_status_wrapper, timeoutData);

    // ------------------------------------------------------------------
    // Right Area: Two Option Columns
    // ------------------------------------------------------------------
    int options_x = imageSize.first + UI_LEFT_MARGIN;
    int options_y = lang_flex_height;
    int options_w = g_main_window->w() - options_x;
    int options_h = g_main_window->h() - lang_flex_height;
    Fl_Flex *options_flex = new Fl_Flex(options_x, options_y, options_w, options_h, Fl_Flex::HORIZONTAL);
    options_flex->margin(80, 80, 20, 20);
    options_flex->gap(20);

    Fl_Flex *options1_flex = new Fl_Flex(0, 0, 0, 0, Fl_Flex::VERTICAL);
    options1_flex->gap(8);

    place_toggle_widget(options1_flex, &trainer, "SetGodMode", "God Mode");

    place_toggle_widget(options1_flex, &trainer, "SetHealth", "Set Health", nullptr, "9999", "0", "999999999");

    place_apply_widget(options1_flex, &trainer, "SetMaxHealth", "Set Max Health", nullptr, "9999", "0", "999999999");

    place_toggle_widget(options1_flex, &trainer, "SetWeaponDamageMultiplier", "Weapon Damage Multiplier", nullptr, "10", "0", "99999999999999999999999999999999999999", FL_FLOAT_INPUT);

    place_toggle_widget(options1_flex, &trainer, "SetAbilityDamageMultiplier", "Ability Damage Multiplier", nullptr, "10", "0", "99999999999999999999999999999999999999", FL_FLOAT_INPUT);

    place_toggle_widget(options1_flex, &trainer, "SetMeleeDamageMultiplier", "Melee Damage Multiplier", nullptr, "10", "0", "99999999999999999999999999999999999999", FL_FLOAT_INPUT);

    place_toggle_widget(options1_flex, &trainer, "SetCriticalHitChanceMultiplier", "Critical Hit Chance Multiplier", nullptr, "10", "0", "99999999999999999999999999999999999999", FL_FLOAT_INPUT);

    place_toggle_widget(options1_flex, &trainer, "SetCriticalHitDamageMultiplier", "Critical Hit Damage Multiplier", nullptr, "10", "0", "99999999999999999999999999999999999999", FL_FLOAT_INPUT);

    place_toggle_widget(options1_flex, &trainer, "SetFiringRateMultiplier", "Firing Rate Multiplier", nullptr, "5", "0.01", "99999999999999999999999999999999999999", FL_FLOAT_INPUT);

    place_toggle_widget(options1_flex, &trainer, "SetReloadTime", "Reload Time", nullptr, "0", "0", "999999999", FL_FLOAT_INPUT);

    place_toggle_widget(options1_flex, &trainer, "SetAmmo", "Set Ammo", nullptr, "999", "0", "999999999");

    place_toggle_widget(options1_flex, &trainer, "SetMovementSpeedMultiplier", "Movement Speed Multiplier", nullptr, "5", "0.01", "99999999999999999999999999999999999999", FL_FLOAT_INPUT);

    place_toggle_widget(options1_flex, &trainer, "SetNoAbilityMeleeCooldown", "No Ability / Melee Cooldown");

    Fl_Box *options1_spacer = new Fl_Box(0, 0, 0, 0);
    options1_flex->end();

    Fl_Flex *options2_flex = new Fl_Flex(0, 0, 0, 0, Fl_Flex::VERTICAL);
    options2_flex->gap(8);

    place_apply_widget(options2_flex, &trainer, "SetCrystals", "Set Crystals", nullptr, "99999", "0", "999999999");

    place_apply_widget(options2_flex, &trainer, "SetKeys", "Set Keys", nullptr, "999", "0", "999999999");

    Fl_Input *weaponAbilityMeleeInput = nullptr;
    const std::vector<std::string> weaponAbilityMeleeColumns = {"Item ID", "Type", "Name", "Current Rank"};
    const std::vector<int> weaponAbilityMeleeColumnWidths = {70, 80, 200, 110};
    Fl_Button *weaponAbilityMeleeInfo = create_info_button(&trainer, &weaponAbilityMeleeInput, weaponAbilityMeleeColumns, weaponAbilityMeleeColumnWidths, "Weapon / Ability / Melee List", &g_weapon_ability_melee_list_window, &Trainer::getWeaponAbilityMeleeList, info_img);
    place_apply_widget(options2_flex, &trainer, "SetWeaponAbilityMeleeRank", "Set Weapon / Ability Rank", &weaponAbilityMeleeInput, "0", "0", "999999999", FL_INT_INPUT, weaponAbilityMeleeInfo);

    const std::vector<std::string> rankColumns = {"Rank ID", "Rank"};
    const std::vector<int> rankColumnWidths = {100, 234};
    Fl_Button *rankInfo = create_info_button(&trainer, &g_rank_input, rankColumns, rankColumnWidths, "Rank List", &g_rank_list_window, &Trainer::getRankList, info_img);
    g_rank_input = place_indented_input_widget(options2_flex, "Rank", "1", "1", "8", FL_INT_INPUT, rankInfo);

    g_apply_rank_to_all_button = dynamic_cast<Fl_Check_Button *>(place_indented_toggle_widget(options2_flex, "Apply to All Weapons / Abilities / Melee").button);

    place_apply_widget(options2_flex, &trainer, "UnlockAllWeapons", "Unlock All Weapons (Non Permanent)");

    place_apply_widget(options2_flex, &trainer, "UnlockAllMelee", "Unlock All Melee (Non Permanent)");

    place_apply_widget(options2_flex, &trainer, "UnlockAllAbilities", "Unlock All Abilities (Non Permanent)");

    place_apply_widget(options2_flex, &trainer, "UnlockAllCosmetics", "Unlock All Cosmetics (Non Permanent)");

    Fl_Box *options2_spacer = new Fl_Box(0, 0, 0, 0);
    options2_flex->end();

    options_flex->end();

    change_language(language, g_main_window);
    update_window_title();

    g_main_window->end();
    g_main_window->show(argc, argv);
    return Fl::run();
}
