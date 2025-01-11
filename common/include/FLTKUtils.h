#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Radio_Round_Button.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Value_Input.H>
#include <FL/Fl_JPEG_Image.H>
#include <FL/fl_draw.H>
#include <FL/Fl_Grid.H>
#include <FL/Fl_Flex.H>
#include <FL/forms.H>
#include <iostream>
#include <locale>
#include <string>
#include <nlohmann/json.hpp>
#include <fstream>
#include <string>
#include <unordered_map>
#include "trainer.h"

using json = nlohmann::json;

int font_size = 15;
std::unordered_map<std::string, std::unordered_map<std::string, std::string>> translations;
std::string lang = "en_US";

void load_translations(const std::string &filename)
{
    std::ifstream file(filename);
    json j;
    file >> j;

    for (auto &lang : j.items())
    {
        std::string lang_code = lang.key();
        for (auto &text : lang.value().items())
        {
            translations[lang_code][text.key()] = text.value();
        }
    }
}

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
    HRSRC hRes = FindResource(nullptr, resource_name, RT_RCDATA);
    if (hRes)
    {
        HGLOBAL hData = LoadResource(nullptr, hRes);
        size = SizeofResource(nullptr, hRes);
        return (const unsigned char *)LockResource(hData);
    }
    return nullptr;
}

void change_language(Fl_Group *group, const std::string &lang)
{
    group->hide();
    if (group->user_data())
    {
        const char *translation_id = static_cast<const char *>(group->user_data());

        // Find the translation for the window title
        auto lang_translations = translations.find(lang);
        if (lang_translations != translations.end())
        {
            auto label_it = lang_translations->second.find(translation_id);
            if (label_it != lang_translations->second.end())
            {
                group->copy_label(label_it->second.c_str());
            }
        }
    }

    for (int i = 0; i < group->children(); ++i)
    {
        Fl_Widget *child = group->child(i);
        if (!child)
            continue;

        if (child->user_data())
        {
            const char *translation_id = static_cast<const char *>(child->user_data());

            // Find the translation for the current label
            auto lang_translations = translations.find(lang);
            if (lang_translations != translations.end())
            {
                auto label_it = lang_translations->second.find(translation_id);
                if (label_it != lang_translations->second.end())
                {
                    // Update the label with the translated text
                    child->copy_label(label_it->second.c_str());
                    child->labelsize(font_size);
                    child->labelfont(FL_FREE_FONT);
                    Fl_Flex *parent_flex = dynamic_cast<Fl_Flex *>(child->parent());
                    if (parent_flex)
                    {
                        parent_flex->fixed(child, fl_width(child->label()));
                    }
                }
            }
        }

        // If the child is a group, recurse into it
        Fl_Group *subgroup = dynamic_cast<Fl_Group *>(child);
        if (subgroup)
        {
            change_language(subgroup, lang);
        }
    }

    group->redraw();
    group->show();
}

void change_language_callback(Fl_Widget *widget, void *data)
{
    ChangeLanguageData *changeLanguageData = static_cast<ChangeLanguageData *>(data);
    std::string lang = changeLanguageData->lang;
    Fl_Group *group = changeLanguageData->group;

    change_language(group, lang);
}

const char *t(const std::string &message, const std::string &lang)
{
    auto lang_translations = translations.find(lang);
    if (lang_translations != translations.end())
    {
        auto message_it = lang_translations->second.find(message);
        if (message_it != lang_translations->second.end())
        {
            return message_it->second.c_str();
        }
    }
    return message.c_str();
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

        std::string min = ((std::pair<std::string, std::string> *)data)->first;
        std::string max = ((std::pair<std::string, std::string> *)data)->second;

        if (compareNumericStrings(value, min))
        {
            input->value(min.c_str());
        }
        else if (compareNumericStrings(max, value))
        {
            input->value(max.c_str());
        }
    };

    // Associate the callback with the input field and pass the min/max data
    std::pair<std::string, std::string> *constraints = new std::pair<std::string, std::string>(min, max);
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
    std::string processExeStr(processExeW.begin(), processExeW.end());
    DWORD processId = trainer->getProcessId();
    std::string processIdStr = "Process ID:" + std::string(" ") + (processId ? std::to_string(processId) : "N/A");

    // Set label color based on process status
    process_exe->copy_label(processExeStr.c_str());
    process_exe->labelcolor(running ? FL_DARK_GREEN : FL_RED);
    process_exe->labelsize(font_size);
    process_id->copy_label(processIdStr.c_str());
    process_id->labelsize(font_size);

    if (!running)
    {
        clean_up(process_exe->window(), trainer);
    }

    // Schedule the next check after 1 second
    Fl::repeat_timeout(1.0, check_process_status, data);
}
