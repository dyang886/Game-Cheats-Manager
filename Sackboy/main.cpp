// main.cpp
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
#include "trainer.h"

struct TimeoutData
{
  Trainer *trainer;
  Fl_Box *process_exe;
  Fl_Box *process_id;
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
  HRSRC resource_handle = FindResourceA(NULL, resource_name, RT_RCDATA);
  if (!resource_handle)
    return nullptr;

  HGLOBAL resource_data = LoadResource(NULL, resource_handle);
  if (!resource_data)
    return nullptr;

  size = SizeofResource(NULL, resource_handle);
  return reinterpret_cast<const unsigned char *>(LockResource(resource_data));
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
  std::string processIdStr = "Process ID: " + (processId ? std::to_string(processId) : "N/A");

  // Set label color based on process status
  process_exe->copy_label(processExeStr.c_str());
  process_exe->labelcolor(running ? FL_DARK_GREEN : FL_RED);
  process_id->copy_label(processIdStr.c_str());

  if (!running)
  {
    clean_up(process_exe->window(), trainer);
  }

  // Schedule the next check after 1 second
  Fl::repeat_timeout(1.0, check_process_status, data);
}

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
    fl_alert("Please run the game first.");
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
    fl_alert("Failed to activate.");
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
    fl_alert("Please run the game first.");
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
  if (optionName == "Bell")
  {
    if (button->value())
    {
      status = trainer->setBell(std::stoi(inputValue));
    }
    else
    {
      status = trainer->disableNamedHook(optionName);
    }
  }

  // Finalize
  if (!status)
  {
    fl_alert("Failed to activate/deactivate.");
    button->value(0);
  }
  button->value() ? input->readonly(1) : input->readonly(0);
}

// Individual option template:

// // ------------------------------------------------------------------
// // Option x: Set {{}} (Apply/Toggle)
// // ------------------------------------------------------------------
// Fl_Flex *{{}}_flex = new Fl_Flex(0, 0, 0, 0, Fl_Flex::HORIZONTAL);
// {{}}_flex->gap(option_gap);

// Fl_Check_Button *{{}}_check_button = new Fl_Check_Button(0, 0, 0, 0);
// {{}}_flex->fixed({{}}_check_button, button_w);

// Fl_Box *{{}}_label = new Fl_Box(0, 0, 0, 0, "Label");
// {{}}_label->labelsize(font_size);
// {{}}_flex->fixed({{}}_label, fl_width({{}}_label->label()));

// Fl_Input *{{}}_input = new Fl_Input(0, 0, 0, 0, "");
// {{}}_flex->fixed({{}}_input, input_w);
// {{}}_input->type(FL_INT_INPUT);
// set_input_values({{}}_input, "default", "minimum", "maximum");

// ToggleData *td_{{}} = new ToggleData{&trainer, "OptionName", {{}}_check_button, {{}}_input};
// {{}}_check_button->callback(toggle_callback, td_{{}});

// {{}}_flex->end();
// options1_flex->fixed({{}}_flex, option_h);

// ===========================================================================
// Main Function
// ===========================================================================
int main(int argc, char **argv)
{
  Trainer trainer;

  // Create the main window
  Fl_Window *window = new Fl_Window(800, 600, "Trainer");

  int font_size = 15;
  int left_margin = 20;
  int button_w = 50;
  int input_w = 200;
  int option_gap = 10;
  int option_h = static_cast<int>(font_size * 1.5);

  fl_font(FL_HELVETICA, font_size);

  // ------------------------------------------------------------------
  // Top Row: Language Selection
  // ------------------------------------------------------------------
  int lang_flex_height = 30;

  Fl_Flex lang_flex(window->w() * 0.8, 0, window->w() * 0.2, lang_flex_height, Fl_Flex::HORIZONTAL);
  lang_flex.align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
  lang_flex.color(FL_BLACK);
  Fl_Radio_Round_Button *lang_en = new Fl_Radio_Round_Button(0, 0, 0, 0, "English");
  Fl_Radio_Round_Button *lang_zh = new Fl_Radio_Round_Button(0, 0, 0, 0, "简体中文");
  lang_flex.end();

  // ------------------------------------------------------------------
  // Left Column: Image and Process Info
  // ------------------------------------------------------------------
  std::pair<int, int> imageSize = std::make_pair(200, 300);

  DWORD img_size = 0;
  const unsigned char *data = load_resource("LOGO_IMG", img_size);
  Fl_JPEG_Image *game_img = new Fl_JPEG_Image(nullptr, data, (int)img_size);
  game_img->scale(imageSize.first, imageSize.second, 1, 0);
  Fl_Box *img_box = new Fl_Box(20, lang_flex_height, imageSize.first, imageSize.second);
  img_box->image(game_img);

  // Process Information
  Fl_Box *process_name = new Fl_Box(left_margin, lang_flex_height + imageSize.second + 10, imageSize.first, font_size, "Process Name:");
  process_name->align(FL_ALIGN_TOP_LEFT | FL_ALIGN_INSIDE);
  process_name->labelsize(font_size);

  Fl_Box *process_exe = new Fl_Box(left_margin, lang_flex_height + imageSize.second + font_size + 15, imageSize.first, font_size);
  process_exe->align(FL_ALIGN_TOP_LEFT | FL_ALIGN_INSIDE);
  process_exe->labelsize(font_size);

  Fl_Box *process_id = new Fl_Box(left_margin, lang_flex_height + imageSize.second + font_size + 45, imageSize.first, font_size);
  process_id->align(FL_ALIGN_TOP_LEFT | FL_ALIGN_INSIDE);
  process_id->labelsize(font_size);

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

  // ------------------------------------------------------------------
  // Option 1: Set bell (Toggle)
  // ------------------------------------------------------------------
  Fl_Flex *bell_flex = new Fl_Flex(0, 0, 0, 0, Fl_Flex::HORIZONTAL);
  bell_flex->gap(option_gap);

  Fl_Check_Button *bell_check_button = new Fl_Check_Button(0, 0, 0, 0);
  bell_flex->fixed(bell_check_button, button_w);

  Fl_Box *bell_label = new Fl_Box(0, 0, 0, 0, "Bell");
  bell_label->labelsize(font_size);
  bell_flex->fixed(bell_label, fl_width(bell_label->label()));

  Fl_Input *bell_input = new Fl_Input(0, 0, 0, 0, "");
  bell_flex->fixed(bell_input, input_w);
  bell_input->type(FL_INT_INPUT);
  set_input_values(bell_input, "9999", "0", "99999999");

  ToggleData *td_bell = new ToggleData{&trainer, "Bell", bell_check_button, bell_input};
  bell_check_button->callback(toggle_callback, td_bell);

  bell_flex->end();
  options1_flex->fixed(bell_flex, option_h);

  options1_flex->end();

  // =========================
  // Finalize and Show Window
  // =========================
  window->end();
  window->show(argc, argv);
  int ret = Fl::run();

  return ret;
}
