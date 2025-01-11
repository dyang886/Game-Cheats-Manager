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
  HRSRC hRes = FindResource(nullptr, resource_name, RT_RCDATA);
  if (hRes)
  {
    HGLOBAL hData = LoadResource(nullptr, hRes);
    size = SizeofResource(nullptr, hRes);
    return (const unsigned char *)LockResource(hData);
  }
  return nullptr;
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
  if (optionName == "Health")
  {
    status = trainer->setHealth(std::stof(inputValue));
  }

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
  // Option 1: Set coin (Toggle)
  // ------------------------------------------------------------------
  Fl_Flex *coin_flex = new Fl_Flex(0, 0, 0, 0, Fl_Flex::HORIZONTAL);
  coin_flex->gap(option_gap);

  Fl_Check_Button *coin_check_button = new Fl_Check_Button(0, 0, 0, 0);
  coin_flex->fixed(coin_check_button, button_w);

  Fl_Box *coin_label = new Fl_Box(0, 0, 0, 0, "Set Coin");
  coin_label->labelsize(font_size);
  coin_flex->fixed(coin_label, fl_width(coin_label->label()));

  Fl_Input *coin_input = new Fl_Input(0, 0, 0, 0, "");
  coin_flex->fixed(coin_input, input_w);
  coin_input->type(FL_INT_INPUT);
  set_input_values(coin_input, "9999", "0", "99999999");

  ToggleData *td_coin = new ToggleData{&trainer, "Coin", coin_check_button, coin_input};
  coin_check_button->callback(toggle_callback, td_coin);

  coin_flex->end();
  options1_flex->fixed(coin_flex, option_h);

  // ------------------------------------------------------------------
  // Option 2: Set health (Apply)
  // ------------------------------------------------------------------
  Fl_Flex *health_flex = new Fl_Flex(0, 0, 0, 0, Fl_Flex::HORIZONTAL);
  health_flex->gap(option_gap);

  // Apply Button for Health
  Fl_Button *health_apply_button = new Fl_Button(0, 0, 0, 0, "Apply");
  health_flex->fixed(health_apply_button, button_w);

  // Label for Health
  Fl_Box *health_label = new Fl_Box(0, 0, 0, 0, "Set Health");
  health_label->labelsize(font_size);
  health_flex->fixed(health_label, fl_width(health_label->label()));

  // Input for Health
  Fl_Input *health_input = new Fl_Input(0, 0, 0, 0, "");
  health_flex->fixed(health_input, input_w);
  health_input->type(FL_INT_INPUT);
  set_input_values(health_input, "9999999999", "0", "99999999999999999999999999999999999999");

  // Assign a callback for apply button
  ApplyData *ad_health = new ApplyData{&trainer, "Health", health_apply_button, health_input};
  health_apply_button->callback(apply_callback, ad_health);

  health_flex->end();
  options1_flex->fixed(health_flex, option_h);

  // ------------------------------------------------------------------
  // Option 3: Set horizontal_speed (Toggle)
  // ------------------------------------------------------------------
  Fl_Flex *horizontal_speed_flex = new Fl_Flex(0, 0, 0, 0, Fl_Flex::HORIZONTAL);
  horizontal_speed_flex->gap(option_gap);

  Fl_Check_Button *horizontal_speed_check_button = new Fl_Check_Button(0, 0, 0, 0);
  horizontal_speed_flex->fixed(horizontal_speed_check_button, button_w);

  Fl_Box *horizontal_speed_label = new Fl_Box(0, 0, 0, 0, "Set Horizontal Speed");
  horizontal_speed_label->labelsize(font_size);
  horizontal_speed_flex->fixed(horizontal_speed_label, fl_width(horizontal_speed_label->label()));

  Fl_Input *horizontal_speed_input = new Fl_Input(0, 0, 0, 0, "");
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

  Fl_Box *arrow_damage_label = new Fl_Box(0, 0, 0, 0, "Set Arrow Damage");
  arrow_damage_label->labelsize(font_size);
  arrow_damage_flex->fixed(arrow_damage_label, fl_width(arrow_damage_label->label()));

  Fl_Input *arrow_damage_input = new Fl_Input(0, 0, 0, 0, "");
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

  Fl_Box *arrow_frequency_label = new Fl_Box(0, 0, 0, 0, "Set Arrow Frequency");
  arrow_frequency_label->labelsize(font_size);
  arrow_frequency_flex->fixed(arrow_frequency_label, fl_width(arrow_frequency_label->label()));

  Fl_Input *arrow_frequency_input = new Fl_Input(0, 0, 0, 0, "");
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

  Fl_Box *arrow_speed_label = new Fl_Box(0, 0, 0, 0, "Set Arrow Speed");
  arrow_speed_label->labelsize(font_size);
  arrow_speed_flex->fixed(arrow_speed_label, fl_width(arrow_speed_label->label()));

  Fl_Input *arrow_speed_input = new Fl_Input(0, 0, 0, 0, "");
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

  Fl_Box *arrow_distance_label = new Fl_Box(0, 0, 0, 0, "Set Arrow Distance");
  arrow_distance_label->labelsize(font_size);
  arrow_distance_flex->fixed(arrow_distance_label, fl_width(arrow_distance_label->label()));

  Fl_Input *arrow_distance_input = new Fl_Input(0, 0, 0, 0, "");
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

  Fl_Box *arrow_count_label = new Fl_Box(0, 0, 0, 0, "Set Arrow Count");
  arrow_count_label->labelsize(font_size);
  arrow_count_flex->fixed(arrow_count_label, fl_width(arrow_count_label->label()));

  Fl_Input *arrow_count_input = new Fl_Input(0, 0, 0, 0, "");
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

  Fl_Box *sword_damage_label = new Fl_Box(0, 0, 0, 0, "Set Sword Damage");
  sword_damage_label->labelsize(font_size);
  sword_damage_flex->fixed(sword_damage_label, fl_width(sword_damage_label->label()));

  Fl_Input *sword_damage_input = new Fl_Input(0, 0, 0, 0, "");
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

  Fl_Box *sword_cool_down_label = new Fl_Box(0, 0, 0, 0, "Set Sword Cooldown");
  sword_cool_down_label->labelsize(font_size);
  sword_cool_down_flex->fixed(sword_cool_down_label, fl_width(sword_cool_down_label->label()));

  Fl_Input *sword_cool_down_input = new Fl_Input(0, 0, 0, 0, "");
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

  Fl_Box *sword_speed_label = new Fl_Box(0, 0, 0, 0, "Set Sword Speed");
  sword_speed_label->labelsize(font_size);
  sword_speed_flex->fixed(sword_speed_label, fl_width(sword_speed_label->label()));

  Fl_Input *sword_speed_input = new Fl_Input(0, 0, 0, 0, "");
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

  Fl_Box *sword_distance_label = new Fl_Box(0, 0, 0, 0, "Set Sword Distance");
  sword_distance_label->labelsize(font_size);
  sword_distance_flex->fixed(sword_distance_label, fl_width(sword_distance_label->label()));

  Fl_Input *sword_distance_input = new Fl_Input(0, 0, 0, 0, "");
  sword_distance_flex->fixed(sword_distance_input, input_w);
  sword_distance_input->type(FL_INT_INPUT);
  set_input_values(sword_distance_input, "99", "1", "999");

  ToggleData *td_sword_distance = new ToggleData{&trainer, "SwordDistance", sword_distance_check_button, sword_distance_input};
  sword_distance_check_button->callback(toggle_callback, td_sword_distance);

  sword_distance_flex->end();
  options1_flex->fixed(sword_distance_flex, option_h);

  options1_flex->end();

  // =========================
  // Finalize and Show Window
  // =========================
  window->end();
  window->show(argc, argv);
  int ret = Fl::run();

  return ret;
}
