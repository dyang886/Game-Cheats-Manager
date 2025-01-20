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
  // load_translations("D:/Resources/Software/Game Trainers/Plants vs. Zombies/translations.json");

  // Create the main window
  Fl_Window *window = new Fl_Window(800, 600);
  window->user_data("Sackboy Trainer");

  int left_margin = 20;
  int button_w = 50;
  int input_w = 200;
  int option_gap = 10;
  int option_h = static_cast<int>(font_size * 1.5);

  // Setup fonts
  Fl::set_font(FL_FREE_FONT, "Noto Sans SC");
  fl_font(FL_FREE_FONT, font_size);

  // ------------------------------------------------------------------
  // Top Row: Language Selection
  // ------------------------------------------------------------------
  int lang_flex_height = 30;
  int lang_flex_width = 200;

  Fl_Flex lang_flex(window->w() - lang_flex_width, 0, lang_flex_width, lang_flex_height, Fl_Flex::HORIZONTAL);
  lang_flex.color(FL_BLACK);
  Fl_Radio_Round_Button *lang_en = new Fl_Radio_Round_Button(0, 0, 0, 0, "English");
  lang_en->labelfont(FL_FREE_FONT);
  lang_en->labelsize(font_size);
  ChangeLanguageData *changeLanguageDataEN = new ChangeLanguageData{"en_US", window};
  lang_en->callback(change_language_callback, changeLanguageDataEN);

  Fl_Radio_Round_Button *lang_zh = new Fl_Radio_Round_Button(0, 0, 0, 0, "简体中文");
  lang_zh->labelfont(FL_FREE_FONT);
  lang_zh->labelsize(font_size);
  ChangeLanguageData *changeLanguageDataSC = new ChangeLanguageData{"zh_CN", window};
  lang_zh->callback(change_language_callback, changeLanguageDataSC);

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
