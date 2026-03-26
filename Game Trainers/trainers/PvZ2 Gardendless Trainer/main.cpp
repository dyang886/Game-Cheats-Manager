// Uses CDP (Chrome DevTools Protocol) to inject JavaScript into the game's WebView2.
// The game must be launched through the trainer so the remote debugging port is enabled.

#include <FL/Fl.H>
#include <FL/Fl_Radio_Round_Button.H>
#include <FL/Fl_PNG_Image.H>
#include <FL/Fl_Flex.H>
#include <FL/Fl_Table.H>
#include <FL/forms.H>
#include <sstream>
#include "trainer.h"
#include "FLTKUtils.h"

// Version info
static constexpr const char *TRAINER_NAME = "PvZ2 Gardendless Trainer";
static constexpr const char *GAME_VERSION = "0.7.1";
static constexpr const char *TRAINER_VERSION = "3.0";

// Forward declarations for custom widgets
static Fl_Input *g_game_path_input = nullptr;
static Fl_Window *g_main_window = nullptr;

// Struct for launch callback data
struct LaunchData
{
    Trainer *trainer;
    Fl_Input *pathInput;
};

// ============================================================
// Callbacks
// ============================================================

/// Apply callback — called by all "Apply" buttons via FLTKUtils.h
void apply_callback(Fl_Widget *widget, void *data)
{
    ApplyData *applyData = static_cast<ApplyData *>(data);
    Trainer *trainer = applyData->trainer;
    std::string optionName = applyData->optionName;
    Fl_Input *input = applyData->input;

    if (!trainer->isProcessRunning())
    {
        fl_alert(t("Please run the game first."));
        return;
    }

    // Capture cerr during trainer call to get specific error details
    std::ostringstream errCapture;
    auto *oldBuf = std::cerr.rdbuf(errCapture.rdbuf());

    bool status = true;
    std::string val = (input && input->value()[0]) ? input->value() : "0";

    if (optionName == "InstantWin")
        status = trainer->instantWin();
    else if (optionName == "SetSun")
        status = trainer->setSun(val);
    else if (optionName == "SetPlantFood")
        status = trainer->setPlantFood(val);
    else if (optionName == "KillAllZombies")
        status = trainer->killAllZombies();
    else if (optionName == "UnlockAllPlants")
        status = trainer->unlockAllPlants();
    else if (optionName == "MatureAllGardenPlants")
        status = trainer->matureAllGardenPlants();
    else if (optionName == "PlantAllGardenPlants")
        status = trainer->plantAllGardenPlants();
    else if (optionName == "ShovelAllGardenPlants")
        status = trainer->shovelAllGardenPlants();
    else if (optionName == "AddSprouts")
        status = trainer->addSprouts(val);
    else if (optionName == "SetPlantBoost")
        status = trainer->setPlantBoost(val);
    else if (optionName == "AddCoins")
        status = trainer->addCoins(val);
    else if (optionName == "AddGems")
        status = trainer->addGems(val);
    else if (optionName == "AddWorldKeys")
        status = trainer->addWorldKeys(val);

    std::cerr.rdbuf(oldBuf);

    if (!status)
    {
        std::string msg = t("Failed to activate.");
        std::string details = errCapture.str();
        if (!details.empty())
            msg += std::string("\n") + details;
        fl_alert("%s", msg.c_str());
    }
}

/// Toggle callback — required by FLTKUtils.h forward declaration but unused
void toggle_callback(Fl_Widget *widget, void *data) {}

/// Browse for game executable
static bool browseDialogOpen = false;

