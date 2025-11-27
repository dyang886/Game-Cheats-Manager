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

    // Check if the game process is running
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

    // Retrieve input values if available
    std::string inputValue = "0";
    bool status = true;
    if (input && input->value())
        inputValue = input->value();

    // Apply the value using the Trainer class
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

struct InfoCallbackData
{
    Trainer *trainer;
    Fl_Input *input;
    const char *windowTitleKey;              // Key for window title translation
    Fl_Window **windowInstance;              // Pointer to window instance (for reuse management)
    std::vector<std::string> columnTitles;   // Column headers
    std::string (Trainer::*dataRetriever)(); // Pointer to Trainer member function that returns std::string
};

class ItemTable : public Fl_Table
{
private:
    std::vector<std::vector<std::string>> items;
    int selected_row = -1;
    Fl_Input *input;
    std::vector<std::string> columnTitles;

protected:
    void draw_cell(TableContext context, int row, int col, int x, int y, int w, int h) override
    {
        switch (context)
        {
        case CONTEXT_STARTPAGE:
            fl_font(FL_FREE_FONT, font_size);
            return;
        case CONTEXT_COL_HEADER:
            fl_push_clip(x, y, w, h);
            fl_color(FL_FREE_COLOR);
            fl_rectf(x, y, w, h);
            fl_color(FL_WHITE);
            if (col >= 0 && col < (int)columnTitles.size())
            {
                fl_draw(t(columnTitles[col]), x + 4, y, w, h, FL_ALIGN_LEFT);
            }
            fl_pop_clip();
            return;
        case CONTEXT_CELL:
            fl_push_clip(x, y, w, h);
            fl_color(row == selected_row ? fl_lighter(FL_FREE_COLOR) : FL_FREE_COLOR);
            fl_rectf(x, y, w, h);
            fl_color(FL_WHITE);
            if (row < (int)items.size() && col < (int)items[row].size())
            {
                fl_draw(items[row][col].c_str(), x + 4, y, w, h, FL_ALIGN_LEFT);
            }
            fl_pop_clip();
            return;
        default:
            return;
        }
    }

    bool find_cell_at(int mouse_x, int mouse_y, int &row, int &col)
    {
        int table_x = x();
        int table_y = y();
        int table_w = w() - scrollbar_size();
        int table_h = h() - scrollbar_size();

        int mx = mouse_x - table_x;
        int my = mouse_y - table_y;

        int scroll_y = vscrollbar->value();
        int adjusted_my = my + scroll_y;

        if (mx < 0 || mx >= table_w)
            return false;

        int y_offset = col_header_height();
        row = -1;
        for (int r = 0; r < rows(); r++)
        {
            int rh = row_height(r);
            if (adjusted_my >= y_offset && adjusted_my < y_offset + rh)
            {
                row = r;
                break;
            }
            y_offset += rh;
        }
        if (row < 0 || row >= (int)items.size())
            return false;

        int x_offset = 0;
        col = -1;
        for (int c = 0; c < cols(); c++)
        {
            int cw = col_width(c);
            if (mx >= x_offset && mx < x_offset + cw)
            {
                col = c;
                break;
            }
            x_offset += cw;
        }
        if (col < 0 || col >= cols())
            return false;

        return true;
    }

    int handle(int event) override
    {
        int ret = Fl_Table::handle(event);
        int row, col;

        if (event == FL_PUSH && Fl::event_button() == FL_LEFT_MOUSE)
        {
            if (find_cell_at(Fl::event_x(), Fl::event_y(), row, col))
            {
                selected_row = row;
                if (input && row < (int)items.size() && !items[row].empty())
                {
                    input->value(items[row][0].c_str());
                }
                redraw();
                return 1;
            }
            else
            {
                selected_row = -1;
                redraw();
                return 1;
            }
        }
        else if (event == FL_RELEASE && Fl::event_button() == FL_LEFT_MOUSE && Fl::event_clicks() > 0)
        {
            if (find_cell_at(Fl::event_x(), Fl::event_y(), row, col))
            {
                selected_row = row;
                if (input && row < (int)items.size() && !items[row].empty())
                {
                    input->value(items[row][0].c_str());
                }
                Fl_Window *win = dynamic_cast<Fl_Window *>(parent()->top_window());
                if (win)
                {
                    win->hide();
                }
                redraw();
                return 1;
            }
        }
        return ret;
    }

public:
    ItemTable(int x, int y, int w, int h, Fl_Input *inp, const std::vector<std::string> &titles, const char *l = 0) : Fl_Table(x, y, w, h, l), input(inp), columnTitles(titles)
    {
        int scrollbarSize = 16;
        int numCols = titles.size();
        cols(numCols);
        col_header(1);

        // Set column widths: first column (ID) is narrower, remaining columns share the rest
        int availableWidth = w - scrollbarSize;
        int idColWidth = 100;
        int otherColsWidth = (availableWidth - idColWidth) / (numCols - 1);

        col_width(0, idColWidth);
        for (int i = 1; i < numCols; i++)
        {
            col_width(i, otherColsWidth);
        }

        row_header(0);
        box(FL_NO_BOX);
        scrollbar_size(scrollbarSize);
        end();
    }

