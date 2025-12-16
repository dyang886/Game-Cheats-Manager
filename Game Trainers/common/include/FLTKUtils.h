#include <nlohmann/json.hpp>
#include <fstream>
#include <shlobj.h>
#include <FL/Fl_Table.H>

using json = nlohmann::json;

// Globals
constexpr int font_size = 15;
static HANDLE font_handle = nullptr;
std::unordered_map<std::string, std::unordered_map<std::string, std::string>> translations;
std::vector<std::pair<Fl_Widget *, std::string>> translatable_widgets;
std::string language = "en_US";
std::string settings_path = "";

// UI Layout Constants
constexpr int UI_LEFT_MARGIN = 20;
constexpr int UI_BUTTON_WIDTH = 50;
constexpr int UI_TOGGLE_WIDTH = 16;
constexpr int UI_TOGGLE_SPACER_WIDTH = 24;
constexpr int UI_INPUT_WIDTH = 200;
constexpr int UI_OPTION_GAP = 10;
constexpr int UI_OPTION_HEIGHT = static_cast<int>(font_size * 1.5);

// ===========================================================================
// Data Structures
// ===========================================================================

struct TimeoutData
{
    Trainer *trainer;
    Fl_Box *process_exe;
    Fl_Box *process_id;
};

struct ChangeLanguageData
{
    std::string lang;
    Fl_Group *group;
};

// Structure to hold callback data for Apply button
struct ApplyData
{
    Trainer *trainer;
    std::string optionName;
    Fl_Button *button;
    Fl_Input *input;
};

// Structure to hold callback data for toggles
struct ToggleData
{
    Trainer *trainer;
    std::string optionName;
    Fl_Check_Button *button;
    Fl_Input *input;
};

// Structure to hold a widget pair (button and input)
struct WidgetPair
{
    Fl_Widget *button; // Fl_Check_Button* or Fl_Button*
    Fl_Input *input;   // nullptr if input not created
};

// Structure to hold callback data for info buttons
struct InfoCallbackData
{
    Trainer *trainer;
    Fl_Input **input_ptr;
    std::string windowTitleKey;
    Fl_Window **windowInstance;
    std::vector<std::string> columnTitles;
    std::string (Trainer::*dataRetriever)();
};

// ===========================================================================
// Resource Management Functions
// ===========================================================================

const unsigned char *load_resource(const char *resource_name, DWORD &size)
{
    HRSRC hRes = FindResourceA(nullptr, resource_name, MAKEINTRESOURCEA(10));
    if (!hRes)
    {
        size = 0;
        return nullptr;
    }
    HGLOBAL hData = LoadResource(nullptr, hRes);
    if (!hData)
    {
        size = 0;
        return nullptr;
    }
    size = SizeofResource(nullptr, hRes);

    if (size == 0)
    {
        std::cerr << "Error: Failed to load resource: " << resource_name << std::endl;
        return nullptr;
    }

    return static_cast<const unsigned char *>(LockResource(hData));
}

void load_translations(const char *resource_name)
{
    DWORD size = 0;
    const unsigned char *data = load_resource(resource_name, size);

    json j = json::parse(std::string(reinterpret_cast<const char *>(data), size));
    for (auto &lang : j.items())
    {
        std::string lang_code = lang.key();
        for (auto &text : lang.value().items())
        {
            translations[lang_code][text.key()] = text.value();
        }
    }
}

void saveSettings()
{
    json settings;

    if (std::filesystem::exists(settings_path))
    {
        std::ifstream file(settings_path);
        file >> settings;
        file.close();
    }
    else
    {
        std::filesystem::create_directories(std::filesystem::path(settings_path).parent_path());
    }

    settings["language"] = language;

    std::ofstream file(settings_path);
    file << settings.dump(4);
    file.close();
}

void getSettingsPath()
{
    PWSTR path = nullptr;
    SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &path);
    std::filesystem::path app_data_path(path);
    std::filesystem::path full_path = app_data_path / "GCM Settings" / "trainers.json";
    settings_path = full_path.string();
    CoTaskMemFree(path);
}

