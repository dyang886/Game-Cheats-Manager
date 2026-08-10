// Big Walk trainer UI.
// Uses IL2CPP DLL injection through Il2CppBase and the embedded IL2CPP payload.

#include <FL/Fl.H>
#include <FL/Fl_Flex.H>
#include <FL/Fl_PNG_Image.H>
#include <FL/Fl_Radio_Round_Button.H>
#include <FL/forms.H>
#include <sstream>
#include "trainer.h"
#include "FLTKUtils.h"

// Version info
static constexpr const char *TRAINER_NAME = "Big Walk Trainer";
static constexpr const char *GAME_VERSION = "1.4.8";
static constexpr const char *TRAINER_VERSION = "1.0";

static Fl_Window *g_main_window = nullptr;

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
        const float value = std::stof(val);

        if (optionName == "ToggleAlwaysDay")
            status = trainer->toggleAlwaysDay(isEnabled);
        else if (optionName == "ToggleNoTripping")
            status = trainer->toggleNoTripping(isEnabled);
        else if (optionName == "SetMovementSpeedMultiplier")
            status = trainer->setMovementSpeedMultiplier(isEnabled, value);
        else if (optionName == "SetJumpHeightMultiplier")
            status = trainer->setJumpHeightMultiplier(isEnabled, value);
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

/// Required by FLTKUtils' place_apply_widget; this trainer only has toggles so far.
void apply_callback(Fl_Widget *, void *)
{
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

    int win_w = 800;
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
    Fl::add_timeout(0, check_process_status, timeoutData);

    // ------------------------------------------------------------------
    // Right Column: Options
    // ------------------------------------------------------------------
    int options_x = imageSize.first + UI_LEFT_MARGIN;
    int options_y = lang_flex_height;
    int options_w = g_main_window->w() - options_x;
    int options_h = g_main_window->h() - lang_flex_height;
    Fl_Flex *options_flex = new Fl_Flex(options_x, options_y, options_w, options_h, Fl_Flex::VERTICAL);
    options_flex->margin(80, 20, 20, 20);
    options_flex->gap(8);

    Fl_Box *spacerTop = new Fl_Box(0, 0, 0, 0);

    place_toggle_widget(options_flex, &trainer, "ToggleAlwaysDay", "Always Day");

    place_toggle_widget(options_flex, &trainer, "ToggleNoTripping", "No Tripping");

    place_toggle_widget(options_flex, &trainer, "SetMovementSpeedMultiplier", "Movement Speed Multiplier", nullptr, "1", "0.01", "100", FL_FLOAT_INPUT);

    place_toggle_widget(options_flex, &trainer, "SetJumpHeightMultiplier", "Jump Height Multiplier", nullptr, "1", "0.01", "100", FL_FLOAT_INPUT);

    Fl_Box *spacerBottom = new Fl_Box(0, 0, 0, 0);
    options_flex->end();

    change_language(language, g_main_window);
    update_window_title();

    g_main_window->end();
    g_main_window->show(argc, argv);
    return Fl::run();
}
