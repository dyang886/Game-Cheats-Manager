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
    if (optionName == "Coin")
    {
        if (button->value())
        {
            status = trainer->setCoin(std::stoi(inputValue));
        }
        else
        {
            status = trainer->disableNamedPointerToggle(optionName);
        }
    }
    else if (optionName == "Health")
    {
        if (button->value())
        {
            status = trainer->setHealth(std::stof(inputValue));
        }
        else
        {
            status = trainer->disableNamedPointerToggle(optionName);
        }
    }
    else if (optionName == "HorizontalSpeed")
    {
        if (button->value())
        {
            status = trainer->setHorizontalSpeed(std::stoi(inputValue));
        }
        else
        {
            status = trainer->disableNamedHook(optionName);
        }
    }
    else if (optionName == "ArrowDamage")
    {
        if (button->value())
        {
            status = trainer->setArrowDamage(std::stof(inputValue));
        }
        else
        {
            status = trainer->disableNamedHook(optionName);
        }
    }
    else if (optionName == "ArrowFrequency")
    {
        if (button->value())
        {
            status = trainer->setArrowFrequency(std::stof(inputValue) / 10.0);
        }
        else
        {
            status = trainer->disableNamedHook(optionName);
        }
    }
    else if (optionName == "ArrowSpeed")
    {
        if (button->value())
        {
            status = trainer->setArrowSpeed(std::stof(inputValue));
        }
        else
        {
            status = trainer->disableNamedHook(optionName);
        }
    }
    else if (optionName == "ArrowDistance")
    {
        if (button->value())
        {
            status = trainer->setArrowDistance(std::stof(inputValue));
        }
        else
        {
            status = trainer->disableNamedHook(optionName);
        }
    }
    else if (optionName == "ArrowCount")
    {
        if (button->value())
        {
            status = trainer->setArrowCount(std::stoi(inputValue));
        }
        else
        {
            status = trainer->disableNamedHook(optionName);
        }
    }
    else if (optionName == "SwordDamage")
    {
        if (button->value())
        {
            status = trainer->setSwordDamage(std::stof(inputValue));
        }
        else
        {
            status = trainer->disableNamedHook(optionName);
        }
    }
    else if (optionName == "SwordCoolDown")
    {
        if (button->value())
        {
            status = trainer->setSwordCoolDown(std::stof(inputValue));
        }
        else
        {
            status = trainer->disableNamedHook(optionName);
        }
    }
    else if (optionName == "SwordSpeed")
    {
        if (button->value())
        {
            status = trainer->setSwordSpeed(std::stof(inputValue));
        }
        else
        {
            status = trainer->disableNamedHook(optionName);
        }
    }
    else if (optionName == "SwordDistance")
    {
        if (button->value())
        {
            status = trainer->setSwordDistance(std::stof(inputValue) * 0.7000122 - 0.0000085);
        }
        else
        {
            status = trainer->disableNamedHook(optionName);
        }
    }

    // Finalize
    if (!status)
    {
        fl_alert(t("Failed to activate/deactivate."));
        button->value(0);
    }
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
    tr(window, "Arrow a Row Trainer");

    // Setup fonts
    DWORD font_mem_size = 0;
    DWORD num_fonts = 0;
    const unsigned char *font_data = load_resource("FONT_TTF", font_mem_size);
    font_handle = AddFontMemResourceEx((void *)font_data, font_mem_size, nullptr, &num_fonts);
    Fl::set_font(FL_FREE_FONT, "Noto Sans SC");
    fl_font(FL_FREE_FONT, font_size);

    int left_margin = 20;
    int button_w = 50;
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
    // Option 1: Set coin (Toggle)
    // ------------------------------------------------------------------
    Fl_Flex *coin_flex = new Fl_Flex(0, 0, 0, 0, Fl_Flex::HORIZONTAL);
    coin_flex->gap(option_gap);

    Fl_Check_Button *coin_check_button = new Fl_Check_Button(0, 0, 0, 0);
    coin_flex->fixed(coin_check_button, button_w);

    Fl_Box *coin_label = new Fl_Box(0, 0, 0, 0);
    tr(coin_label, "Set Coins");

    Fl_Input *coin_input = new Fl_Input(0, 0, 0, 0);
    coin_flex->fixed(coin_input, input_w);
    coin_input->type(FL_INT_INPUT);
    set_input_values(coin_input, "9999", "0", "99999999");

    ToggleData *td_coin = new ToggleData{&trainer, "Coin", coin_check_button, coin_input};
    coin_check_button->callback(toggle_callback, td_coin);

    coin_flex->end();
    options1_flex->fixed(coin_flex, option_h);

    // ------------------------------------------------------------------
    // Option 2: Set health (Toggle)
    // ------------------------------------------------------------------
    Fl_Flex *health_flex = new Fl_Flex(0, 0, 0, 0, Fl_Flex::HORIZONTAL);
    health_flex->gap(option_gap);

    Fl_Check_Button *health_check_button = new Fl_Check_Button(0, 0, 0, 0);
    health_flex->fixed(health_check_button, button_w);

    Fl_Box *health_label = new Fl_Box(0, 0, 0, 0);
    tr(health_label, "Set Health");

    Fl_Input *health_input = new Fl_Input(0, 0, 0, 0);
    health_flex->fixed(health_input, input_w);
    health_input->type(FL_INT_INPUT);
    set_input_values(health_input, "9999999999", "0", "99999999999999999999999999999999999999");

    ToggleData *td_health = new ToggleData{&trainer, "Health", health_check_button, health_input};
    health_check_button->callback(toggle_callback, td_health);

    health_flex->end();
    options1_flex->fixed(health_flex, option_h);

    // ------------------------------------------------------------------
    // Option 3: Set horizontal_speed (Toggle)
    // ------------------------------------------------------------------
    Fl_Flex *horizontal_speed_flex = new Fl_Flex(0, 0, 0, 0, Fl_Flex::HORIZONTAL);
    horizontal_speed_flex->gap(option_gap);

    Fl_Check_Button *horizontal_speed_check_button = new Fl_Check_Button(0, 0, 0, 0);
    horizontal_speed_flex->fixed(horizontal_speed_check_button, button_w);

    Fl_Box *horizontal_speed_label = new Fl_Box(0, 0, 0, 0);
    tr(horizontal_speed_label, "Set Horizontal Speed");

    Fl_Input *horizontal_speed_input = new Fl_Input(0, 0, 0, 0);
    horizontal_speed_flex->fixed(horizontal_speed_input, input_w);
    horizontal_speed_input->type(FL_INT_INPUT);
    set_input_values(horizontal_speed_input, "30", "0", "99");

    ToggleData *td_horizontal_speed = new ToggleData{&trainer, "HorizontalSpeed", horizontal_speed_check_button, horizontal_speed_input};
    horizontal_speed_check_button->callback(toggle_callback, td_horizontal_speed);

    horizontal_speed_flex->end();
    options1_flex->fixed(horizontal_speed_flex, option_h);

    // ------------------------------------------------------------------
    // Option 4: Set arrow_damage (Toggle)
    // ------------------------------------------------------------------
    Fl_Flex *arrow_damage_flex = new Fl_Flex(0, 0, 0, 0, Fl_Flex::HORIZONTAL);
    arrow_damage_flex->gap(option_gap);

    Fl_Check_Button *arrow_damage_check_button = new Fl_Check_Button(0, 0, 0, 0);
    arrow_damage_flex->fixed(arrow_damage_check_button, button_w);

    Fl_Box *arrow_damage_label = new Fl_Box(0, 0, 0, 0);
    tr(arrow_damage_label, "Set Arrow Damage");

    Fl_Input *arrow_damage_input = new Fl_Input(0, 0, 0, 0);
    arrow_damage_flex->fixed(arrow_damage_input, input_w);
    arrow_damage_input->type(FL_INT_INPUT);
    set_input_values(arrow_damage_input, "99", "1", "9999");

    ToggleData *td_arrow_damage = new ToggleData{&trainer, "ArrowDamage", arrow_damage_check_button, arrow_damage_input};
    arrow_damage_check_button->callback(toggle_callback, td_arrow_damage);

    arrow_damage_flex->end();
    options1_flex->fixed(arrow_damage_flex, option_h);

    // ------------------------------------------------------------------
    // Option 5: Set arrow_frequency (Toggle)
    // ------------------------------------------------------------------
    Fl_Flex *arrow_frequency_flex = new Fl_Flex(0, 0, 0, 0, Fl_Flex::HORIZONTAL);
    arrow_frequency_flex->gap(option_gap);

    Fl_Check_Button *arrow_frequency_check_button = new Fl_Check_Button(0, 0, 0, 0);
    arrow_frequency_flex->fixed(arrow_frequency_check_button, button_w);

    Fl_Box *arrow_frequency_label = new Fl_Box(0, 0, 0, 0);
    tr(arrow_frequency_label, "Set Arrow Frequency");

    Fl_Input *arrow_frequency_input = new Fl_Input(0, 0, 0, 0);
    arrow_frequency_flex->fixed(arrow_frequency_input, input_w);
    arrow_frequency_input->type(FL_INT_INPUT);
    set_input_values(arrow_frequency_input, "99", "1", "999");

    ToggleData *td_arrow_frequency = new ToggleData{&trainer, "ArrowFrequency", arrow_frequency_check_button, arrow_frequency_input};
    arrow_frequency_check_button->callback(toggle_callback, td_arrow_frequency);

    arrow_frequency_flex->end();
    options1_flex->fixed(arrow_frequency_flex, option_h);

    // ------------------------------------------------------------------
    // Option 6: Set arrow_speed (Toggle)
    // ------------------------------------------------------------------
    Fl_Flex *arrow_speed_flex = new Fl_Flex(0, 0, 0, 0, Fl_Flex::HORIZONTAL);
    arrow_speed_flex->gap(option_gap);

    Fl_Check_Button *arrow_speed_check_button = new Fl_Check_Button(0, 0, 0, 0);
    arrow_speed_flex->fixed(arrow_speed_check_button, button_w);

    Fl_Box *arrow_speed_label = new Fl_Box(0, 0, 0, 0);
    tr(arrow_speed_label, "Set Arrow Speed");

    Fl_Input *arrow_speed_input = new Fl_Input(0, 0, 0, 0);
    arrow_speed_flex->fixed(arrow_speed_input, input_w);
    arrow_speed_input->type(FL_INT_INPUT);
    set_input_values(arrow_speed_input, "99", "1", "999");

    ToggleData *td_arrow_speed = new ToggleData{&trainer, "ArrowSpeed", arrow_speed_check_button, arrow_speed_input};
    arrow_speed_check_button->callback(toggle_callback, td_arrow_speed);

    arrow_speed_flex->end();
    options1_flex->fixed(arrow_speed_flex, option_h);

    // ------------------------------------------------------------------
    // Option 7: Set arrow_distance (Toggle)
    // ------------------------------------------------------------------
    Fl_Flex *arrow_distance_flex = new Fl_Flex(0, 0, 0, 0, Fl_Flex::HORIZONTAL);
    arrow_distance_flex->gap(option_gap);

    Fl_Check_Button *arrow_distance_check_button = new Fl_Check_Button(0, 0, 0, 0);
    arrow_distance_flex->fixed(arrow_distance_check_button, button_w);

    Fl_Box *arrow_distance_label = new Fl_Box(0, 0, 0, 0);
    tr(arrow_distance_label, "Set Arrow Distance");

    Fl_Input *arrow_distance_input = new Fl_Input(0, 0, 0, 0);
    arrow_distance_flex->fixed(arrow_distance_input, input_w);
    arrow_distance_input->type(FL_INT_INPUT);
    set_input_values(arrow_distance_input, "99", "1", "999");

    ToggleData *td_arrow_distance = new ToggleData{&trainer, "ArrowDistance", arrow_distance_check_button, arrow_distance_input};
    arrow_distance_check_button->callback(toggle_callback, td_arrow_distance);

    arrow_distance_flex->end();
    options1_flex->fixed(arrow_distance_flex, option_h);

    // ------------------------------------------------------------------
    // Option 8: Set arrow_count (Toggle)
    // ------------------------------------------------------------------
    Fl_Flex *arrow_count_flex = new Fl_Flex(0, 0, 0, 0, Fl_Flex::HORIZONTAL);
    arrow_count_flex->gap(option_gap);

    Fl_Check_Button *arrow_count_check_button = new Fl_Check_Button(0, 0, 0, 0);
    arrow_count_flex->fixed(arrow_count_check_button, button_w);

    Fl_Box *arrow_count_label = new Fl_Box(0, 0, 0, 0);
    tr(arrow_count_label, "Set Arrow Count");

    Fl_Input *arrow_count_input = new Fl_Input(0, 0, 0, 0);
    arrow_count_flex->fixed(arrow_count_input, input_w);
    arrow_count_input->type(FL_INT_INPUT);
    set_input_values(arrow_count_input, "99", "1", "999");

    ToggleData *td_arrow_count = new ToggleData{&trainer, "ArrowCount", arrow_count_check_button, arrow_count_input};
    arrow_count_check_button->callback(toggle_callback, td_arrow_count);

    arrow_count_flex->end();
    options1_flex->fixed(arrow_count_flex, option_h);

    // ------------------------------------------------------------------
    // Option 9: Set sword_damage (Toggle)
    // ------------------------------------------------------------------
    Fl_Flex *sword_damage_flex = new Fl_Flex(0, 0, 0, 0, Fl_Flex::HORIZONTAL);
    sword_damage_flex->gap(option_gap);

    Fl_Check_Button *sword_damage_check_button = new Fl_Check_Button(0, 0, 0, 0);
    sword_damage_flex->fixed(sword_damage_check_button, button_w);

    Fl_Box *sword_damage_label = new Fl_Box(0, 0, 0, 0);
    tr(sword_damage_label, "Set Sword Damage");

    Fl_Input *sword_damage_input = new Fl_Input(0, 0, 0, 0);
    sword_damage_flex->fixed(sword_damage_input, input_w);
    sword_damage_input->type(FL_INT_INPUT);
    set_input_values(sword_damage_input, "99", "1", "999");

    ToggleData *td_sword_damage = new ToggleData{&trainer, "SwordDamage", sword_damage_check_button, sword_damage_input};
    sword_damage_check_button->callback(toggle_callback, td_sword_damage);

    sword_damage_flex->end();
    options1_flex->fixed(sword_damage_flex, option_h);

    // ------------------------------------------------------------------
    // Option 10: Set sword_cool_down (Toggle)
    // ------------------------------------------------------------------
    Fl_Flex *sword_cool_down_flex = new Fl_Flex(0, 0, 0, 0, Fl_Flex::HORIZONTAL);
    sword_cool_down_flex->gap(option_gap);

    Fl_Check_Button *sword_cool_down_check_button = new Fl_Check_Button(0, 0, 0, 0);
    sword_cool_down_flex->fixed(sword_cool_down_check_button, button_w);

    Fl_Box *sword_cool_down_label = new Fl_Box(0, 0, 0, 0);
    tr(sword_cool_down_label, "Set Sword Cooldown");

    Fl_Input *sword_cool_down_input = new Fl_Input(0, 0, 0, 0);
    sword_cool_down_flex->fixed(sword_cool_down_input, input_w);
    sword_cool_down_input->type(FL_INT_INPUT);
    set_input_values(sword_cool_down_input, "0", "0", "999");

    ToggleData *td_sword_cool_down = new ToggleData{&trainer, "SwordCoolDown", sword_cool_down_check_button, sword_cool_down_input};
    sword_cool_down_check_button->callback(toggle_callback, td_sword_cool_down);

    sword_cool_down_flex->end();
    options1_flex->fixed(sword_cool_down_flex, option_h);

    // ------------------------------------------------------------------
    // Option 11: Set sword_speed (Toggle)
    // ------------------------------------------------------------------
    Fl_Flex *sword_speed_flex = new Fl_Flex(0, 0, 0, 0, Fl_Flex::HORIZONTAL);
    sword_speed_flex->gap(option_gap);

    Fl_Check_Button *sword_speed_check_button = new Fl_Check_Button(0, 0, 0, 0);
    sword_speed_flex->fixed(sword_speed_check_button, button_w);

    Fl_Box *sword_speed_label = new Fl_Box(0, 0, 0, 0);
    tr(sword_speed_label, "Set Sword Speed");

    Fl_Input *sword_speed_input = new Fl_Input(0, 0, 0, 0);
    sword_speed_flex->fixed(sword_speed_input, input_w);
    sword_speed_input->type(FL_INT_INPUT);
    set_input_values(sword_speed_input, "99", "1", "999");

    ToggleData *td_sword_speed = new ToggleData{&trainer, "SwordSpeed", sword_speed_check_button, sword_speed_input};
    sword_speed_check_button->callback(toggle_callback, td_sword_speed);

    sword_speed_flex->end();
    options1_flex->fixed(sword_speed_flex, option_h);

    // ------------------------------------------------------------------
    // Option 12: Set sword_distance (Toggle)
    // ------------------------------------------------------------------
    Fl_Flex *sword_distance_flex = new Fl_Flex(0, 0, 0, 0, Fl_Flex::HORIZONTAL);
    sword_distance_flex->gap(option_gap);

    Fl_Check_Button *sword_distance_check_button = new Fl_Check_Button(0, 0, 0, 0);
    sword_distance_flex->fixed(sword_distance_check_button, button_w);

    Fl_Box *sword_distance_label = new Fl_Box(0, 0, 0, 0);
    tr(sword_distance_label, "Set Sword Distance");

    Fl_Input *sword_distance_input = new Fl_Input(0, 0, 0, 0);
    sword_distance_flex->fixed(sword_distance_input, input_w);
    sword_distance_input->type(FL_INT_INPUT);
    set_input_values(sword_distance_input, "99", "1", "999");

    ToggleData *td_sword_distance = new ToggleData{&trainer, "SwordDistance", sword_distance_check_button, sword_distance_input};
    sword_distance_check_button->callback(toggle_callback, td_sword_distance);

    sword_distance_flex->end();
    options1_flex->fixed(sword_distance_flex, option_h);

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
