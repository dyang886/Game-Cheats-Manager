// main.cpp
#include <FL/Fl.H>
#include <FL/Fl_Radio_Round_Button.H>
#include <FL/Fl_PNG_Image.H>
#include <FL/Fl_Flex.H>
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

    // Check if the game process is running
    if (!applyData->trainer->isProcessRunning())
    {
        fl_alert(t("Please run the game first."));
        return;
    }

    // Retrieve and validate the input value
    std::string inputValue = "0";
    bool status = true;
    if (input && input->value())
    {
        inputValue = input->value();
    }

    // Apply the value using the Trainer class

    // Finalize
    if (!status)
    {
        fl_alert(t("Failed to activate."));
    }
    if (input)
        button->value() ? input->readonly(1) : input->readonly(0);
}

// Callback function for toggles
void toggle_callback(Fl_Widget *widget, void *data)
{
    ToggleData *toggleData = static_cast<ToggleData *>(data);
    Trainer *trainer = toggleData->trainer;
    std::string optionName = toggleData->optionName;
    Fl_Check_Button *button = toggleData->button;
    Fl_Input *input = toggleData->input;

    if (!toggleData->trainer->isProcessRunning())
    {
        fl_alert(t("Please run the game first."));
        button->value(0);
        return;
    }

    // Retrieve and validate the input value
    std::string inputValue = "0";
    bool status = true;
    if (input && input->value())
    {
        inputValue = input->value();
    }

    // Apply the value using the Trainer class
    if (optionName == "SetHealth")
    {
        if (button->value())
        {
            status = trainer->setHealth(std::stoi(inputValue));
        }
        else
        {
            status = trainer->disableNamedPointerToggle(optionName);
        }
    }
    else if (optionName == "SetMaxHealth")
    {
        if (button->value())
        {
            status = trainer->setMaxHealth(std::stof(inputValue));
        }
        else
        {
            status = trainer->disableNamedPointerToggle(optionName);
        }
    }
    else if (optionName == "SetMovementSpeed")
    {
        if (button->value())
        {
            status = trainer->setSpeed(std::stof(inputValue));
        }
        else
        {
            status = trainer->disableNamedPointerToggle(optionName);
        }
    }
    else if (optionName == "SetCoins")
    {
        if (button->value())
        {
            status = trainer->setCoins(std::stof(inputValue));
        }
        else
        {
            status = trainer->disableNamedPointerToggle(optionName);
        }
    }
    else if (optionName == "SetGems")
    {
        if (button->value())
        {
            status = trainer->setGems(std::stoi(inputValue));
        }
        else
        {
            status = trainer->disableNamedPointerToggle(optionName);
        }
    }
    else if (optionName == "SetArcaneChromo")
    {
        if (button->value())
        {
            status = trainer->setArcaneChromo(std::stoi(inputValue));
        }
        else
        {
            status = trainer->disableNamedPointerToggle(optionName);
        }
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
    tr(window, "Wizard of Legend 2 Trainer");

    // Setup fonts
    DWORD font_mem_size = 0;
    DWORD num_fonts = 0;
    const unsigned char *font_data = load_resource("FONT_TTF", font_mem_size);
    font_handle = AddFontMemResourceEx((void *)font_data, font_mem_size, nullptr, &num_fonts);
    Fl::set_font(FL_FREE_FONT, "Noto Sans SC");
    fl_font(FL_FREE_FONT, font_size);

    int left_margin = 20;
    int button_w = 50;
    int toggle_w = 16;
    int toggle_spacer_w = 24;
    int input_w = 200;
    int option_gap = 10;
    int option_h = static_cast<int>(font_size * 1.5);

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
    Fl_Box *process_name = new Fl_Box(left_margin, lang_flex_height + imageSize.second + 10, imageSize.first, font_size);
    process_name->align(FL_ALIGN_TOP_LEFT | FL_ALIGN_INSIDE);
    tr(process_name, "Process Name:");

    Fl_Box *process_exe = new Fl_Box(left_margin, lang_flex_height + imageSize.second + font_size + 20, imageSize.first, font_size);
    process_exe->align(FL_ALIGN_TOP_LEFT | FL_ALIGN_INSIDE);

    Fl_Flex *process_id_flex = new Fl_Flex(left_margin, lang_flex_height + imageSize.second + font_size + 55, imageSize.first, font_size, Fl_Flex::HORIZONTAL);
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
    int col1_x = imageSize.first + left_margin;
    int col1_y = lang_flex_height;
    int col1_w = window->w() - (imageSize.first + left_margin);
    int col1_h = window->h() - 30;
    Fl_Flex *options1_flex = new Fl_Flex(col1_x, col1_y, col1_w, col1_h, Fl_Flex::VERTICAL);
    options1_flex->margin(50, 20, 20, 20);
    options1_flex->gap(10);
    Fl_Box *spacerTop = new Fl_Box(0, 0, 0, 0);

    // ------------------------------------------------------------------
    // Set Health (Toggle)
    // ------------------------------------------------------------------
    Fl_Flex *health_flex = new Fl_Flex(0, 0, 0, 0, Fl_Flex::HORIZONTAL);
    health_flex->gap(option_gap);

    Fl_Box *health_toggle_spacer = new Fl_Box(0, 0, 0, 0);
    health_flex->fixed(health_toggle_spacer, toggle_spacer_w);

    Fl_Check_Button *health_check_button = new Fl_Check_Button(0, 0, 0, 0);
    health_flex->fixed(health_check_button, toggle_w);

    Fl_Box *health_label = new Fl_Box(0, 0, 0, 0);
    tr(health_label, "Set Health");

    Fl_Input *health_input = new Fl_Input(0, 0, 0, 0);
    health_flex->fixed(health_input, input_w);
    health_input->type(FL_INT_INPUT);
    set_input_values(health_input, "9999", "0", "999999999");

    ToggleData *data_health = new ToggleData{&trainer, "SetHealth", health_check_button, health_input};
    health_check_button->callback(toggle_callback, data_health);

    health_flex->end();
    options1_flex->fixed(health_flex, option_h);

    // ------------------------------------------------------------------
    // Set Max Health (Toggle)
    // ------------------------------------------------------------------
    Fl_Flex *max_health_flex = new Fl_Flex(0, 0, 0, 0, Fl_Flex::HORIZONTAL);
    max_health_flex->gap(option_gap);

    Fl_Box *max_health_toggle_spacer = new Fl_Box(0, 0, 0, 0);
    max_health_flex->fixed(max_health_toggle_spacer, toggle_spacer_w);

    Fl_Check_Button *max_health_check_button = new Fl_Check_Button(0, 0, 0, 0);
    max_health_flex->fixed(max_health_check_button, toggle_w);

    Fl_Box *max_health_label = new Fl_Box(0, 0, 0, 0);
    tr(max_health_label, "Set Max Health");

    Fl_Input *max_health_input = new Fl_Input(0, 0, 0, 0);
    max_health_flex->fixed(max_health_input, input_w);
    max_health_input->type(FL_INT_INPUT);
    set_input_values(max_health_input, "9999", "0", "999999999");

    ToggleData *data_max_health = new ToggleData{&trainer, "SetMaxHealth", max_health_check_button, max_health_input};
    max_health_check_button->callback(toggle_callback, data_max_health);

    max_health_flex->end();
    options1_flex->fixed(max_health_flex, option_h);

    // ------------------------------------------------------------------
    // Set Movement Speed (Toggle)
    // ------------------------------------------------------------------
    Fl_Flex *speed_flex = new Fl_Flex(0, 0, 0, 0, Fl_Flex::HORIZONTAL);
    speed_flex->gap(option_gap);

    Fl_Box *speed_toggle_spacer = new Fl_Box(0, 0, 0, 0);
    speed_flex->fixed(speed_toggle_spacer, toggle_spacer_w);

    Fl_Check_Button *speed_check_button = new Fl_Check_Button(0, 0, 0, 0);
    speed_flex->fixed(speed_check_button, toggle_w);

    Fl_Box *speed_label = new Fl_Box(0, 0, 0, 0);
    tr(speed_label, "Set Movement Speed");

    Fl_Input *speed_input = new Fl_Input(0, 0, 0, 0);
    speed_flex->fixed(speed_input, input_w);
    speed_input->type(FL_INT_INPUT);
    set_input_values(speed_input, "5000", "1100", "999999999");

    ToggleData *data_speed = new ToggleData{&trainer, "SetMovementSpeed", speed_check_button, speed_input};
    speed_check_button->callback(toggle_callback, data_speed);

    speed_flex->end();
    options1_flex->fixed(speed_flex, option_h);

    // ------------------------------------------------------------------
    // Set Coins (Toggle)
    // ------------------------------------------------------------------
    Fl_Flex *coins_flex = new Fl_Flex(0, 0, 0, 0, Fl_Flex::HORIZONTAL);
    coins_flex->gap(option_gap);

    Fl_Box *coins_toggle_spacer = new Fl_Box(0, 0, 0, 0);
    coins_flex->fixed(coins_toggle_spacer, toggle_spacer_w);

    Fl_Check_Button *coins_check_button = new Fl_Check_Button(0, 0, 0, 0);
    coins_flex->fixed(coins_check_button, toggle_w);

    Fl_Box *coins_label = new Fl_Box(0, 0, 0, 0);
    tr(coins_label, "Set Coins");

    Fl_Input *coins_input = new Fl_Input(0, 0, 0, 0);
    coins_flex->fixed(coins_input, input_w);
    coins_input->type(FL_INT_INPUT);
    set_input_values(coins_input, "9999", "0", "999999999");

    ToggleData *data_coins = new ToggleData{&trainer, "SetCoins", coins_check_button, coins_input};
    coins_check_button->callback(toggle_callback, data_coins);

    coins_flex->end();
    options1_flex->fixed(coins_flex, option_h);

    // ------------------------------------------------------------------
    // Set Gems (Toggle)
    // ------------------------------------------------------------------
    Fl_Flex *gems_flex = new Fl_Flex(0, 0, 0, 0, Fl_Flex::HORIZONTAL);
    gems_flex->gap(option_gap);

    Fl_Box *gems_toggle_spacer = new Fl_Box(0, 0, 0, 0);
    gems_flex->fixed(gems_toggle_spacer, toggle_spacer_w);

    Fl_Check_Button *gems_check_button = new Fl_Check_Button(0, 0, 0, 0);
    gems_flex->fixed(gems_check_button, toggle_w);

    Fl_Box *gems_label = new Fl_Box(0, 0, 0, 0);
    tr(gems_label, "Set Gems");

    Fl_Input *gems_input = new Fl_Input(0, 0, 0, 0);
    gems_flex->fixed(gems_input, input_w);
    gems_input->type(FL_INT_INPUT);
    set_input_values(gems_input, "999999", "0", "999999999");

    ToggleData *data_gems = new ToggleData{&trainer, "SetGems", gems_check_button, gems_input};
    gems_check_button->callback(toggle_callback, data_gems);

    gems_flex->end();
    options1_flex->fixed(gems_flex, option_h);

    // ------------------------------------------------------------------
    // Set Arcane Chromo (Toggle)
    // ------------------------------------------------------------------
    Fl_Flex *arcane_chromo_flex = new Fl_Flex(0, 0, 0, 0, Fl_Flex::HORIZONTAL);
    arcane_chromo_flex->gap(option_gap);

    Fl_Box *arcane_chromo_toggle_spacer = new Fl_Box(0, 0, 0, 0);
    arcane_chromo_flex->fixed(arcane_chromo_toggle_spacer, toggle_spacer_w);

    Fl_Check_Button *arcane_chromo_check_button = new Fl_Check_Button(0, 0, 0, 0);
    arcane_chromo_flex->fixed(arcane_chromo_check_button, toggle_w);

    Fl_Box *arcane_chromo_label = new Fl_Box(0, 0, 0, 0);
    tr(arcane_chromo_label, "Set Arcane Chromo");

    Fl_Input *arcane_chromo_input = new Fl_Input(0, 0, 0, 0);
    arcane_chromo_flex->fixed(arcane_chromo_input, input_w);
    arcane_chromo_input->type(FL_INT_INPUT);
    set_input_values(arcane_chromo_input, "999", "0", "999999999");

    ToggleData *data_arcane_chromo = new ToggleData{&trainer, "SetArcaneChromo", arcane_chromo_check_button, arcane_chromo_input};
    arcane_chromo_check_button->callback(toggle_callback, data_arcane_chromo);

    arcane_chromo_flex->end();
    options1_flex->fixed(arcane_chromo_flex, option_h);

    // ------------------------------------------------------------------
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
