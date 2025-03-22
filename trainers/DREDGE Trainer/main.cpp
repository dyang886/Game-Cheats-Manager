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
  if (optionName == "SpawnItem")
  {
    status = trainer->spawnItem(std::stoi(inputValue));
  }

  // Finalize
  if (!status)
  {
    fl_alert(t("Failed to activate.", language));
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
  window->user_data("DREDGE Trainer");

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
  // Option 1: Spawn item (Apply)
  // ------------------------------------------------------------------
  Fl_Flex *spawn_item_flex = new Fl_Flex(0, 0, 0, 0, Fl_Flex::HORIZONTAL);
  spawn_item_flex->gap(option_gap);

  Fl_Button *spawn_item_apply_button = new Fl_Button(0, 0, 0, 0);
  spawn_item_flex->fixed(spawn_item_apply_button, button_w);

  Fl_Box *spawn_item_label = new Fl_Box(0, 0, 0, 0);
  spawn_item_label->user_data("Spawn Item");

  Fl_Input *spawn_item_input = new Fl_Input(0, 0, 0, 0);
  spawn_item_flex->fixed(spawn_item_input, input_w);
  spawn_item_input->type(FL_INT_INPUT);
  set_input_values(spawn_item_input, "35", "0", "9999999");

  ApplyData *ad_spawn_item = new ApplyData{&trainer, "SpawnItem", spawn_item_apply_button, spawn_item_input};
  spawn_item_apply_button->callback(apply_callback, ad_spawn_item);

  spawn_item_flex->end();
  options1_flex->fixed(spawn_item_flex, option_h);

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