void setupLanguage()
{
    getSettingsPath();

    if (std::filesystem::exists(settings_path))
    {
        std::ifstream file(settings_path);
        json settings;
        file >> settings;
        file.close();

        if (settings.contains("language"))
        {
            language = settings["language"].get<std::string>();
            return;
        }
    }

    // If the file doesn't exist or "language" entry is missing, detect system locale
    wchar_t locale_name[LOCALE_NAME_MAX_LENGTH];
    if (GetUserDefaultLocaleName(locale_name, LOCALE_NAME_MAX_LENGTH) > 0)
    {
        int size_needed = WideCharToMultiByte(CP_UTF8, 0, locale_name, -1, nullptr, 0, nullptr, nullptr);
        if (size_needed > 0)
        {
            std::string detected_locale(size_needed - 1, 0);
            WideCharToMultiByte(CP_UTF8, 0, locale_name, -1, &detected_locale[0], size_needed, nullptr, nullptr);
            std::cout << "Detected locale: " << detected_locale << std::endl;

            if (detected_locale == "zh-CN" || detected_locale == "zh-Hans-HK" || detected_locale == "zh-Hans-MO" || detected_locale == "zh-SG")
            {
                language = "zh_CN";
            }
            else if (detected_locale == "zh-HK" || detected_locale == "zh-MO" || detected_locale == "zh-TW")
            {
                language = "zh_CN";
            }
            else if (detected_locale == "en-US")
            {
                language = "en_US";
            }
        }
    }

    saveSettings();
}

void change_language(const std::string &lang, Fl_Group *group)
{
    group->hide();

    for (const auto &[child, key] : translatable_widgets)
    {
        auto lang_translations = translations.find(lang);
        if (lang_translations != translations.end())
        {
            auto label_it = lang_translations->second.find(key);
            if (label_it != lang_translations->second.end())
            {
                // Update the label with the translated text
                child->copy_label(label_it->second.c_str());
                child->labelsize(font_size);
                child->labelfont(FL_FREE_FONT);
                child->labelcolor(FL_WHITE);

                Fl_Button *button = dynamic_cast<Fl_Button *>(child);
                if (button)
                {
                    button->labelcolor(FL_BLACK);
                    continue;
                }

                Fl_Flex *parent_flex = dynamic_cast<Fl_Flex *>(child->parent());
                if (parent_flex)
                {
                    parent_flex->fixed(child, fl_width(child->label()));
                }
            }
        }
    }

    group->redraw();
    group->show();
    language = lang;
    saveSettings();
}

void change_language_callback(Fl_Widget *widget, void *data)
{
    ChangeLanguageData *changeLanguageData = static_cast<ChangeLanguageData *>(data);
    std::string lang = changeLanguageData->lang;
    Fl_Group *group = changeLanguageData->group;

    change_language(lang, group);
}

// Assign translation key for translating widget's label
void tr(Fl_Widget *widget, const std::string &key)
{
    translatable_widgets.emplace_back(widget, key);
}

// Directly translate a string using the current language
const char *t(const std::string &key)
{
    auto lang_it = translations.find(language);
    if (lang_it != translations.end())
    {
        auto trans_it = lang_it->second.find(key);
        if (trans_it != lang_it->second.end())
        {
            return trans_it->second.c_str();
        }
    }
    return key.c_str();
}

void uncheck_all_checkbuttons(Fl_Group *group)
{
    if (!group)
        return;

    for (int i = 0; i < group->children(); ++i)
    {
        Fl_Widget *child = group->child(i);
        if (!child)
            continue;

        Fl_Check_Button *check_btn = dynamic_cast<Fl_Check_Button *>(child);
        Fl_Input *input = dynamic_cast<Fl_Input *>(child);
        if (check_btn)
        {
            check_btn->value(0);
        }
        if (input)
        {
            input->readonly(0);
        }

        // If the child is a group, recurse into it
        Fl_Group *subgroup = dynamic_cast<Fl_Group *>(child);
        if (subgroup)
        {
            uncheck_all_checkbuttons(subgroup);
        }
    }
}

void clean_up(Fl_Window *window, Trainer *trainer)
{
    trainer->cleanUp();
    uncheck_all_checkbuttons(window);
}

