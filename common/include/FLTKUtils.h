#include <nlohmann/json.hpp>
#include <fstream>
#include <shlobj.h>

using json = nlohmann::json;

// Individual option template:

// // ------------------------------------------------------------------
// // {{}} (Toggle)
// // ------------------------------------------------------------------
// Fl_Flex *{{}}_flex = new Fl_Flex(0, 0, 0, 0, Fl_Flex::HORIZONTAL);
// {{}}_flex->gap(option_gap);

// Fl_Check_Button *{{}}_check_button = new Fl_Check_Button(0, 0, 0, 0);
// {{}}_flex->fixed({{}}_check_button, button_w);

// Fl_Box *{{}}_label = new Fl_Box(0, 0, 0, 0);
// tr({{}}_label, "Label");

// Fl_Input *{{}}_input = new Fl_Input(0, 0, 0, 0);
// {{}}_flex->fixed({{}}_input, input_w);
// {{}}_input->type(FL_INT_INPUT);
// set_input_values({{}}_input, "default", "minimum", "maximum");

// ToggleData *data_{{}} = new ToggleData{&trainer, "OptionName", {{}}_check_button, {{}}_input};
// {{}}_check_button->callback(toggle_callback, data_{{}});

// {{}}_flex->end();
// options1_flex->fixed({{}}_flex, option_h);

// // ------------------------------------------------------------------
// // {{}} (Apply)
// // ------------------------------------------------------------------
// Fl_Flex *{{}}_flex = new Fl_Flex(0, 0, 0, 0, Fl_Flex::HORIZONTAL);
// {{}}_flex->gap(option_gap);

// Fl_Button *{{}}_apply_button = new Fl_Button(0, 0, 0, 0);
// {{}}_flex->fixed({{}}_apply_button, button_w);
// tr({{}}_apply_button, "Apply");

// Fl_Box *{{}}_label = new Fl_Box(0, 0, 0, 0);
// tr({{}}_label, "Label");

// Fl_Input *{{}}_input = new Fl_Input(0, 0, 0, 0);
// {{}}_flex->fixed({{}}_input, input_w);
// {{}}_input->type(FL_INT_INPUT);
// set_input_values({{}}_input, "default", "minimum", "maximum");

// ApplyData *data_{{}} = new ApplyData{&trainer, "OptionName", {{}}_apply_button, {{}}_input};
// {{}}_apply_button->callback(apply_callback, data_{{}});

// {{}}_flex->end();
// options1_flex->fixed({{}}_flex, option_h);

int font_size = 15;
static HANDLE font_handle = nullptr;
std::unordered_map<std::string, std::unordered_map<std::string, std::string>> translations;
std::vector<std::pair<Fl_Widget *, std::string>> translatable_widgets;
std::string language = "en_US";
std::string settings_path = "";

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
