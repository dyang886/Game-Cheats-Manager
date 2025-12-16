// main.cpp
#include <FL/Fl.H>
#include <FL/Fl_Radio_Round_Button.H>
#include <FL/Fl_PNG_Image.H>
#include <FL/Fl_Flex.H>
#include <FL/Fl_Table.H>
#include <FL/forms.H>
#include "trainer.h"
#include "FLTKUtils.h"

// Callback function for apply button
void apply_callback(Fl_Widget *widget, void *data)
{
    ApplyData *applyData = static_cast<ApplyData *>(data);
    Trainer *trainer = applyData->trainer;
    std::string optionName = applyData->optionName;
    Fl_Button *button = applyData->button;
    Fl_Input *input = applyData->input;

    if (!trainer->isProcessRunning())
    {
        fl_alert(t("Please run the game first."));
        return;
    }

    std::string inputValue = "0";
    bool status = true;
    if (input && input->value())
        inputValue = input->value();

    if (optionName == "AddBeatPoints")
    {
        status = trainer->addBeatPoints(std::stoi(inputValue));
    }
    else if (optionName == "FinishLevel")
    {
        status = trainer->finishLevel();
    }

    // Finalize
    if (!status)
    {
        fl_alert(t("Failed to activate."));
    }
}

// Callback function for toggles
void toggle_callback(Fl_Widget *widget, void *data)
{
    ToggleData *toggleData = static_cast<ToggleData *>(data);
    Trainer *trainer = toggleData->trainer;
    std::string optionName = toggleData->optionName;
    Fl_Check_Button *button = toggleData->button;
    Fl_Input *input = toggleData->input;

    if (!trainer->isProcessRunning())
    {
        fl_alert(t("Please run the game first."));
        button->value(0);
        return;
    }

    std::string inputValue = "0";
    bool status = true;
    if (input && input->value())
        inputValue = input->value();

    if (optionName == "GodMode")
    {
        if (button->value())
            status = trainer->godMode(true);
        else
            status = trainer->godMode(false);
    }

    // Finalize
    if (!status)
    {
        fl_alert(t("Failed to activate/deactivate."));
        button->value(0);
    }

    if (input)
        button->value() ? input->readonly(1) : input->readonly(0);
}

static void main_window_close_callback(Fl_Widget *w, void *)
{
    RemoveFontMemResourceEx(font_handle);
    Fl::delete_widget(w);
}

// ===========================================================================
// Main Function
// ===========================================================================
int main(int argc, char **argv)
{
    Trainer trainer;
    setupLanguage();
    load_translations("TRANSLATION_JSON");

    // Create the main window
    int win_w = 800;
    int win_h = 600;
    int screen_w = Fl::w();
    int screen_h = Fl::h();
    int win_x = (screen_w - win_w) / 2;
    int win_y = (screen_h - win_h) / 2;

    Fl_Window *window = new Fl_Window(win_x, win_y, win_w, win_h);
    Fl::scheme("gtk+");
    Fl::set_color(FL_FREE_COLOR, 0x1c1c1c00);
    window->color(FL_FREE_COLOR);
    window->icon((char *)LoadIconA(GetModuleHandle(NULL), "APP_ICON"));
    window->callback(main_window_close_callback);
    tr(window, "Just Shapes & Beats Trainer");

    // Setup fonts
    DWORD font_mem_size = 0;
    DWORD num_fonts = 0;
    const unsigned char *font_data = load_resource("FONT_TTF", font_mem_size);
    font_handle = AddFontMemResourceEx((void *)font_data, font_mem_size, nullptr, &num_fonts);
    Fl::set_font(FL_FREE_FONT, "Noto Sans SC");
    fl_font(FL_FREE_FONT, font_size);

    DWORD info_img_size = 0;
    const unsigned char *info_img_data = load_resource("INFO_IMG", info_img_size);
    Fl_PNG_Image *info_img = new Fl_PNG_Image(nullptr, info_img_data, (int)info_img_size);
    info_img->scale(20, 20, 1, 0);

    // ------------------------------------------------------------------
    // Top Row: Language Selection
    // ------------------------------------------------------------------
    int lang_flex_height = 30;
    int lang_flex_width = 200;

    Fl_Flex lang_flex(window->w() - lang_flex_width, 0, lang_flex_width, lang_flex_height, Fl_Flex::HORIZONTAL);
    lang_flex.color(FL_BLACK);
    Fl_Radio_Round_Button *lang_en = new Fl_Radio_Round_Button(0, 0, 0, 0, "English");
    if (language == "en_US")
        lang_en->set();
    lang_en->labelfont(FL_FREE_FONT);
    lang_en->labelsize(font_size);
    lang_en->labelcolor(FL_WHITE);
    ChangeLanguageData *changeLanguageDataEN = new ChangeLanguageData{"en_US", window};
    lang_en->callback(change_language_callback, changeLanguageDataEN);

    Fl_Radio_Round_Button *lang_zh = new Fl_Radio_Round_Button(0, 0, 0, 0, "简体中文");
    if (language == "zh_CN")
        lang_zh->set();
    lang_zh->labelfont(FL_FREE_FONT);
    lang_zh->labelsize(font_size);
    lang_zh->labelcolor(FL_WHITE);
    ChangeLanguageData *changeLanguageDataSC = new ChangeLanguageData{"zh_CN", window};
    lang_zh->callback(change_language_callback, changeLanguageDataSC);

    lang_flex.end();

    // ------------------------------------------------------------------
    // Left Column: Image and Process Info
    // ------------------------------------------------------------------
    std::pair<int, int> imageSize = std::make_pair(200, 300);

    DWORD img_size = 0;
    const unsigned char *data = load_resource("LOGO_IMG", img_size);
    Fl_PNG_Image *game_img = new Fl_PNG_Image(nullptr, data, (int)img_size);
    game_img->scale(imageSize.first, imageSize.second, 1, 0);
    Fl_Box *img_box = new Fl_Box(20, lang_flex_height, imageSize.first, imageSize.second);
    img_box->image(game_img);

    // Process Information
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
    // Option column 1
    // ------------------------------------------------------------------
    int col1_x = imageSize.first + UI_LEFT_MARGIN;
    int col1_y = lang_flex_height;
    int col1_w = window->w() - (imageSize.first + UI_LEFT_MARGIN);
    int col1_h = window->h() - 30;
    Fl_Flex *options1_flex = new Fl_Flex(col1_x, col1_y, col1_w, col1_h, Fl_Flex::VERTICAL);
    options1_flex->margin(50, 20, 20, 20);
    options1_flex->gap(10);
    Fl_Box *spacerTop = new Fl_Box(0, 0, 0, 0);

    // Widget Placements
    place_toggle_widget(options1_flex, &trainer, "GodMode", "God Mode");

    place_apply_widget(options1_flex, &trainer, "AddBeatPoints", "Add Beat Points", nullptr, "5000", "1", "999999999");

    place_apply_widget(options1_flex, &trainer, "FinishLevel", "Finish Level");

    // End of Option Column 1
    Fl_Box *spacerBottom = new Fl_Box(0, 0, 0, 0);
    options1_flex->end();
    change_language(language, window);

    // =========================
    // Finalize and Show Window
    // =========================
    window->end();
    window->show(argc, argv);
    int ret = Fl::run();

    return ret;
}