bool compareNumericStrings(const std::string &a, const std::string &b)
{
    // Handle negative numbers
    if (a[0] == '-' && b[0] != '-')
        return true; // Negative is less than positive
    if (a[0] != '-' && b[0] == '-')
        return false; // Positive is greater than negative
    if (a[0] == '-' && b[0] == '-')
        return compareNumericStrings(b.substr(1), a.substr(1)); // Reverse comparison for negatives

    // Remove leading zeros
    std::string a_trim = a;
    std::string b_trim = b;
    a_trim.erase(0, a_trim.find_first_not_of('0'));
    b_trim.erase(0, b_trim.find_first_not_of('0'));

    // Compare by length
    if (a_trim.size() != b_trim.size())
        return a_trim.size() < b_trim.size();

    // Compare lexicographically if lengths are equal
    return a_trim < b_trim;
}

void set_input_values(Fl_Input *input, std::string def, std::string min, std::string max)
{
    input->value(def.c_str());

    // A callback function to handle input changes
    auto input_callback = [](Fl_Widget *widget, void *data)
    {
        Fl_Input *input = dynamic_cast<Fl_Input *>(widget);
        if (!input)
            return;

        std::string value = input->value();
        auto constraints = static_cast<std::tuple<std::string, std::string, std::string> *>(data);
        std::string def = std::get<0>(*constraints);
        std::string min = std::get<1>(*constraints);
        std::string max = std::get<2>(*constraints);

        if (value.empty())
        {
            input->value(def.c_str());
        }
        else if (compareNumericStrings(value, min))
        {
            input->value(min.c_str());
        }
        else if (compareNumericStrings(max, value))
        {
            input->value(max.c_str());
        }
    };

    // Associate the callback with the input field and pass the min/max data
    auto *constraints = new std::tuple<std::string, std::string, std::string>(def, min, max);
    input->callback(input_callback, (void *)constraints);
}

// Function to periodically check process status and update GUI
void check_process_status(void *data)
{
    TimeoutData *timeoutData = static_cast<TimeoutData *>(data);
    Trainer *trainer = timeoutData->trainer;
    Fl_Box *process_exe = timeoutData->process_exe;
    Fl_Box *process_id = timeoutData->process_id;

    bool running = trainer->isProcessRunning();

    // Retrieve process information
    std::wstring processExeW = trainer->getProcessName();
    std::string processExeStr;
    if (!processExeW.empty())
    {
        int size_needed = WideCharToMultiByte(CP_UTF8, 0, processExeW.c_str(), -1, nullptr, 0, nullptr, nullptr);
        if (size_needed > 0)
        {
            processExeStr.resize(size_needed - 1);
            WideCharToMultiByte(CP_UTF8, 0, processExeW.c_str(), -1, &processExeStr[0], size_needed, nullptr, nullptr);
        }
    }
    DWORD processId = trainer->getProcessId();
    std::string processIdStr = processId ? std::to_string(processId) : "N/A";

    // Set label color based on process status
    process_exe->copy_label(processExeStr.c_str());
    process_exe->labelcolor(running ? FL_DARK_GREEN : FL_RED);
    process_exe->labelsize(font_size);

    process_id->copy_label(processIdStr.c_str());
    process_id->labelcolor(FL_WHITE);
    process_id->labelsize(font_size);

    if (!running)
    {
        clean_up(process_exe->window(), trainer);
    }

    Fl::repeat_timeout(1.0, check_process_status, data);
}

// ===========================================================================
// Info Table and Callbacks
// ===========================================================================

// Displayed by pressing info button
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

