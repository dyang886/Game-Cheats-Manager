// main.cpp
#include <FL/Fl.H>
#include <FL/Fl_Radio_Round_Button.H>
#include <FL/Fl_PNG_Image.H>
#include <FL/Fl_Flex.H>
#include <FL/Fl_Table.H>
#include <FL/forms.H>
#include "trainer.h"
#include "MonoBase.h"
#include "FLTKUtils.h"

static Fl_Window *info_window = nullptr;

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
    if (optionName == "SpawnItem")
    {
        status = trainer->spawnItem(std::stoi(inputValue));
    }
    else if (optionName == "AddFunds")
    {
        status = trainer->addFunds(std::stof(inputValue));
    }
    else if (optionName == "RepairAll")
    {
        status = trainer->repairAll();
    }
    else if (optionName == "RestockAll")
    {
        status = trainer->restockAll();
    }
    else if (optionName == "ClearWeather")
    {
        status = trainer->clearWeather();
    }

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
    if (optionName == "FreezeTime")
    {
        if (button->value())
        {
            status = trainer->freezeTime(true);
        }
        else
        {
            status = trainer->freezeTime(false);
        }
    }
    else if (optionName == "GodMode")
    {
        if (button->value())
        {
            status = trainer->godMode(true);
        }
        else
        {
            status = trainer->godMode(false);
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

struct InfoCallbackData
{
    Trainer *trainer;
    Fl_Input *input;
};

class ItemTable : public Fl_Table
{
private:
    std::vector<std::pair<int, std::string>> items;
    int selected_row = -1;
    Fl_Input *input;

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
            if (col == 0)
                fl_draw(t("Item ID"), x + 4, y, w, h, FL_ALIGN_LEFT);
            else
                fl_draw(t("Item Name"), x + 4, y, w, h, FL_ALIGN_LEFT);
            fl_pop_clip();
            return;
        case CONTEXT_CELL:
            fl_push_clip(x, y, w, h);
            fl_color(row == selected_row ? fl_lighter(FL_FREE_COLOR) : FL_FREE_COLOR);
            fl_rectf(x, y, w, h);
            fl_color(FL_WHITE);
            if (row < (int)items.size())
            {
                if (col == 0)
                {
                    fl_draw(std::to_string(items[row].first).c_str(), x + 4, y, w, h, FL_ALIGN_LEFT);
                }
                else
                {
                    fl_draw(items[row].second.c_str(), x + 4, y, w, h, FL_ALIGN_LEFT);
                }
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

        // Convert mouse coordinates to table-relative
        int mx = mouse_x - table_x;
        int my = mouse_y - table_y;

        int scroll_y = vscrollbar->value();
        int adjusted_my = my + scroll_y;

        // Check if mouse is within table bounds
        if (mx < 0 || mx >= table_w)
        {
            return false;
        }

        // Calculate row
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
        {
            return false;
        }

        // Calculate column
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
        {
            return false;
        }

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
                if (input)
                {
                    input->value(std::to_string(items[row].first).c_str());
                }
                redraw();
                return 1;
            }
            else
            {
                selected_row = -1; // Deselect if click outside valid cell
                redraw();
                return 1;
            }
        }
        else if (event == FL_RELEASE && Fl::event_button() == FL_LEFT_MOUSE && Fl::event_clicks() > 0)
        {
            if (find_cell_at(Fl::event_x(), Fl::event_y(), row, col))
            {
                selected_row = row;
                if (input)
                {
                    input->value(std::to_string(items[row].first).c_str());
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
    ItemTable(int x, int y, int w, int h, Fl_Input *inp, const char *l = 0) : Fl_Table(x, y, w, h, l), input(inp)
    {
        int scrollbarSize = 16;
        cols(2);
        col_header(1);
        col_width(0, 60);
        col_width(1, w - 60 - scrollbarSize);
        row_header(0);
        box(FL_NO_BOX);
        scrollbar_size(scrollbarSize);
        end();
    }

    void setItems(const std::vector<std::pair<int, std::string>> &item_list)
    {
        items = item_list;
        rows(item_list.size());
        selected_row = -1;
        redraw();
    }
};

void info_callback(Fl_Widget *widget, void *data)
{
    InfoCallbackData *info_data = static_cast<InfoCallbackData *>(data);
    Trainer *trainer = info_data->trainer;
    Fl_Input *input = info_data->input;

    if (info_window && info_window->shown())
    {
        info_window->show();
        return;
    }

    Fl_Window *trainer_window = widget->window();
    int trainer_x = trainer_window->x();
    int trainer_y = trainer_window->y();
    int trainer_w = trainer_window->w();
    int trainer_h = trainer_window->h();

    int info_w = 350;
    int info_h = 500;
    int info_x = trainer_x + (trainer_w - info_w) / 2;
    int info_y = trainer_y + (trainer_h - info_h) / 2;

    info_window = new Fl_Window(info_x, info_y, info_w, info_h, t("Item List"));
    info_window->icon((char *)LoadIconA(GetModuleHandle(NULL), "APP_ICON"));

    std::string itemList = trainer->getItemList();
    std::vector<std::pair<int, std::string>> items;
    std::istringstream iss(itemList);
    std::string line;
    while (std::getline(iss, line))
    {
        line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
        size_t pos = line.find('>');
        if (pos != std::string::npos)
        {
            try
            {
                int id = std::stoi(line.substr(0, pos));
                std::string name = line.substr(pos + 1);
                items.emplace_back(id, name);
            }
            catch (...)
            {
                // Skip malformed lines
            }
        }
    }

    ItemTable *table = new ItemTable(0, 0, info_w, info_h, input);
    table->setItems(items);
    table->color(FL_FREE_COLOR);

    info_window->end();
    info_window->show();
}

static void main_window_close_callback(Fl_Widget *w, void *)
{
    if (info_window)
    {
        Fl::delete_widget(info_window);
        info_window = nullptr;
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
    tr(window, "DREDGE Trainer");

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

    Fl::add_timeout(0, MonoBase::check_logging_available, &trainer);

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
    // god_mode (Toggle)
    // ------------------------------------------------------------------
    Fl_Flex *god_mode_flex = new Fl_Flex(0, 0, 0, 0, Fl_Flex::HORIZONTAL);
    god_mode_flex->gap(option_gap);

    Fl_Check_Button *god_mode_check_button = new Fl_Check_Button(0, 0, 0, 0);
    god_mode_flex->fixed(god_mode_check_button, button_w);

    Fl_Box *god_mode_label = new Fl_Box(0, 0, 0, 0);
    tr(god_mode_label, "God Mode");

    ToggleData *data_god_mode = new ToggleData{&trainer, "GodMode", god_mode_check_button, nullptr};
    god_mode_check_button->callback(toggle_callback, data_god_mode);

    god_mode_flex->end();
    options1_flex->fixed(god_mode_flex, option_h);

    // ------------------------------------------------------------------
    // Spawn item (Apply)
    // ------------------------------------------------------------------
    Fl_Flex *spawn_item_flex = new Fl_Flex(0, 0, 0, 0, Fl_Flex::HORIZONTAL);
    spawn_item_flex->gap(option_gap);

    Fl_Button *spawn_item_apply_button = new Fl_Button(0, 0, 0, 0);
    spawn_item_flex->fixed(spawn_item_apply_button, button_w);
    tr(spawn_item_apply_button, "Apply");

    Fl_Box *spawn_item_label = new Fl_Box(0, 0, 0, 0);
    tr(spawn_item_label, "Spawn Item");

    Fl_Input *spawn_item_input = new Fl_Input(0, 0, 0, 0);
    spawn_item_flex->fixed(spawn_item_input, input_w);
    spawn_item_input->type(FL_INT_INPUT);
    set_input_values(spawn_item_input, "0", "0", "419");

    DWORD info_img_size = 0;
    const unsigned char *info_img_data = load_resource("INFO_IMG", info_img_size);
    Fl_PNG_Image *info_img = new Fl_PNG_Image(nullptr, info_img_data, (int)info_img_size);
    info_img->scale(20, 20, 1, 0);

    Fl_Button *info_icon = new Fl_Button(0, 0, 0, 0);
    spawn_item_flex->fixed(info_icon, 20);
    spawn_item_flex->add(info_icon);
    info_icon->box(FL_NO_BOX);
    info_icon->image(info_img);
    InfoCallbackData *info_data = new InfoCallbackData{&trainer, spawn_item_input};
    info_icon->callback(info_callback, info_data);

    ApplyData *ad_spawn_item = new ApplyData{&trainer, "SpawnItem", spawn_item_apply_button, spawn_item_input};
    spawn_item_apply_button->callback(apply_callback, ad_spawn_item);

    spawn_item_flex->end();
    options1_flex->fixed(spawn_item_flex, option_h);

    // ------------------------------------------------------------------
    // Add funds (Apply)
    // ------------------------------------------------------------------
    Fl_Flex *funds_flex = new Fl_Flex(0, 0, 0, 0, Fl_Flex::HORIZONTAL);
    funds_flex->gap(option_gap);

    Fl_Button *funds_apply_button = new Fl_Button(0, 0, 0, 0);
    funds_flex->fixed(funds_apply_button, button_w);
    tr(funds_apply_button, "Apply");

    Fl_Box *funds_label = new Fl_Box(0, 0, 0, 0);
    tr(funds_label, "Add Funds");

    Fl_Input *funds_input = new Fl_Input(0, 0, 0, 0);
    funds_flex->fixed(funds_input, input_w);
    funds_input->type(FL_INT_INPUT);
    set_input_values(funds_input, "999999", "-999999999", "999999999");

    ApplyData *data_funds = new ApplyData{&trainer, "AddFunds", funds_apply_button, funds_input};
    funds_apply_button->callback(apply_callback, data_funds);

    funds_flex->end();
    options1_flex->fixed(funds_flex, option_h);

    // ------------------------------------------------------------------
    // repair_all (Apply)
    // ------------------------------------------------------------------
    Fl_Flex *repair_all_flex = new Fl_Flex(0, 0, 0, 0, Fl_Flex::HORIZONTAL);
    repair_all_flex->gap(option_gap);

    Fl_Button *repair_all_apply_button = new Fl_Button(0, 0, 0, 0);
    repair_all_flex->fixed(repair_all_apply_button, button_w);
    tr(repair_all_apply_button, "Apply");

    Fl_Box *repair_all_label = new Fl_Box(0, 0, 0, 0);
    tr(repair_all_label, "Repair All");

    ApplyData *data_repair_all = new ApplyData{&trainer, "RepairAll", repair_all_apply_button, nullptr};
    repair_all_apply_button->callback(apply_callback, data_repair_all);

    repair_all_flex->end();
    options1_flex->fixed(repair_all_flex, option_h);

    // ------------------------------------------------------------------
    // restock_all (Apply)
    // ------------------------------------------------------------------
    Fl_Flex *restock_all_flex = new Fl_Flex(0, 0, 0, 0, Fl_Flex::HORIZONTAL);
    restock_all_flex->gap(option_gap);

    Fl_Button *restock_all_apply_button = new Fl_Button(0, 0, 0, 0);
    restock_all_flex->fixed(restock_all_apply_button, button_w);
    tr(restock_all_apply_button, "Apply");

    Fl_Box *restock_all_label = new Fl_Box(0, 0, 0, 0);
    tr(restock_all_label, "Restock All");

    ApplyData *data_restock_all = new ApplyData{&trainer, "RestockAll", restock_all_apply_button, nullptr};
    restock_all_apply_button->callback(apply_callback, data_restock_all);

    restock_all_flex->end();
    options1_flex->fixed(restock_all_flex, option_h);

    // ------------------------------------------------------------------
    // clear_weather (Apply)
    // ------------------------------------------------------------------
    Fl_Flex *clear_weather_flex = new Fl_Flex(0, 0, 0, 0, Fl_Flex::HORIZONTAL);
    clear_weather_flex->gap(option_gap);

    Fl_Button *clear_weather_apply_button = new Fl_Button(0, 0, 0, 0);
    clear_weather_flex->fixed(clear_weather_apply_button, button_w);
    tr(clear_weather_apply_button, "Apply");

    Fl_Box *clear_weather_label = new Fl_Box(0, 0, 0, 0);
    tr(clear_weather_label, "Clear Weather");

    ApplyData *data_clear_weather = new ApplyData{&trainer, "ClearWeather", clear_weather_apply_button, nullptr};
    clear_weather_apply_button->callback(apply_callback, data_clear_weather);

    clear_weather_flex->end();
    options1_flex->fixed(clear_weather_flex, option_h);

    // ------------------------------------------------------------------
    // freeze_time (Toggle)
    // ------------------------------------------------------------------
    Fl_Flex *freeze_time_flex = new Fl_Flex(0, 0, 0, 0, Fl_Flex::HORIZONTAL);
    freeze_time_flex->gap(option_gap);

    Fl_Check_Button *freeze_time_check_button = new Fl_Check_Button(0, 0, 0, 0);
    freeze_time_flex->fixed(freeze_time_check_button, button_w);

    Fl_Box *freeze_time_label = new Fl_Box(0, 0, 0, 0);
    tr(freeze_time_label, "Freeze Time");

    ToggleData *data_freeze_time = new ToggleData{&trainer, "FreezeTime", freeze_time_check_button, nullptr};
    freeze_time_check_button->callback(toggle_callback, data_freeze_time);

    freeze_time_flex->end();
    options1_flex->fixed(freeze_time_flex, option_h);

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