    void setItems(const std::vector<std::vector<std::string>> &item_list)
    {
        items = item_list;
        rows(item_list.size());
        selected_row = -1;
        redraw();
    }
};

// Callback function for info buttons
void info_callback(Fl_Widget *widget, void *data)
{
    InfoCallbackData *info_data = static_cast<InfoCallbackData *>(data);
    if (!info_data || !info_data->trainer || info_data->columnTitles.empty() || !info_data->windowInstance)
        return;

    Trainer *trainer = info_data->trainer;
    Fl_Input *input = info_data->input;
    const std::vector<std::string> &columnTitles = info_data->columnTitles;
    const char *windowTitleKey = info_data->windowTitleKey;
    Fl_Window *&list_window = *info_data->windowInstance;

    if (!trainer->isProcessRunning())
    {
        fl_alert(t("Please run the game first."));
        return;
    }

    // Call the member function pointer to get the list data
    std::string itemListData = (trainer->*info_data->dataRetriever)();

    if (list_window && list_window->shown())
    {
        list_window->show();
        return;
    }

    Fl_Window *trainer_window = widget->window();
    int trainer_x = trainer_window->x();
    int trainer_y = trainer_window->y();
    int trainer_w = trainer_window->w();
    int trainer_h = trainer_window->h();

    int list_w = 350;
    int list_h = 500;
    int list_x = trainer_x + (trainer_w - list_w) / 2;
    int list_y = trainer_y + (trainer_h - list_h) / 2;

    list_window = new Fl_Window(list_x, list_y, list_w, list_h, t(windowTitleKey));
    list_window->icon((char *)LoadIconA(GetModuleHandle(NULL), "APP_ICON"));

    std::vector<std::vector<std::string>> items;
    int numCols = columnTitles.size();
    std::istringstream iss(itemListData);
    std::string line;

    while (std::getline(iss, line))
    {
        line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());

        // Parse columns separated by '>'
        std::vector<std::string> rowData;
        std::istringstream lineStream(line);
        std::string column;

        while (std::getline(lineStream, column, '>'))
        {
            const char *translatedText = t(column);
            std::string displayText = (translatedText && *translatedText) ? std::string(translatedText) : column;
            rowData.push_back(displayText);
        }

        if ((int)rowData.size() >= numCols)
        {
            rowData.resize(numCols);
            items.push_back(rowData);
        }
    }

    ItemTable *table = new ItemTable(0, 0, list_w, list_h, input, columnTitles);
    table->setItems(items);
    table->color(FL_FREE_COLOR);

    list_window->end();
    list_window->show();
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

    int left_margin = 20;
    int button_w = 50;
    int toggle_w = 16;
    int toggle_spacer_w = 24;
    int input_w = 200;
    int option_gap = 10;
    int option_h = static_cast<int>(font_size * 1.5);

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
    // no_cooldown (Toggle)
    // ------------------------------------------------------------------
    Fl_Flex *no_cooldown_flex = new Fl_Flex(0, 0, 0, 0, Fl_Flex::HORIZONTAL);
    no_cooldown_flex->gap(option_gap);

    Fl_Box *no_cooldown_toggle_spacer = new Fl_Box(0, 0, 0, 0);
    no_cooldown_flex->fixed(no_cooldown_toggle_spacer, toggle_spacer_w);

    Fl_Check_Button *no_cooldown_check_button = new Fl_Check_Button(0, 0, 0, 0);
    no_cooldown_flex->fixed(no_cooldown_check_button, toggle_w);

    Fl_Box *no_cooldown_label = new Fl_Box(0, 0, 0, 0);
    tr(no_cooldown_label, "No Plant Cooldown");

    ToggleData *data_no_cooldown = new ToggleData{&trainer, "NoPlantCooldown", no_cooldown_check_button, nullptr};
    no_cooldown_check_button->callback(toggle_callback, data_no_cooldown);

    no_cooldown_flex->end();
    options1_flex->fixed(no_cooldown_flex, option_h);

    // ------------------------------------------------------------------
    // no_plant_cost (Toggle)
    // ------------------------------------------------------------------
    Fl_Flex *no_plant_cost_flex = new Fl_Flex(0, 0, 0, 0, Fl_Flex::HORIZONTAL);
    no_plant_cost_flex->gap(option_gap);

    Fl_Box *no_plant_cost_toggle_spacer = new Fl_Box(0, 0, 0, 0);
    no_plant_cost_flex->fixed(no_plant_cost_toggle_spacer, toggle_spacer_w);

    Fl_Check_Button *no_plant_cost_check_button = new Fl_Check_Button(0, 0, 0, 0);
    no_plant_cost_flex->fixed(no_plant_cost_check_button, toggle_w);

    Fl_Box *no_plant_cost_label = new Fl_Box(0, 0, 0, 0);
    tr(no_plant_cost_label, "No Plant Cost");

    ToggleData *data_no_plant_cost = new ToggleData{&trainer, "NoPlantCost", no_plant_cost_check_button, nullptr};
    no_plant_cost_check_button->callback(toggle_callback, data_no_plant_cost);

    no_plant_cost_flex->end();
    options1_flex->fixed(no_plant_cost_flex, option_h);

    // ------------------------------------------------------------------
    // Add sun (Toggle)
    // ------------------------------------------------------------------
    Fl_Flex *sun_flex = new Fl_Flex(0, 0, 0, 0, Fl_Flex::HORIZONTAL);
    sun_flex->gap(option_gap);

    Fl_Box *sun_toggle_spacer = new Fl_Box(0, 0, 0, 0);
    sun_flex->fixed(sun_toggle_spacer, toggle_spacer_w);

    Fl_Check_Button *sun_check_button = new Fl_Check_Button(0, 0, 0, 0);
    sun_flex->fixed(sun_check_button, toggle_w);

    Fl_Box *sun_label = new Fl_Box(0, 0, 0, 0);
    tr(sun_label, "Add Sun");

    Fl_Input *sun_input = new Fl_Input(0, 0, 0, 0);
    sun_flex->fixed(sun_input, input_w);
    sun_input->type(FL_INT_INPUT);
    set_input_values(sun_input, "999", "0", "9990");

    ToggleData *td_sun = new ToggleData{&trainer, "AddSun", sun_check_button, sun_input};
    sun_check_button->callback(toggle_callback, td_sun);

    sun_flex->end();
    options1_flex->fixed(sun_flex, option_h);

    // ------------------------------------------------------------------
    // Set coin (Toggle)
    // ------------------------------------------------------------------
    Fl_Flex *coin_flex = new Fl_Flex(0, 0, 0, 0, Fl_Flex::HORIZONTAL);
    coin_flex->gap(option_gap);

    Fl_Box *coin_toggle_spacer = new Fl_Box(0, 0, 0, 0);
    coin_flex->fixed(coin_toggle_spacer, toggle_spacer_w);

    Fl_Check_Button *coin_check_button = new Fl_Check_Button(0, 0, 0, 0);
    coin_flex->fixed(coin_check_button, toggle_w);

    Fl_Box *coin_label = new Fl_Box(0, 0, 0, 0);
    tr(coin_label, "Set Coins");

    Fl_Input *coin_input = new Fl_Input(0, 0, 0, 0);
    coin_flex->fixed(coin_input, input_w);
    coin_input->type(FL_INT_INPUT);
    set_input_values(coin_input, "9999999", "0", "999999990");

    ToggleData *td_coin = new ToggleData{&trainer, "SetCoin", coin_check_button, coin_input};
    coin_check_button->callback(toggle_callback, td_coin);

    coin_flex->end();
    options1_flex->fixed(coin_flex, option_h);

    // ------------------------------------------------------------------
    // Set fertilizer and bug spray (Toggle)
    // ------------------------------------------------------------------
    Fl_Flex *fertilizer_flex = new Fl_Flex(0, 0, 0, 0, Fl_Flex::HORIZONTAL);
    fertilizer_flex->gap(option_gap);

    Fl_Box *fertilizer_toggle_spacer = new Fl_Box(0, 0, 0, 0);
    fertilizer_flex->fixed(fertilizer_toggle_spacer, toggle_spacer_w);

    Fl_Check_Button *fertilizer_check_button = new Fl_Check_Button(0, 0, 0, 0);
    fertilizer_flex->fixed(fertilizer_check_button, toggle_w);

    Fl_Box *fertilizer_label = new Fl_Box(0, 0, 0, 0);
    tr(fertilizer_label, "Set Fertilizer and Bug Spray");

    Fl_Input *fertilizer_input = new Fl_Input(0, 0, 0, 0);
    fertilizer_flex->fixed(fertilizer_input, input_w);
    fertilizer_input->type(FL_INT_INPUT);
    set_input_values(fertilizer_input, "99", "0", "999999999");

    ToggleData *td_fertilizer = new ToggleData{&trainer, "SetFertilizerAndBugSpray", fertilizer_check_button, fertilizer_input};
    fertilizer_check_button->callback(toggle_callback, td_fertilizer);

    fertilizer_flex->end();
    options1_flex->fixed(fertilizer_flex, option_h);

    // ------------------------------------------------------------------
    // Set chocolate (Toggle)
    // ------------------------------------------------------------------
    Fl_Flex *chocolate_flex = new Fl_Flex(0, 0, 0, 0, Fl_Flex::HORIZONTAL);
    chocolate_flex->gap(option_gap);

    Fl_Box *chocolate_toggle_spacer = new Fl_Box(0, 0, 0, 0);
    chocolate_flex->fixed(chocolate_toggle_spacer, toggle_spacer_w);

    Fl_Check_Button *chocolate_check_button = new Fl_Check_Button(0, 0, 0, 0);
    chocolate_flex->fixed(chocolate_check_button, toggle_w);

    Fl_Box *chocolate_label = new Fl_Box(0, 0, 0, 0);
    tr(chocolate_label, "Set Chocolate");

    Fl_Input *chocolate_input = new Fl_Input(0, 0, 0, 0);
    chocolate_flex->fixed(chocolate_input, input_w);
    chocolate_input->type(FL_INT_INPUT);
    set_input_values(chocolate_input, "99", "0", "999999999");

    ToggleData *td_chocolate = new ToggleData{&trainer, "SetChocolate", chocolate_check_button, chocolate_input};
    chocolate_check_button->callback(toggle_callback, td_chocolate);

    chocolate_flex->end();
    options1_flex->fixed(chocolate_flex, option_h);

    // ------------------------------------------------------------------
    // Set tree_food (Toggle)
    // ------------------------------------------------------------------
    Fl_Flex *tree_food_flex = new Fl_Flex(0, 0, 0, 0, Fl_Flex::HORIZONTAL);
    tree_food_flex->gap(option_gap);

    Fl_Box *tree_food_toggle_spacer = new Fl_Box(0, 0, 0, 0);
    tree_food_flex->fixed(tree_food_toggle_spacer, toggle_spacer_w);

    Fl_Check_Button *tree_food_check_button = new Fl_Check_Button(0, 0, 0, 0);
    tree_food_flex->fixed(tree_food_check_button, toggle_w);

    Fl_Box *tree_food_label = new Fl_Box(0, 0, 0, 0);
    tr(tree_food_label, "Set Tree Food");

    Fl_Input *tree_food_input = new Fl_Input(0, 0, 0, 0);
    tree_food_flex->fixed(tree_food_input, input_w);
    tree_food_input->type(FL_INT_INPUT);
    set_input_values(tree_food_input, "99", "0", "999999999");

    ToggleData *td_tree_food = new ToggleData{&trainer, "SetTreeFood", tree_food_check_button, tree_food_input};
    tree_food_check_button->callback(toggle_callback, td_tree_food);

    tree_food_flex->end();
    options1_flex->fixed(tree_food_flex, option_h);

    // ------------------------------------------------------------------
    // add_plant (Apply)
    // ------------------------------------------------------------------
    Fl_Flex *add_plant_flex = new Fl_Flex(0, 0, 0, 0, Fl_Flex::HORIZONTAL);
    add_plant_flex->gap(option_gap);

    Fl_Button *add_plant_apply_button = new Fl_Button(0, 0, 0, 0);
    add_plant_flex->fixed(add_plant_apply_button, button_w);
    tr(add_plant_apply_button, "Apply");

    Fl_Box *add_plant_label = new Fl_Box(0, 0, 0, 0);
    tr(add_plant_label, "Add Plant to Garden");

    Fl_Input *add_plant_input = new Fl_Input(0, 0, 0, 0);
    add_plant_flex->fixed(add_plant_input, input_w);
    add_plant_input->type(FL_INT_INPUT);
    set_input_values(add_plant_input, "0", "0", "39");

    // Info button
    Fl_Button *plant_info_icon = new Fl_Button(0, 0, 0, 0);
    add_plant_flex->fixed(plant_info_icon, 20);
    add_plant_flex->add(plant_info_icon);
    plant_info_icon->box(FL_NO_BOX);
    plant_info_icon->image(info_img);
    std::vector<std::string> plant_columns = {"Plant ID", "Plant Name"};
    InfoCallbackData *plant_info_data = new InfoCallbackData{&trainer, add_plant_input, "Plant List", &plant_list_window, plant_columns, &Trainer::getPlantList};
    plant_info_icon->callback(info_callback, plant_info_data);

    ApplyData *data_add_plant = new ApplyData{&trainer, "AddPlantToGarden", add_plant_apply_button, add_plant_input};
    add_plant_apply_button->callback(apply_callback, data_add_plant);

    add_plant_flex->end();
    options1_flex->fixed(add_plant_flex, option_h);

    // sub-control - facing direction (Toggle)
    Fl_Flex *direction_flex = new Fl_Flex(0, 0, 0, 0, Fl_Flex::HORIZONTAL);
    direction_flex->gap(option_gap);

    Fl_Box *direction_indent_spacer = new Fl_Box(0, 0, 0, 0);
    direction_flex->fixed(direction_indent_spacer, button_w);

    Fl_Box *direction_toggle_spacer = new Fl_Box(0, 0, 0, 0);
    direction_flex->fixed(direction_toggle_spacer, toggle_spacer_w);

    Fl_Check_Button *direction_check_button = new Fl_Check_Button(0, 0, 0, 0);
    direction_flex->fixed(direction_check_button, toggle_w);
    g_direction_check_button = direction_check_button; // Store globally for add_plant callback

    Fl_Box *direction_label = new Fl_Box(0, 0, 0, 0);
    tr(direction_label, "Facing Left");

    direction_flex->end();
    options1_flex->fixed(direction_flex, option_h);

    // ------------------------------------------------------------------
    // spawn_zombie (Apply)
    // ------------------------------------------------------------------
    Fl_Flex *spawn_zombie_flex = new Fl_Flex(0, 0, 0, 0, Fl_Flex::HORIZONTAL);
    spawn_zombie_flex->gap(option_gap);

    Fl_Button *spawn_zombie_apply_button = new Fl_Button(0, 0, 0, 0);
    spawn_zombie_flex->fixed(spawn_zombie_apply_button, button_w);
    tr(spawn_zombie_apply_button, "Apply");

    Fl_Box *spawn_zombie_label = new Fl_Box(0, 0, 0, 0);
    tr(spawn_zombie_label, "Spawn Zombie");

    Fl_Input *spawn_zombie_input = new Fl_Input(0, 0, 0, 0);
    spawn_zombie_flex->fixed(spawn_zombie_input, input_w);
    spawn_zombie_input->type(FL_INT_INPUT);
    set_input_values(spawn_zombie_input, "0", "0", "99");

    // Info button
    Fl_Button *spawn_zombie_info_icon = new Fl_Button(0, 0, 0, 0);
    spawn_zombie_flex->fixed(spawn_zombie_info_icon, 20);
    spawn_zombie_flex->add(spawn_zombie_info_icon);
    spawn_zombie_info_icon->box(FL_NO_BOX);
    spawn_zombie_info_icon->image(info_img);
    std::vector<std::string> zombie_columns = {"Zombie ID", "Zombie Name"};
    InfoCallbackData *spawn_zombie_info_data = new InfoCallbackData{&trainer, spawn_zombie_input, "Zombie List", &zombie_list_window, zombie_columns, &Trainer::getZombieList};
    spawn_zombie_info_icon->callback(info_callback, spawn_zombie_info_data);

    ApplyData *data_spawn_zombie = new ApplyData{&trainer, "SpawnZombie", spawn_zombie_apply_button, spawn_zombie_input};
    spawn_zombie_apply_button->callback(apply_callback, data_spawn_zombie);

    spawn_zombie_flex->end();
    options1_flex->fixed(spawn_zombie_flex, option_h);

    // sub-control - row number (Input)
    Fl_Flex *spawn_row_flex = new Fl_Flex(0, 0, 0, 0, Fl_Flex::HORIZONTAL);
    spawn_row_flex->gap(option_gap);

    Fl_Box *spawn_row_indent_spacer = new Fl_Box(0, 0, 0, 0);
    spawn_row_flex->fixed(spawn_row_indent_spacer, button_w);

    Fl_Input *spawn_row_input = new Fl_Input(0, 0, 0, 0);
    spawn_row_flex->fixed(spawn_row_input, button_w);
    spawn_row_input->type(FL_INT_INPUT);
    set_input_values(spawn_row_input, "1", "1", "6");
    g_spawn_row_input = spawn_row_input;

    Fl_Box *spawn_row_label = new Fl_Box(0, 0, 0, 0);
    tr(spawn_row_label, "Which Row");

    spawn_row_flex->end();
    options1_flex->fixed(spawn_row_flex, option_h);

    // sub-control - spawn on every row (Toggle)
    Fl_Flex *spawn_every_row_flex = new Fl_Flex(0, 0, 0, 0, Fl_Flex::HORIZONTAL);
    spawn_every_row_flex->gap(option_gap);

    Fl_Box *spawn_every_row_indent_spacer = new Fl_Box(0, 0, 0, 0);
    spawn_every_row_flex->fixed(spawn_every_row_indent_spacer, button_w);

    Fl_Box *spawn_every_row_toggle_spacer = new Fl_Box(0, 0, 0, 0);
    spawn_every_row_flex->fixed(spawn_every_row_toggle_spacer, toggle_spacer_w);

    Fl_Check_Button *spawn_every_row_check_button = new Fl_Check_Button(0, 0, 0, 0);
    spawn_every_row_flex->fixed(spawn_every_row_check_button, toggle_w);
    g_spawn_every_row_check_button = spawn_every_row_check_button;

    Fl_Box *spawn_every_row_label = new Fl_Box(0, 0, 0, 0);
    tr(spawn_every_row_label, "Spawn in Every Row");

    spawn_every_row_flex->end();
    options1_flex->fixed(spawn_every_row_flex, option_h);

    // ------------------------------------------------------------------
    // full_screen_jalapeno (Apply)
    // ------------------------------------------------------------------
    Fl_Flex *full_screen_jalapeno_flex = new Fl_Flex(0, 0, 0, 0, Fl_Flex::HORIZONTAL);
    full_screen_jalapeno_flex->gap(option_gap);

    Fl_Button *full_screen_jalapeno_apply_button = new Fl_Button(0, 0, 0, 0);
    full_screen_jalapeno_flex->fixed(full_screen_jalapeno_apply_button, button_w);
    tr(full_screen_jalapeno_apply_button, "Apply");

    Fl_Box *full_screen_jalapeno_label = new Fl_Box(0, 0, 0, 0);
    tr(full_screen_jalapeno_label, "Full Screen Jalapeno");

    ApplyData *data_full_screen_jalapeno = new ApplyData{&trainer, "FullScreenJalapeno", full_screen_jalapeno_apply_button, nullptr};
    full_screen_jalapeno_apply_button->callback(apply_callback, data_full_screen_jalapeno);

    full_screen_jalapeno_flex->end();
    options1_flex->fixed(full_screen_jalapeno_flex, option_h);

    // ------------------------------------------------------------------
    // instant_complete_level (Apply)
    // ------------------------------------------------------------------
    Fl_Flex *instant_complete_flex = new Fl_Flex(0, 0, 0, 0, Fl_Flex::HORIZONTAL);
    instant_complete_flex->gap(option_gap);

    Fl_Button *instant_complete_apply_button = new Fl_Button(0, 0, 0, 0);
    instant_complete_flex->fixed(instant_complete_apply_button, button_w);
    tr(instant_complete_apply_button, "Apply");

    Fl_Box *instant_complete_label = new Fl_Box(0, 0, 0, 0);
    tr(instant_complete_label, "Instant Complete Level");

    ApplyData *data_instant_complete = new ApplyData{&trainer, "InstantCompleteLevel", instant_complete_apply_button, nullptr};
    instant_complete_apply_button->callback(apply_callback, data_instant_complete);

    instant_complete_flex->end();
    options1_flex->fixed(instant_complete_flex, option_h);

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