void toggle_callback(Fl_Widget *widget, void *data);
void apply_callback(Fl_Widget *widget, void *data);
void info_callback(Fl_Widget *widget, void *data)
{
    InfoCallbackData *info_data = static_cast<InfoCallbackData *>(data);
    if (!info_data || !info_data->trainer || info_data->columnTitles.empty() || !info_data->windowInstance)
        return;

    Trainer *trainer = info_data->trainer;
    Fl_Input *input = info_data->input_ptr ? *info_data->input_ptr : nullptr;
    const std::vector<std::string> &columnTitles = info_data->columnTitles;
    const std::string &windowTitleKey = info_data->windowTitleKey;
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

    list_window = new Fl_Window(list_x, list_y, list_w, list_h, t(windowTitleKey.c_str()));
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

// ===========================================================================
// Widget Placement Helper Functions
// ===========================================================================

Fl_Button *create_info_button(
    Trainer *trainer,
    Fl_Input **input_field_ptr,
    const std::vector<std::string> &info_columns,
    const std::string &info_window_key,
    Fl_Window **info_window_instance,
    std::string (Trainer::*info_data_retriever)(),
    Fl_PNG_Image *info_img)
{
    Fl_Button *info_button = new Fl_Button(0, 0, 0, 0);
    info_button->box(FL_NO_BOX);
    info_button->image(info_img);

    InfoCallbackData *info_data = new InfoCallbackData{trainer, input_field_ptr, info_window_key, info_window_instance, info_columns, info_data_retriever};
    info_button->callback(info_callback, info_data);

    return info_button;
}

Fl_Check_Button *place_toggle_widget(
    Fl_Flex *parent_flex,
    Trainer *trainer,
    const std::string &optionName,
    const std::string &labelKey,
    Fl_Input **out_input = nullptr,
    const char *input_default = nullptr,
    const char *input_min = nullptr,
    const char *input_max = nullptr,
    int input_type = FL_INT_INPUT,
    Fl_Button *info_button = nullptr)
{
    Fl_Flex *toggle_flex = new Fl_Flex(0, 0, 0, 0, Fl_Flex::HORIZONTAL);
    toggle_flex->gap(UI_OPTION_GAP);

    Fl_Box *toggle_spacer = new Fl_Box(0, 0, 0, 0);
    toggle_flex->fixed(toggle_spacer, UI_TOGGLE_SPACER_WIDTH);

    Fl_Check_Button *check_button = new Fl_Check_Button(0, 0, 0, 0);
    toggle_flex->fixed(check_button, UI_TOGGLE_WIDTH);

    Fl_Box *label = new Fl_Box(0, 0, 0, 0);
    tr(label, labelKey);

    Fl_Input *input = nullptr;
    if (input_default && input_min && input_max)
    {
        input = new Fl_Input(0, 0, 0, 0);
        toggle_flex->fixed(input, UI_INPUT_WIDTH);
        input->type(input_type);
        set_input_values(input, input_default, input_min, input_max);
        if (out_input)
            *out_input = input;
    }

    // Optional info button
    if (info_button)
    {
        toggle_flex->fixed(info_button, 20);
        toggle_flex->add(info_button);
    }

    ToggleData *toggle_data = new ToggleData{trainer, optionName, check_button, input};
    check_button->callback(toggle_callback, toggle_data);

    toggle_flex->end();
    parent_flex->fixed(toggle_flex, UI_OPTION_HEIGHT);

    return check_button;
}

Fl_Button *place_apply_widget(
    Fl_Flex *parent_flex,
    Trainer *trainer,
    const std::string &optionName,
    const std::string &labelKey,
    Fl_Input **out_input = nullptr,
    const char *input_default = nullptr,
    const char *input_min = nullptr,
    const char *input_max = nullptr,
    int input_type = FL_INT_INPUT,
    Fl_Button *info_button = nullptr)
{
    Fl_Flex *apply_flex = new Fl_Flex(0, 0, 0, 0, Fl_Flex::HORIZONTAL);
    apply_flex->gap(UI_OPTION_GAP);

    Fl_Button *apply_button = new Fl_Button(0, 0, 0, 0);
    apply_flex->fixed(apply_button, UI_BUTTON_WIDTH);
    tr(apply_button, "Apply");

    Fl_Box *label = new Fl_Box(0, 0, 0, 0);
    tr(label, labelKey);

    Fl_Input *input = nullptr;
    if (input_default && input_min && input_max)
    {
        input = new Fl_Input(0, 0, 0, 0);
        apply_flex->fixed(input, UI_INPUT_WIDTH);
        input->type(input_type);
        set_input_values(input, input_default, input_min, input_max);
        if (out_input)
            *out_input = input;
    }

    // Optional info button
    if (info_button)
    {
        apply_flex->fixed(info_button, 20);
        apply_flex->add(info_button);
    }

    ApplyData *apply_data = new ApplyData{trainer, optionName, apply_button, input};
    apply_button->callback(apply_callback, apply_data);

    apply_flex->end();
    parent_flex->fixed(apply_flex, UI_OPTION_HEIGHT);

    return apply_button;
}

WidgetPair place_indented_toggle_widget(
    Fl_Flex *parent_flex,
    const std::string &labelKey,
    const char *input_default = nullptr,
    const char *input_min = nullptr,
    const char *input_max = nullptr,
    int input_type = FL_INT_INPUT)
{
    Fl_Flex *toggle_flex = new Fl_Flex(0, 0, 0, 0, Fl_Flex::HORIZONTAL);
    toggle_flex->gap(UI_OPTION_GAP);

    Fl_Box *indent_spacer = new Fl_Box(0, 0, 0, 0);
    toggle_flex->fixed(indent_spacer, UI_BUTTON_WIDTH);

    Fl_Box *toggle_spacer = new Fl_Box(0, 0, 0, 0);
    toggle_flex->fixed(toggle_spacer, UI_TOGGLE_SPACER_WIDTH);

    Fl_Check_Button *check_button = new Fl_Check_Button(0, 0, 0, 0);
    toggle_flex->fixed(check_button, UI_TOGGLE_WIDTH);

    Fl_Box *label = new Fl_Box(0, 0, 0, 0);
    tr(label, labelKey);

    Fl_Input *input = nullptr;
    if (input_default && input_min && input_max)
    {
        input = new Fl_Input(0, 0, 0, 0);
        toggle_flex->fixed(input, UI_INPUT_WIDTH);
        input->type(input_type);
        set_input_values(input, input_default, input_min, input_max);
    }

    toggle_flex->end();
    parent_flex->fixed(toggle_flex, UI_OPTION_HEIGHT);

    return WidgetPair{check_button, input};
}

WidgetPair place_indented_apply_widget(
    Fl_Flex *parent_flex,
    const std::string &labelKey,
    const char *input_default = nullptr,
    const char *input_min = nullptr,
    const char *input_max = nullptr,
    int input_type = FL_INT_INPUT)
{
    Fl_Flex *apply_flex = new Fl_Flex(0, 0, 0, 0, Fl_Flex::HORIZONTAL);
    apply_flex->gap(UI_OPTION_GAP);

    Fl_Box *indent_spacer = new Fl_Box(0, 0, 0, 0);
    apply_flex->fixed(indent_spacer, UI_BUTTON_WIDTH);

    Fl_Button *apply_button = new Fl_Button(0, 0, 0, 0);
    apply_flex->fixed(apply_button, UI_BUTTON_WIDTH);
    tr(apply_button, "Apply");

    Fl_Box *label = new Fl_Box(0, 0, 0, 0);
    tr(label, labelKey);

    Fl_Input *input = nullptr;
    if (input_default && input_min && input_max)
    {
        input = new Fl_Input(0, 0, 0, 0);
        apply_flex->fixed(input, UI_INPUT_WIDTH);
        input->type(input_type);
        set_input_values(input, input_default, input_min, input_max);
    }

    apply_flex->end();
    parent_flex->fixed(apply_flex, UI_OPTION_HEIGHT);

    return WidgetPair{apply_button, input};
}

Fl_Input *place_indented_input_widget(
    Fl_Flex *parent_flex,
    const std::string &labelKey,
    const char *input_default,
    const char *input_min,
    const char *input_max,
    int input_type = FL_INT_INPUT)
{
    Fl_Flex *input_flex = new Fl_Flex(0, 0, 0, 0, Fl_Flex::HORIZONTAL);
    input_flex->gap(UI_OPTION_GAP);

    Fl_Box *indent_spacer = new Fl_Box(0, 0, 0, 0);
    input_flex->fixed(indent_spacer, UI_BUTTON_WIDTH);

    Fl_Input *input = new Fl_Input(0, 0, 0, 0);
    input_flex->fixed(input, UI_BUTTON_WIDTH);
    input->type(input_type);
    set_input_values(input, input_default, input_min, input_max);

    Fl_Box *label = new Fl_Box(0, 0, 0, 0);
    tr(label, labelKey);

    input_flex->end();
    parent_flex->fixed(input_flex, UI_OPTION_HEIGHT);

    return input;
}