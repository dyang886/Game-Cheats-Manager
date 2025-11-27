// main.cpp
#include <FL/Fl.H>
#include <FL/Fl_Radio_Round_Button.H>
#include <FL/Fl_PNG_Image.H>
#include <FL/Fl_Flex.H>
#include <FL/Fl_Table.H>
#include <FL/forms.H>
#include "trainer.h"
#include "FLTKUtils.h"

static Fl_Window *plant_list_window = nullptr;
static Fl_Window *zombie_list_window = nullptr;
static Fl_Check_Button *g_direction_check_button = nullptr;
static Fl_Input *g_spawn_row_input = nullptr;
static Fl_Check_Button *g_spawn_every_row_check_button = nullptr;

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

    bool status = true;

    if (optionName == "AddPlantToGarden")
    {
        int plantID = 0;
        int direction = 0;

        if (input && input->value())
            plantID = std::stoi(input->value());

        if (g_direction_check_button)
            direction = g_direction_check_button->value() ? 1 : 0;

        status = trainer->addPlantToGarden(plantID, direction);
    }
    else if (optionName == "InstantCompleteLevel")
    {
        status = trainer->instantCompleteLevel();
    }
    else if (optionName == "SpawnZombie")
    {
        int zombieType = 0;
        int row = 0;
        bool isEveryRow = false;

        if (input && input->value())
            zombieType = std::stoi(input->value());

        if (g_spawn_row_input && g_spawn_row_input->value())
            row = std::stoi(g_spawn_row_input->value()) - 1;

        if (g_spawn_every_row_check_button)
            isEveryRow = g_spawn_every_row_check_button->value() != 0;

        status = trainer->spawnZombie(zombieType, row, isEveryRow);
    }
    else if (optionName == "FullScreenJalapeno")
    {
        status = trainer->fullScreenJalapeno();
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

    if (optionName == "NoPlantCooldown")
    {
        if (button->value())
            status = trainer->noPlantCooldown();
        else
            status = trainer->disableNamedHook(optionName);
    }
    else if (optionName == "NoPlantCost")
    {
        if (button->value())
            status = trainer->noPlantCost();
        else
            status = trainer->disableNamedHook("NoPlantCost1") && trainer->disableNamedHook("NoPlantCost2") && trainer->disableNamedHook("NoPlantCost3");
    }
    else if (optionName == "AddSun")
    {
        if (button->value())
            status = trainer->addSun(std::stoi(inputValue));
        else
            status = trainer->disableNamedHook(optionName);
    }
    else if (optionName == "SetCoin")
    {
        if (button->value())
            status = trainer->setCoin(std::stoi(inputValue) / 10);
        else
            status = trainer->disableNamedHook(optionName);
    }
    else if (optionName == "SetFertilizerAndBugSpray")
    {
        if (button->value())
            status = trainer->setFertilizerAndBugSpray(std::stoi(inputValue) + 1000);
        else
            status = trainer->disableNamedHook("SetFertilizerAndBugSprayInc") && trainer->disableNamedHook("SetFertilizerDec") && trainer->disableNamedHook("SetBugSprayDec");
    }
    else if (optionName == "SetChocolate")
    {
        if (button->value())
            status = trainer->setChocolate(std::stoi(inputValue) + 1000);
        else
            status = trainer->disableNamedHook("SetChocolateInc") && trainer->disableNamedHook("SetChocolateDec");
    }
    else if (optionName == "SetTreeFood")
    {
        if (button->value())
            status = trainer->setTreeFood(std::stoi(inputValue) + 1000);
        else
            status = trainer->disableNamedHook("TreeFoodInc") && trainer->disableNamedHook("TreeFoodDec");
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
    if (plant_list_window)
    {
        Fl::delete_widget(plant_list_window);
        plant_list_window = nullptr;
    }
    if (zombie_list_window)
    {
        Fl::delete_widget(zombie_list_window);
        zombie_list_window = nullptr;
    }
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
    tr(window, "Plants vs. Zombies: Replanted Trainer");

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
    place_toggle_widget(options1_flex, &trainer, "NoPlantCooldown", "No Plant Cooldown");

    place_toggle_widget(options1_flex, &trainer, "NoPlantCost", "No Plant Cost");

    place_toggle_widget(options1_flex, &trainer, "AddSun", "Add Sun", nullptr, "999", "0", "9990");

    place_toggle_widget(options1_flex, &trainer, "SetCoin", "Set Coins", nullptr, "9999999", "0", "999999990");

    place_toggle_widget(options1_flex, &trainer, "SetFertilizerAndBugSpray", "Set Fertilizer and Bug Spray", nullptr, "99", "0", "999999999");

    place_toggle_widget(options1_flex, &trainer, "SetChocolate", "Set Chocolate", nullptr, "99", "0", "999999999");

    place_toggle_widget(options1_flex, &trainer, "SetTreeFood", "Set Tree Food", nullptr, "99", "0", "999999999");

    Fl_Input *add_plant_input = nullptr;
    std::vector<std::string> plant_columns = {"Plant ID", "Plant Name"};
    Fl_Button *plant_info = create_info_button(&trainer, &add_plant_input, plant_columns, "Plant List", &plant_list_window, &Trainer::getPlantList, info_img);
    place_apply_widget(options1_flex, &trainer, "AddPlantToGarden", "Add Plant to Garden", &add_plant_input, "0", "0", "39", FL_INT_INPUT, plant_info);
    place_indented_toggle_widget(options1_flex, "Facing Left", &g_direction_check_button);

    Fl_Input *spawn_zombie_input = nullptr;
    std::vector<std::string> zombie_columns = {"Zombie ID", "Zombie Name"};
    Fl_Button *zombie_info = create_info_button(&trainer, &spawn_zombie_input, zombie_columns, "Zombie List", &zombie_list_window, &Trainer::getZombieList, info_img);
    place_apply_widget(options1_flex, &trainer, "SpawnZombie", "Spawn Zombie", &spawn_zombie_input, "0", "0", "99", FL_INT_INPUT, zombie_info);
    place_indented_input_widget(options1_flex, "Which Row", &g_spawn_row_input, "1", "1", "6");
    place_indented_toggle_widget(options1_flex, "Spawn in Every Row", &g_spawn_every_row_check_button);

    place_apply_widget(options1_flex, &trainer, "FullScreenJalapeno", "Full Screen Jalapeno");

    place_apply_widget(options1_flex, &trainer, "InstantCompleteLevel", "Instant Complete Level");

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