static void browse_callback(Fl_Widget *widget, void *data)
{
    if (browseDialogOpen)
        return;
    browseDialogOpen = true;

    LaunchData *launchData = static_cast<LaunchData *>(data);

    wchar_t wpath[MAX_PATH] = {};
    const char *current = launchData->pathInput->value();
    if (current && current[0])
        MultiByteToWideChar(CP_UTF8, 0, current, -1, wpath, MAX_PATH);

    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = nullptr;
    ofn.lpstrFilter = L"Executables (*.exe)\0*.exe\0All Files\0*.*\0";
    ofn.lpstrFile = wpath;
    ofn.nMaxFile = MAX_PATH;
    wchar_t wtitle[256] = {};
    MultiByteToWideChar(CP_UTF8, 0, t("Select Game Executable"), -1, wtitle, 256);
    ofn.lpstrTitle = wtitle;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

    if (GetOpenFileNameW(&ofn))
    {
        char utf8[MAX_PATH * 3] = {};
        WideCharToMultiByte(CP_UTF8, 0, wpath, -1, utf8, sizeof(utf8), nullptr, nullptr);
        launchData->pathInput->value(utf8);
        launchData->trainer->gamePath = utf8;
        launchData->trainer->saveSettings();
    }

    browseDialogOpen = false;
}

/// Launch game with CDP enabled
static void launch_callback(Fl_Widget *widget, void *data)
{
    LaunchData *launchData = static_cast<LaunchData *>(data);
    Trainer *trainer = launchData->trainer;

    // Update game path from input field
    if (launchData->pathInput && launchData->pathInput->value())
        trainer->gamePath = launchData->pathInput->value();

    if (trainer->gamePath.empty())
    {
        fl_alert(t("Please set the game path first."));
        return;
    }

    if (!trainer->launchGame())
        fl_alert(t("Failed to launch game."));
}

/// Wraps check_process_status to re-assert game path readonly after clean_up.
/// Also re-registers itself since check_process_status uses repeat_timeout internally.
static void check_process_status_wrapper(void *data)
{
    // Remove the repeat that check_process_status will schedule
    check_process_status(data);
    Fl::remove_timeout(check_process_status, data);

    if (g_game_path_input)
        g_game_path_input->readonly(1);

    Fl::repeat_timeout(1.0, check_process_status_wrapper, data);
}

static void update_window_title()
{
    if (!g_main_window)
        return;
    std::string title = std::string(t(TRAINER_NAME)) + " | Game v" + GAME_VERSION + " | Trainer v" + TRAINER_VERSION;
    g_main_window->copy_label(title.c_str());
}

static void lang_title_callback(Fl_Widget *widget, void *data)
{
    change_language_callback(widget, data);
    update_window_title();
}

