// main.cpp
#include <FL/Fl.H>
#include <FL/Fl_Radio_Round_Button.H>
#include <FL/Fl_JPEG_Image.H>
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
    fl_alert(t("Please run the game first.", language));
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
    fl_alert(t("Failed to activate.", language));
  }
  if (input)
  {
    button->value() ? input->readonly(1) : input->readonly(0);
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

  if (!toggleData->trainer->isProcessRunning())
  {
    fl_alert(t("Please run the game first.", language));
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
  if (optionName == "Life")
  {
    if (button->value())
    {
      status = trainer->setLife(std::stoi(inputValue));
    }
    else
    {
      status = trainer->disableNamedPointerToggle(optionName);
    }
  }
  else if (optionName == "Shield")
  {
    if (button->value())
    {
      status = trainer->setShield(std::stof(inputValue));
    }
    else
    {
      status = trainer->disableNamedPointerToggle(optionName);
    }
  }
  else if (optionName == "Nuke")
  {
    if (button->value())
    {
      status = trainer->setNuke(std::stof(inputValue));
    }
    else
    {
      status = trainer->disableNamedPointerToggle(optionName);
    }
  }
  else if (optionName == "DefenseOrbs")
  {
    if (button->value())
    {
      status = trainer->setDefenseOrbs(std::stof(inputValue));
    }
    else
    {
      status = trainer->disableNamedPointerToggle(optionName);
    }
  }
  else if (optionName == "HomingMissile")
  {
    if (button->value())
    {
      status = trainer->setHomingMissile(std::stof(inputValue));
    }
    else
    {
      status = trainer->disableNamedPointerToggle(optionName);
    }
  }
  else if (optionName == "Laser")
  {
    if (button->value())
    {
      status = trainer->setLaser(std::stof(inputValue));
    }
    else
    {
      status = trainer->disableNamedPointerToggle(optionName);
    }
  }
  else if (optionName == "Rockets")
  {
    if (button->value())
    {
      status = trainer->setRockets(std::stof(inputValue));
    }
    else
    {
      status = trainer->disableNamedPointerToggle(optionName);
    }
  }
  else if (optionName == "FlakCannon")
  {
    if (button->value())
    {
      status = trainer->setFlakCannon(std::stof(inputValue));
    }
    else
    {
      status = trainer->disableNamedPointerToggle(optionName);
    }
  }
  else if (optionName == "ThunderStrike")
  {
    if (button->value())
    {
      status = trainer->setThunderStrike(std::stof(inputValue));
    }
    else
    {
      status = trainer->disableNamedPointerToggle(optionName);
    }
  }

  // Finalize
  if (!status)
  {
    fl_alert(t("Failed to activate/deactivate.", language));
    button->value(0);
  }
  button->value() ? input->readonly(1) : input->readonly(0);
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
  Fl::scheme("gtk+");
  Fl_Window *window = new Fl_Window(800, 600);
  Fl::set_color(FL_FREE_COLOR, 0x1c1c1c00);
  window->color(FL_FREE_COLOR);
  window->icon((char *)LoadIcon(GetModuleHandle(NULL), "APP_ICON"));
  window->user_data("Trainer Name");

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
  Fl_JPEG_Image *game_img = new Fl_JPEG_Image(nullptr, data, (int)img_size);
  game_img->scale(imageSize.first, imageSize.second, 1, 0);
  Fl_Box *img_box = new Fl_Box(20, lang_flex_height, imageSize.first, imageSize.second);
  img_box->image(game_img);

  // Process Information
  Fl_Box *process_name = new Fl_Box(left_margin, lang_flex_height + imageSize.second + 10, imageSize.first, font_size);
  process_name->user_data("Process Name:");
  process_name->align(FL_ALIGN_TOP_LEFT | FL_ALIGN_INSIDE);

  Fl_Box *process_exe = new Fl_Box(left_margin, lang_flex_height + imageSize.second + font_size + 20, imageSize.first, font_size);
  process_exe->align(FL_ALIGN_TOP_LEFT | FL_ALIGN_INSIDE);

  Fl_Flex *process_id_flex = new Fl_Flex(left_margin, lang_flex_height + imageSize.second + font_size + 55, imageSize.first, font_size, Fl_Flex::HORIZONTAL);
  process_id_flex->gap(5);

  Fl_Box *process_id_label = new Fl_Box(0, 0, 0, 0);
  process_id_label->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
  process_id_label->user_data("Process ID:");

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
  // Option 1: Set life (Toggle)
  // ------------------------------------------------------------------
  Fl_Flex *life_flex = new Fl_Flex(0, 0, 0, 0, Fl_Flex::HORIZONTAL);
  life_flex->gap(option_gap);

  Fl_Check_Button *life_check_button = new Fl_Check_Button(0, 0, 0, 0);
  life_flex->fixed(life_check_button, button_w);

  Fl_Box *life_label = new Fl_Box(0, 0, 0, 0);
  life_label->user_data("Set Life Count");

  Fl_Input *life_input = new Fl_Input(0, 0, 0, 0);
  life_flex->fixed(life_input, input_w);
  life_input->type(FL_INT_INPUT);
  set_input_values(life_input, "2", "0", "2");

  ToggleData *td_life = new ToggleData{&trainer, "Life", life_check_button, life_input};
  life_check_button->callback(toggle_callback, td_life);

  life_flex->end();
  options1_flex->fixed(life_flex, option_h);

  // ------------------------------------------------------------------
  // Option 2: Set shield (Toggle)
  // ------------------------------------------------------------------
  Fl_Flex *shield_flex = new Fl_Flex(0, 0, 0, 0, Fl_Flex::HORIZONTAL);
  shield_flex->gap(option_gap);

  Fl_Check_Button *shield_check_button = new Fl_Check_Button(0, 0, 0, 0);
  shield_flex->fixed(shield_check_button, button_w);

  Fl_Box *shield_label = new Fl_Box(0, 0, 0, 0);
  shield_label->user_data("Set Shield Level");

  Fl_Input *shield_input = new Fl_Input(0, 0, 0, 0);
  shield_flex->fixed(shield_input, input_w);
  shield_input->type(FL_INT_INPUT);
  set_input_values(shield_input, "4", "0", "4");

  ToggleData *td_shield = new ToggleData{&trainer, "Shield", shield_check_button, shield_input};
  shield_check_button->callback(toggle_callback, td_shield);

  shield_flex->end();
  options1_flex->fixed(shield_flex, option_h);

  // ------------------------------------------------------------------
  // Option 3: Set nuke (Toggle)
  // ------------------------------------------------------------------
  Fl_Flex *nuke_flex = new Fl_Flex(0, 0, 0, 0, Fl_Flex::HORIZONTAL);
  nuke_flex->gap(option_gap);

  Fl_Check_Button *nuke_check_button = new Fl_Check_Button(0, 0, 0, 0);
  nuke_flex->fixed(nuke_check_button, button_w);

  Fl_Box *nuke_label = new Fl_Box(0, 0, 0, 0);
  nuke_label->user_data("Set Nuke Amount");

  Fl_Input *nuke_input = new Fl_Input(0, 0, 0, 0);
  nuke_flex->fixed(nuke_input, input_w);
  nuke_input->type(FL_INT_INPUT);
  set_input_values(nuke_input, "3", "0", "3");

  ToggleData *td_nuke = new ToggleData{&trainer, "Nuke", nuke_check_button, nuke_input};
  nuke_check_button->callback(toggle_callback, td_nuke);

  nuke_flex->end();
  options1_flex->fixed(nuke_flex, option_h);

  // ------------------------------------------------------------------
  // Option 4: Set defense_orbs (Toggle)
  // ------------------------------------------------------------------
  Fl_Flex *defense_orbs_flex = new Fl_Flex(0, 0, 0, 0, Fl_Flex::HORIZONTAL);
  defense_orbs_flex->gap(option_gap);

  Fl_Check_Button *defense_orbs_check_button = new Fl_Check_Button(0, 0, 0, 0);
  defense_orbs_flex->fixed(defense_orbs_check_button, button_w);

  Fl_Box *defense_orbs_label = new Fl_Box(0, 0, 0, 0);
  defense_orbs_label->user_data("Set Defense Orbs Level");

  Fl_Input *defense_orbs_input = new Fl_Input(0, 0, 0, 0);
  defense_orbs_flex->fixed(defense_orbs_input, input_w);
  defense_orbs_input->type(FL_INT_INPUT);
  set_input_values(defense_orbs_input, "3", "0", "3");

  ToggleData *td_defense_orbs = new ToggleData{&trainer, "DefenseOrbs", defense_orbs_check_button, defense_orbs_input};
  defense_orbs_check_button->callback(toggle_callback, td_defense_orbs);

  defense_orbs_flex->end();
  options1_flex->fixed(defense_orbs_flex, option_h);

  // ------------------------------------------------------------------
  // Option 5: Set homing_missile (Toggle)
  // ------------------------------------------------------------------
  Fl_Flex *homing_missile_flex = new Fl_Flex(0, 0, 0, 0, Fl_Flex::HORIZONTAL);
  homing_missile_flex->gap(option_gap);

  Fl_Check_Button *homing_missile_check_button = new Fl_Check_Button(0, 0, 0, 0);
  homing_missile_flex->fixed(homing_missile_check_button, button_w);

  Fl_Box *homing_missile_label = new Fl_Box(0, 0, 0, 0);
  homing_missile_label->user_data("Set Homing Missile Level");

  Fl_Input *homing_missile_input = new Fl_Input(0, 0, 0, 0);
  homing_missile_flex->fixed(homing_missile_input, input_w);
  homing_missile_input->type(FL_INT_INPUT);
  set_input_values(homing_missile_input, "3", "0", "3");

  ToggleData *td_homing_missile = new ToggleData{&trainer, "HomingMissile", homing_missile_check_button, homing_missile_input};
  homing_missile_check_button->callback(toggle_callback, td_homing_missile);

  homing_missile_flex->end();
  options1_flex->fixed(homing_missile_flex, option_h);

  // ------------------------------------------------------------------
  // Option 6: Set laser (Toggle)
  // ------------------------------------------------------------------
  Fl_Flex *laser_flex = new Fl_Flex(0, 0, 0, 0, Fl_Flex::HORIZONTAL);
  laser_flex->gap(option_gap);

  Fl_Check_Button *laser_check_button = new Fl_Check_Button(0, 0, 0, 0);
  laser_flex->fixed(laser_check_button, button_w);

  Fl_Box *laser_label = new Fl_Box(0, 0, 0, 0);
  laser_label->user_data("Set Laser Level");

  Fl_Input *laser_input = new Fl_Input(0, 0, 0, 0);
  laser_flex->fixed(laser_input, input_w);
  laser_input->type(FL_INT_INPUT);
  set_input_values(laser_input, "3", "0", "3");

  ToggleData *td_laser = new ToggleData{&trainer, "Laser", laser_check_button, laser_input};
  laser_check_button->callback(toggle_callback, td_laser);

  laser_flex->end();
  options1_flex->fixed(laser_flex, option_h);

  // ------------------------------------------------------------------
  // Option 7: Set rockets (Toggle)
  // ------------------------------------------------------------------
  Fl_Flex *rockets_flex = new Fl_Flex(0, 0, 0, 0, Fl_Flex::HORIZONTAL);
  rockets_flex->gap(option_gap);

  Fl_Check_Button *rockets_check_button = new Fl_Check_Button(0, 0, 0, 0);
  rockets_flex->fixed(rockets_check_button, button_w);

  Fl_Box *rockets_label = new Fl_Box(0, 0, 0, 0);
  rockets_label->user_data("Set Rockets Level");

  Fl_Input *rockets_input = new Fl_Input(0, 0, 0, 0);
  rockets_flex->fixed(rockets_input, input_w);
  rockets_input->type(FL_INT_INPUT);
  set_input_values(rockets_input, "3", "0", "3");

  ToggleData *td_rockets = new ToggleData{&trainer, "Rockets", rockets_check_button, rockets_input};
  rockets_check_button->callback(toggle_callback, td_rockets);

  rockets_flex->end();
  options1_flex->fixed(rockets_flex, option_h);

  // ------------------------------------------------------------------
  // Option 8: Set flak_cannon (Toggle)
  // ------------------------------------------------------------------
  Fl_Flex *flak_cannon_flex = new Fl_Flex(0, 0, 0, 0, Fl_Flex::HORIZONTAL);
  flak_cannon_flex->gap(option_gap);

  Fl_Check_Button *flak_cannon_check_button = new Fl_Check_Button(0, 0, 0, 0);
  flak_cannon_flex->fixed(flak_cannon_check_button, button_w);

  Fl_Box *flak_cannon_label = new Fl_Box(0, 0, 0, 0);
  flak_cannon_label->user_data("Set Flak Cannon Level");

  Fl_Input *flak_cannon_input = new Fl_Input(0, 0, 0, 0);
  flak_cannon_flex->fixed(flak_cannon_input, input_w);
  flak_cannon_input->type(FL_INT_INPUT);
  set_input_values(flak_cannon_input, "3", "0", "3");

  ToggleData *td_flak_cannon = new ToggleData{&trainer, "FlakCannon", flak_cannon_check_button, flak_cannon_input};
  flak_cannon_check_button->callback(toggle_callback, td_flak_cannon);

  flak_cannon_flex->end();
  options1_flex->fixed(flak_cannon_flex, option_h);

  // ------------------------------------------------------------------
  // Option 9: Set thunder_strike (Toggle)
  // ------------------------------------------------------------------
  Fl_Flex *thunder_strike_flex = new Fl_Flex(0, 0, 0, 0, Fl_Flex::HORIZONTAL);
  thunder_strike_flex->gap(option_gap);

  Fl_Check_Button *thunder_strike_check_button = new Fl_Check_Button(0, 0, 0, 0);
  thunder_strike_flex->fixed(thunder_strike_check_button, button_w);

  Fl_Box *thunder_strike_label = new Fl_Box(0, 0, 0, 0);
  thunder_strike_label->user_data("Set Thunder Strike Level");

  Fl_Input *thunder_strike_input = new Fl_Input(0, 0, 0, 0);
  thunder_strike_flex->fixed(thunder_strike_input, input_w);
  thunder_strike_input->type(FL_INT_INPUT);
  set_input_values(thunder_strike_input, "3", "0", "3");

  ToggleData *td_thunder_strike = new ToggleData{&trainer, "ThunderStrike", thunder_strike_check_button, thunder_strike_input};
  thunder_strike_check_button->callback(toggle_callback, td_thunder_strike);

  thunder_strike_flex->end();
  options1_flex->fixed(thunder_strike_flex, option_h);

  Fl_Box *spacerBottom = new Fl_Box(0, 0, 0, 0);
  options1_flex->end();
  change_language(window, language);

  // =========================
  // Finalize and Show Window
  // =========================
  window->end();
  window->show(argc, argv);
  int ret = Fl::run();

  return ret;
}