static void main_window_close_callback(Fl_Widget *w, void *)
{
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

    // Create the main window
    int win_w = 800;
    int win_h = 650;
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

    // Setup fonts
    DWORD font_mem_size = 0;
    DWORD num_fonts = 0;
    const unsigned char *font_data = load_resource("FONT_TTF", font_mem_size);
    font_handle = AddFontMemResourceEx((void *)font_data, font_mem_size, nullptr, &num_fonts);
    Fl::set_font(FL_FREE_FONT, "Noto Sans SC");
    fl_font(FL_FREE_FONT, font_size);

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
    // Left Column: Image and CDP Status
    // ------------------------------------------------------------------
    std::pair<int, int> imageSize = std::make_pair(200, 300);

    DWORD img_size = 0;
    const unsigned char *img_data = load_resource("LOGO_IMG", img_size);
    if (img_data && img_size > 0)
    {
        Fl_PNG_Image *game_img = new Fl_PNG_Image(nullptr, img_data, (int)img_size);
        game_img->scale(imageSize.first, imageSize.second, 1, 0);
        Fl_Box *img_box = new Fl_Box(20, lang_flex_height, imageSize.first, imageSize.second);
        img_box->image(game_img);
    }

    // CDP Status (reuses "Process Name" / "Process ID" labels from FLTKUtils)
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
    // Right Column: Options
    // ------------------------------------------------------------------
    int col1_x = imageSize.first + UI_LEFT_MARGIN;
    int col1_y = lang_flex_height;
    int col1_w = g_main_window->w() - (imageSize.first + UI_LEFT_MARGIN);
    int col1_h = g_main_window->h() - lang_flex_height;
    Fl_Flex *options_flex = new Fl_Flex(col1_x, col1_y, col1_w, col1_h, Fl_Flex::VERTICAL);
    options_flex->margin(30, 20, 20, 20);
    options_flex->gap(8);

    // --- Game Path + Launch (stays at top) ---
    {
        Fl_Flex *path_flex = new Fl_Flex(0, 0, 0, 0, Fl_Flex::HORIZONTAL);
        path_flex->gap(5);

        Fl_Box *path_label = new Fl_Box(0, 0, 0, 0);
        path_flex->fixed(path_label, 80);
        tr(path_label, "Game Path:");

        g_game_path_input = new Fl_Input(0, 0, 0, 0);
        g_game_path_input->value(trainer.gamePath.c_str());
        g_game_path_input->labelfont(FL_FREE_FONT);
        g_game_path_input->labelsize(font_size);
        g_game_path_input->textfont(FL_FREE_FONT);
        g_game_path_input->textsize(font_size);
        g_game_path_input->readonly(1);

        LaunchData *launchData = new LaunchData{&trainer, g_game_path_input};

        Fl_Button *browse_btn = new Fl_Button(0, 0, 0, 0);
        path_flex->fixed(browse_btn, 30);
        browse_btn->label("...");
        browse_btn->labelfont(FL_FREE_FONT);
        browse_btn->labelsize(font_size);
        browse_btn->callback(browse_callback, launchData);

        Fl_Button *launch_btn = new Fl_Button(0, 0, 0, 0);
        path_flex->fixed(launch_btn, 100);
        tr(launch_btn, "Launch Game");
        launch_btn->callback(launch_callback, launchData);

        path_flex->end();
        options_flex->fixed(path_flex, UI_OPTION_HEIGHT);
    }

    // --- Separator ---
    {
        Fl_Box *sep = new Fl_Box(0, 0, 0, 0);
        options_flex->fixed(sep, 5);
    }

    // --- Top spacer (pushes cheats to vertical center) ---
    Fl_Box *spacerTop = new Fl_Box(0, 0, 0, 0);

    // --- In-Level Cheats ---
    place_apply_widget(options_flex, &trainer, "InstantWin", "Instant Win");

    place_apply_widget(options_flex, &trainer, "SetSun", "Set Sun", nullptr, "9900", "0", "99999");

    place_apply_widget(options_flex, &trainer, "SetPlantFood", "Set Plant Food", nullptr, "10", "0", "999");

    place_apply_widget(options_flex, &trainer, "KillAllZombies", "Kill All Zombies");

    place_apply_widget(options_flex, &trainer, "UnlockAllPlants", "Unlock All Plants");

    place_apply_widget(options_flex, &trainer, "SetPlantBoost", "Set All Plant Boosts", nullptr, "5", "0", "999999999");

    // --- Separator ---
    {
        Fl_Box *sep = new Fl_Box(0, 0, 0, 0);
        options_flex->fixed(sep, 5);
    }

    // --- Zen Garden Cheats ---
    place_apply_widget(options_flex, &trainer, "PlantAllGardenPlants", "Plant All Garden Slots");

    place_apply_widget(options_flex, &trainer, "MatureAllGardenPlants", "Mature All Garden Plants");

    place_apply_widget(options_flex, &trainer, "ShovelAllGardenPlants", "Shovel All Garden Plants");

    // --- Separator ---
    {
        Fl_Box *sep = new Fl_Box(0, 0, 0, 0);
        options_flex->fixed(sep, 5);
    }

    // --- Currency Cheats ---
    place_apply_widget(options_flex, &trainer, "AddCoins", "Add Coins", nullptr, "99999", "-999999999", "999999999");

    place_apply_widget(options_flex, &trainer, "AddGems", "Add Gems", nullptr, "999", "-999999999", "999999999");

    place_apply_widget(options_flex, &trainer, "AddSprouts", "Add Sprouts", nullptr, "999", "-999999999", "999999999");

    place_apply_widget(options_flex, &trainer, "AddWorldKeys", "Add World Keys", nullptr, "10", "-999999999", "999999999");

    // --- Bottom spacer ---
    Fl_Box *spacerBottom = new Fl_Box(0, 0, 0, 0);
    options_flex->end();

    // Apply translations
    change_language(language, g_main_window);
    update_window_title();

    // =========================
    // Finalize and Show Window
    // =========================
    g_main_window->end();
    g_main_window->show(argc, argv);
    return Fl::run();
}
