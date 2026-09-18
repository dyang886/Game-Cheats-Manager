// TrainerApp.h
// Owns the window, the language switch, the process status readout and the two dispatch callbacks
// that FLTKUtils.h forward-declares. A trainer supplies one function; everything else about it is
// declared in its CMakeLists.txt (see cmake/AddTrainer.cmake) and arrives as compile definitions.
//
//   #include "TrainerApp.h"
//
//   void register_cheats(TrainerUI &ui)
//   {
//       ui.toggle(
//           "God Mode",
//           [](Trainer &t, bool enable, const Value &, std::string &message)
//           { return t.toggleGodMode(enable, message); }
//       );
//
//       ui.toggle(
//           "Game Speed Multiplier", FloatInput("1", "0.01", "100"),
//           [](Trainer &t, bool enable, const Value &value, std::string &message)
//           { return t.setGameSpeedMultiplier(enable, value.f(), message); },
//           "Restart Level"                              // info tooltip; needs INFO_ICON
//       );
//
//       ui.apply(
//           "Set Health", IntInput("100", "0", "999999999"),
//           [](Trainer &t, const Value &value, std::string &message)
//           { return t.setHealth(value.i(), message); }
//       );
//
//       // An option with rows of its own is written as a block. The sub-rows are assigned after the
//       // option so they sit under it, and are static because the handler outlives this function.
//       {
//           static SubInput level;
//           static SubToggle every_level;
//
//           ui.apply(
//               "Unlock All Levels",
//               [](Trainer &t, const Value &, std::string &message)
//               { return t.unlockLevels(level.i(), every_level.on(), message); }
//           );
//
//           level = ui.subInput("Which Level", IntInput("1", "1", "99"));
//           every_level = ui.subToggle("Unlock Every Level");
//           ui.subApply("Relock All", handler);   // a second action on the same subject
//       }
//
//       ui.column(2);    // everything after this goes to the second column
//       ui.separator();  // blank row
//   }
//
// An option can also carry a lookup table instead of tooltip text, which writes the row the user
// picks into that option's own input:
//
//   ui.apply(
//       "Add Plant to Garden", IntInput("0", "0", "39"),
//       handler,
//       InfoTable("Plant List", {"ID", "Name"}, {60, 200}, &Trainer::plantList)
//   );
//
// A label is the option's key everywhere: the widget text, the translations.json entry and the
// handler's identity, so there is no second name-keyed list to keep in step.
//
// What a value means belongs in trainer.h; the option list only reports what the widgets say.
//
// ------------------------------------------------------------------------------------------------
// Reporting failure
//
// The alert a user sees always opens with the generic line - "Failed to activate." - and then
// carries whatever the failure actually said. Nothing is swallowed: the handler's message, any
// exception text, and anything written to std::cerr all reach the box.
//
// So a handler reports the real reason and lets failure_message() frame it. A message that
// matches a translations.json key is translated, which is what a refusal the trainer itself
// predicts should use; anything else - an exception string, a runtime error - is shown verbatim,
// which is the right answer for a failure no one anticipated. Both are better than returning
// false with nothing, which leaves the generic line standing alone.
//
// Carrying the reason back from the payload differs by runtime:
//
//   mono     invokeMethodReturn() returns the payload's reply. The convention is "OK" for success
//   il2cpp   and anything else being the text to show; see the invokeCheat() in a trainer.h. The
//            payload's own catch should answer with the exception's type and message.
//
//   cdp      executeJS() hands back the expression's result and already prints a throw to stderr
//            as "fail: <what>". Return 'ok' or 'fail: <reason>' from the script.
//
//   none     the work happens in trainer.h itself, so it writes message directly.
//
//   ue       UEBase has invokeMethod() and invokeMethodReadBack<T>() but no string channel, so a
//            message has to ride back in the read-back struct. No trainer does this yet.
#pragma once

#include <FL/Fl.H>
#include <FL/Fl_Flex.H>
#include <FL/Fl_JPEG_Image.H>
#include <FL/Fl_PNG_Image.H>
#include <FL/Fl_Radio_Round_Button.H>
#include <FL/forms.H>
#include <exception>
#include <functional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "trainer.h"
#include "FLTKUtils.h"

// ============================================================
// What a trainer declares
// ============================================================

/// Where a column's options sit when they do not fill its height.
enum class ColumnAlign
{
    Top,
    Center,
};

// IntelliSense parses this header outside any target, so it sees none of add_trainer()'s
// definitions. __INTELLISENSE__ is never defined by the compiler, so these cannot reach a build.
#if defined(__INTELLISENSE__)
#ifndef TRAINER_APP_NAME
#define TRAINER_APP_NAME "Trainer"
#endif
#ifndef TRAINER_APP_GAME_VERSION
#define TRAINER_APP_GAME_VERSION "0.0"
#endif
#ifndef TRAINER_APP_TRAINER_VERSION
#define TRAINER_APP_TRAINER_VERSION "0.0"
#endif
#ifndef TRAINER_APP_WIDTH
#define TRAINER_APP_WIDTH 800
#endif
#ifndef TRAINER_APP_HEIGHT
#define TRAINER_APP_HEIGHT 600
#endif
#ifndef TRAINER_APP_COLUMN_GAPS
#define TRAINER_APP_COLUMN_GAPS 80
#endif
#ifndef TRAINER_APP_COLUMN_ALIGN
#define TRAINER_APP_COLUMN_ALIGN ColumnAlign::Center
#endif
#ifndef TRAINER_APP_ROW_GAP
#define TRAINER_APP_ROW_GAP 8
#endif
#endif

#if !defined(TRAINER_APP_NAME) || !defined(TRAINER_APP_GAME_VERSION) ||   \
    !defined(TRAINER_APP_TRAINER_VERSION) || !defined(TRAINER_APP_WIDTH) || \
    !defined(TRAINER_APP_HEIGHT) || !defined(TRAINER_APP_COLUMN_GAPS) ||   \
    !defined(TRAINER_APP_COLUMN_ALIGN) || !defined(TRAINER_APP_ROW_GAP)
#error "TrainerApp.h expects the trainer to be built through add_trainer() in its CMakeLists.txt"
#endif

struct TrainerAppInfo
{
    /// The trainer's folder name: also the executable name and the title's translations.json key.
    const char *name;
    const char *gameVersion;
    const char *trainerVersion;

    int width;
    int height;

    /// Applies to every column; per-column alignment would only look broken.
    ColumnAlign align;

    /// Vertical space between one option row and the next.
    int rowGap;
};

static const TrainerAppInfo trainer_app{
    TRAINER_APP_NAME,
    TRAINER_APP_GAME_VERSION,
    TRAINER_APP_TRAINER_VERSION,
    TRAINER_APP_WIDTH,
    TRAINER_APP_HEIGHT,
    TRAINER_APP_COLUMN_ALIGN,
    TRAINER_APP_ROW_GAP,
};

/// One gap per column, so the length is the column count. The first is the space after the cover
/// image; each of the rest precedes the column it belongs to.
static const int trainer_app_column_gaps[] = {TRAINER_APP_COLUMN_GAPS};
static constexpr int trainer_app_column_count =
    static_cast<int>(sizeof(trainer_app_column_gaps) / sizeof(trainer_app_column_gaps[0]));

/// An option's input text when it fired. Options without an input get "0".
class Value
{
public:
    explicit Value(const char *text) : text_((text && text[0]) ? text : "0") {}

    int i() const { return std::stoi(text_); }
    float f() const { return std::stof(text_); }
    const std::string &str() const { return text_; }

private:
    std::string text_;
};

/// A numeric input on an option. FLTKUtils clamps typed values to [min, max] before a handler runs.
struct Input
{
    const char *value = nullptr;
    const char *min = nullptr;
    const char *max = nullptr;
    int type = FL_INT_INPUT;
};

inline Input IntInput(const char *value, const char *min, const char *max)
{
    return Input{value, min, max, FL_INT_INPUT};
}

inline Input FloatInput(const char *value, const char *min, const char *max)
{
    return Input{value, min, max, FL_FLOAT_INPUT};
}

/// A sub-option's input. Safe to read before the widget exists or while empty, so handlers need no
/// guarding.
class SubInput
{
public:
    SubInput() = default;
    explicit SubInput(Fl_Input *widget) : widget_(widget) {}

    int i() const { return filled() ? std::stoi(widget_->value()) : 0; }
    float f() const { return filled() ? std::stof(widget_->value()) : 0.0f; }
    std::string str() const { return filled() ? widget_->value() : std::string(); }

private:
    bool filled() const { return widget_ && widget_->value() && widget_->value()[0]; }
    Fl_Input *widget_ = nullptr;
};

/// The same for a sub-option's checkbox.
class SubToggle
{
public:
    SubToggle() = default;
    explicit SubToggle(Fl_Check_Button *widget) : widget_(widget) {}

    bool on() const { return widget_ && widget_->value() != 0; }

private:
    Fl_Check_Button *widget_ = nullptr;
};

/// The "i" affordance beside an option. A plain string is tooltip text; InfoTable() instead opens a
/// lookup window, and the row the user picks is written into that option's own input. Both need
/// INFO_ICON in the manifest.
struct Info
{
    Info() = default;
    Info(const char *tooltip) : text(tooltip) {} // implicit, so a bare string still reads as an info

    const char *text = nullptr;

    const char *titleKey = nullptr;
    std::vector<std::string> columns;
    std::vector<int> widths;
    std::string (Trainer::*rows)() = nullptr;

    bool isTable() const { return rows != nullptr; }
    bool any() const { return text || isTable(); }
};

/// titleKey names the window and is a translations.json key. rows is a Trainer member returning one
/// row per line as '>'-separated fields, each run through t(); picking a row writes its first field
/// into the input.
inline Info InfoTable(const char *titleKey, std::vector<std::string> columns, std::vector<int> widths,
                      std::string (Trainer::*rows)())
{
    Info info;
    info.titleKey = titleKey;
    info.columns = std::move(columns);
    info.widths = std::move(widths);
    info.rows = rows;
    return info;
}

/// Return false to fail. A message is shown through t(), so it must match a translations.json key;
/// empty falls back to generic wording plus anything written to std::cerr.
using ToggleHandler = std::function<bool(Trainer &, bool, const Value &, std::string &)>;
using ApplyHandler = std::function<bool(Trainer &, const Value &, std::string &)>;

// ============================================================
// Registry behind the two dispatch callbacks
// ============================================================

static std::unordered_map<std::string, ToggleHandler> g_toggle_handlers;
static std::unordered_map<std::string, ApplyHandler> g_apply_handlers;

/// Declares the options, in the order they appear down the window. Handed to register_cheats().
class TrainerUI
{
public:
    TrainerUI(Trainer *trainer, std::vector<Fl_Flex *> columns)
        : trainer_(trainer), columns_(std::move(columns))
    {
    }

    /// Sends everything declared after this into column n, counting from 1. Out-of-range is clamped
    /// rather than fatal, so a mistake misplaces options instead of refusing to start.
    void column(int n)
    {
        const int count = static_cast<int>(columns_.size());
        current_ = (n < 1) ? 0 : (n > count ? count - 1 : n - 1);
    }

    void toggle(const char *label, ToggleHandler handler, Info info = {})
    {
        place(label, Input{}, std::move(info), /*isToggle=*/true);
        g_toggle_handlers[label] = std::move(handler);
    }

    void toggle(const char *label, Input input, ToggleHandler handler, Info info = {})
    {
        place(label, input, std::move(info), /*isToggle=*/true);
        g_toggle_handlers[label] = std::move(handler);
    }

    void apply(const char *label, ApplyHandler handler, Info info = {})
    {
        place(label, Input{}, std::move(info), /*isToggle=*/false);
        g_apply_handlers[label] = std::move(handler);
    }

    void apply(const char *label, Input input, ApplyHandler handler, Info info = {})
    {
        place(label, input, std::move(info), /*isToggle=*/false);
        g_apply_handlers[label] = std::move(handler);
    }

    /// A checkbox indented under the option above, with no handler of its own - that option's
    /// handler reads it when it fires.
    SubToggle subToggle(const char *label)
    {
        Fl_Flex *target = active();
        Fl_Group *previous = Fl_Group::current();
        Fl_Group::current(target);
        WidgetPair pair = place_indented_toggle_widget(target, label);
        Fl_Group::current(previous);
        return SubToggle{dynamic_cast<Fl_Check_Button *>(pair.button)};
    }

    /// An Apply button indented under the option above, for a second action on the same subject.
    void subApply(const char *label, ApplyHandler handler)
    {
        Fl_Flex *target = active();
        Fl_Group *previous = Fl_Group::current();
        Fl_Group::current(target);

        // Unlike place_apply_widget, this helper only builds the row - the callback is wired here.
        WidgetPair row = place_indented_apply_widget(target, label);
        Fl_Button *button = static_cast<Fl_Button *>(row.button);
        button->callback(apply_callback, new ApplyData{trainer_, label, button, row.input});

        Fl_Group::current(previous);
        g_apply_handlers[label] = std::move(handler);
    }

    /// As subToggle, for a value rather than a flag.
    SubInput subInput(const char *label, Input input, Info info = {})
    {
        Fl_Flex *target = active();
        Fl_Group *previous = Fl_Group::current();
        Fl_Group::current(target);
        // A sub-row has no input of its own to write into, so a table makes no sense here.
        Fl_Box *hover = info.text ? create_info_hover(info.text, info_image()) : nullptr;
        Fl_Input *field = place_indented_input_widget(target, label, input.value, input.min,
                                                      input.max, input.type, hover);
        Fl_Group::current(previous);
        return SubInput{field};
    }

    /// Blank row, for grouping unrelated options.
    void separator(int height = 5)
    {
        Fl_Flex *target = active();
        Fl_Group *previous = Fl_Group::current();
        Fl_Group::current(target);
        Fl_Box *gap = new Fl_Box(0, 0, 0, 0);
        target->fixed(gap, height);
        Fl_Group::current(previous);
    }

    /// The column currently being filled, for the rare trainer that places something of its own.
    Fl_Flex *column() const { return active(); }

private:
    Fl_Flex *active() const { return columns_[current_]; }

    void place(const char *label, Input input, Info info, bool isToggle)
    {
        Fl_Flex *target = active();

        // The widget helpers build their row with a bare `new`, so it joins the *current* group
        // rather than the flex they are handed.
        Fl_Group *previous = Fl_Group::current();
        Fl_Group::current(target);

        // A table reads the option's input when clicked and writes the picked row back into it, so
        // it needs that pointer before the row exists. These slots outlive the call for that reason.
        Fl_Input **field = new Fl_Input *(nullptr);
        Fl_Widget *affordance = nullptr;

        if (info.isTable())
            affordance = create_info_button(trainer_, field, info.columns, info.widths,
                                            info.titleKey, new Fl_Window *(nullptr), info.rows,
                                            info_image());
        else if (info.text)
            affordance = create_info_hover(info.text, info_image());

        if (isToggle)
            place_toggle_widget(target, trainer_, label, label, field,
                                input.value, input.min, input.max, input.type, affordance);
        else
            place_apply_widget(target, trainer_, label, label, field,
                               input.value, input.min, input.max, input.type, affordance);

        Fl_Group::current(previous);
    }

    /// Loaded on first use, so a trainer with no info text needs no INFO_ICON in its resources.
    static Fl_PNG_Image *info_image()
    {
        static Fl_PNG_Image *image = nullptr;
        if (!image)
        {
            DWORD size = 0;
            const unsigned char *data = load_resource("INFO_IMG", size);
            if (data && size > 0)
            {
                image = new Fl_PNG_Image(nullptr, data, (int)size);
                image->scale(20, 20, 1, 0);
            }
        }
        return image;
    }

    Trainer *trainer_ = nullptr;
    std::vector<Fl_Flex *> columns_;
    int current_ = 0;
};

// The one thing the trainer supplies.
void register_cheats(TrainerUI &ui);

// ============================================================
// Dispatch
// ============================================================

/// Always leads with the generic line, then everything the failure actually said: the handler's
/// message - translated when it is a translations.json key, left as-is when it is raw text like
/// an exception - and whatever reached std::cerr.
static std::string failure_message(const std::string &generic, const std::string &message,
                                   const std::string &details)
{
    std::string text = t(generic);

    if (!message.empty())
        text += std::string("\n\n") + t(message);

    if (!details.empty())
        text += std::string("\n\n") + details;

    return text;
}

void toggle_callback(Fl_Widget *, void *data)
{
    ToggleData *toggleData = static_cast<ToggleData *>(data);
    Trainer *trainer = toggleData->trainer;
    Fl_Check_Button *button = toggleData->button;
    const bool isEnabled = button->value() != 0;

    if (!trainer->isProcessRunning())
    {
        trainer_alert(t("Please run the game first."));
        button->value(0);
        return;
    }

    std::ostringstream errCapture;
    auto *oldBuf = std::cerr.rdbuf(errCapture.rdbuf());

    bool status = true;
    std::string message;

    try
    {
        auto handler = g_toggle_handlers.find(toggleData->optionName);
        status = (handler != g_toggle_handlers.end()) &&
                 handler->second(*trainer, isEnabled,
                                 Value(toggleData->input ? toggleData->input->value() : nullptr), message);
    }
    // A thrown exception is a reason too, so it is reported rather than folded into generic wording.
    catch (const std::exception &e)
    {
        errCapture << e.what();
        status = false;
    }
    catch (...)
    {
        status = false;
    }

    std::cerr.rdbuf(oldBuf);

    if (!status)
    {
        trainer_alert(failure_message("Failed to activate / deactivate.", message, errCapture.str()));
        button->value(isEnabled ? 0 : 1);
    }
    else if (toggleData->input)
    {
        toggleData->input->readonly(isEnabled ? 1 : 0);
    }
}

void apply_callback(Fl_Widget *, void *data)
{
    ApplyData *applyData = static_cast<ApplyData *>(data);
    Trainer *trainer = applyData->trainer;

    if (!trainer->isProcessRunning())
    {
        trainer_alert(t("Please run the game first."));
        return;
    }

    std::ostringstream errCapture;
    auto *oldBuf = std::cerr.rdbuf(errCapture.rdbuf());

    bool status = true;
    std::string message;

    try
    {
        auto handler = g_apply_handlers.find(applyData->optionName);
        status = (handler != g_apply_handlers.end()) &&
                 handler->second(*trainer,
                                 Value(applyData->input ? applyData->input->value() : nullptr), message);
    }
    // A thrown exception is a reason too, so it is reported rather than folded into generic wording.
    catch (const std::exception &e)
    {
        errCapture << e.what();
        status = false;
    }
    catch (...)
    {
        status = false;
    }

    std::cerr.rdbuf(oldBuf);

    if (!status)
        trainer_alert(failure_message("Failed to activate.", message, errCapture.str()));
}

// ============================================================
// Window
// ============================================================

static Fl_Window *g_main_window = nullptr;

/// Re-registers itself since check_process_status uses repeat_timeout internally.
static void check_process_status_wrapper(void *data)
{
    check_process_status(data);
    Fl::remove_timeout(check_process_status, data);
    Fl::repeat_timeout(1.0, check_process_status_wrapper, data);
}

static void update_window_title()
{
    if (!g_main_window)
        return;

    std::string title = std::string(t(trainer_app.name)) + " | Game ver." + trainer_app.gameVersion +
                        " | Trainer ver." + trainer_app.trainerVersion;
    g_main_window->copy_label(title.c_str());
}

static void lang_title_callback(Fl_Widget *widget, void *data)
{
    change_language_callback(widget, data);
    update_window_title();
}

static void main_window_close_callback(Fl_Widget *w, void *)
{
    turn_off_all_toggles();
    if (font_handle)
        RemoveFontMemResourceEx(font_handle);
    Fl::delete_widget(w);
}

int main(int argc, char **argv)
{
    Trainer trainer;
    setupLanguage();
    load_translations("TRANSLATION_JSON");

    int win_w = trainer_app.width;
    int win_h = trainer_app.height;
    int screen_w = Fl::w();
    int screen_h = Fl::h();
    int win_x = (screen_w - win_w) / 2;
    int win_y = (screen_h - win_h) / 2;

    g_main_window = new Fl_Window(win_x, win_y, win_w, win_h);
    Fl::scheme("gtk+");
    Fl::set_color(FL_FREE_COLOR, 0x1c1c1c00);
    g_main_window->color(FL_FREE_COLOR);
    g_main_window->icon((char *)LoadIconA(GetModuleHandle(NULL), "APP_ICON"));
    g_main_window->callback(main_window_close_callback);

    DWORD font_mem_size = 0;
    DWORD num_fonts = 0;
    const unsigned char *font_data = load_resource("FONT_TTF", font_mem_size);
    font_handle = AddFontMemResourceEx((void *)font_data, font_mem_size, nullptr, &num_fonts);
    Fl::set_font(FL_FREE_FONT, "Noto Sans SC");
    fl_font(FL_FREE_FONT, font_size);

    // ------------------------------------------------------------------
    // Top Row: Language Selection
    // ------------------------------------------------------------------
    int lang_flex_height = 30;
    int lang_flex_width = 200;

    Fl_Flex lang_flex(g_main_window->w() - lang_flex_width, 0, lang_flex_width, lang_flex_height, Fl_Flex::HORIZONTAL);
    lang_flex.color(FL_BLACK);

    Fl_Radio_Round_Button *lang_en = new Fl_Radio_Round_Button(0, 0, 0, 0, "English");
    if (language == "en_US")
        lang_en->set();
    lang_en->labelfont(FL_FREE_FONT);
    lang_en->labelsize(font_size);
    lang_en->labelcolor(FL_WHITE);
    ChangeLanguageData *changeLanguageDataEN = new ChangeLanguageData{"en_US", g_main_window};
    lang_en->callback(lang_title_callback, changeLanguageDataEN);

    Fl_Radio_Round_Button *lang_zh = new Fl_Radio_Round_Button(0, 0, 0, 0, "简体中文");
    if (language == "zh_CN")
        lang_zh->set();
    lang_zh->labelfont(FL_FREE_FONT);
    lang_zh->labelsize(font_size);
    lang_zh->labelcolor(FL_WHITE);
    ChangeLanguageData *changeLanguageDataSC = new ChangeLanguageData{"zh_CN", g_main_window};
    lang_zh->callback(lang_title_callback, changeLanguageDataSC);

    lang_flex.end();

    // ------------------------------------------------------------------
    // Left Column: Image and Process Status
    // ------------------------------------------------------------------
    std::pair<int, int> imageSize = std::make_pair(200, 300);

    DWORD img_size = 0;
    const unsigned char *img_data = load_resource("LOGO_IMG", img_size);
    if (img_data && img_size > 0)
    {
        Fl_JPEG_Image *game_img = new Fl_JPEG_Image(nullptr, img_data, (int)img_size);
        game_img->scale(imageSize.first, imageSize.second, 1, 0);
        Fl_Box *img_box = new Fl_Box(UI_LEFT_MARGIN, lang_flex_height, imageSize.first, imageSize.second);
        img_box->image(game_img);
    }

    Fl_Box *process_name = new Fl_Box(UI_LEFT_MARGIN, lang_flex_height + imageSize.second + 10, imageSize.first, font_size);
    process_name->align(FL_ALIGN_TOP_LEFT | FL_ALIGN_INSIDE);
    tr(process_name, "Process Name:");

    Fl_Box *process_exe = new Fl_Box(UI_LEFT_MARGIN, lang_flex_height + imageSize.second + font_size + 20, imageSize.first, font_size);
    process_exe->align(FL_ALIGN_TOP_LEFT | FL_ALIGN_INSIDE);

    Fl_Flex *process_id_flex = new Fl_Flex(UI_LEFT_MARGIN, lang_flex_height + imageSize.second + font_size + 55, imageSize.first, font_size, Fl_Flex::HORIZONTAL);
    process_id_flex->gap(5);

    Fl_Box *process_id_label = new Fl_Box(0, 0, 0, 0);
    process_id_label->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    tr(process_id_label, "Process ID:");

    Fl_Box *process_id = new Fl_Box(0, 0, 0, 0);
    process_id->align(FL_ALIGN_TOP_LEFT | FL_ALIGN_INSIDE);

    process_id_flex->end();

    TimeoutData *timeoutData = new TimeoutData{&trainer, process_exe, process_id};
    Fl::add_timeout(0, check_process_status_wrapper, timeoutData);

    // Mono trainers stream their injected assembly's log back; other bases have nothing to poll.
    // Templated lambda because a discarded if constexpr branch is still type-checked outside a
    // template, and Trainer is concrete here.
    [&]<class T = Trainer>()
    {
        if constexpr (requires { &T::check_logging_available; })
            Fl::add_timeout(0, &T::check_logging_available, &trainer);
    }();

    // ------------------------------------------------------------------
    // Right Column: Options
    // ------------------------------------------------------------------
    int options_x = imageSize.first + UI_LEFT_MARGIN;
    int options_y = lang_flex_height;
    int options_w = g_main_window->w() - options_x;
    int options_h = g_main_window->h() - lang_flex_height;
    // Fl_Flex has one uniform gap, but the manifest gives each column its own - so gap(0) and an
    // explicit box per gap.
    Fl_Flex *columns_flex = new Fl_Flex(options_x, options_y, options_w, options_h, Fl_Flex::HORIZONTAL);
    columns_flex->margin(0, 20, 20, 20);
    columns_flex->gap(0);

    std::vector<Fl_Flex *> columns;

    for (int i = 0; i < trainer_app_column_count; ++i)
    {
        // Constructing an Fl_Group leaves it current, so set the target group explicitly.
        Fl_Group::current(columns_flex);
        Fl_Box *gap = new Fl_Box(0, 0, 0, 0);
        columns_flex->fixed(gap, trainer_app_column_gaps[i]);

        Fl_Flex *column = new Fl_Flex(0, 0, 0, 0, Fl_Flex::VERTICAL);
        column->gap(trainer_app.rowGap);

        // Centred pads both ends, top-aligned only the tail - so this box precedes the options.
        Fl_Group::current(column);
        if (trainer_app.align == ColumnAlign::Center)
            new Fl_Box(0, 0, 0, 0);
        column->end();

        columns.push_back(column);
    }

    TrainerUI ui(&trainer, columns);
    register_cheats(ui);

    // Closing waits until the options exist, so the trailing box lands last.
    for (Fl_Flex *column : columns)
    {
        Fl_Group::current(column);
        new Fl_Box(0, 0, 0, 0);
        column->end();
    }
    columns_flex->end();

    change_language(language, g_main_window);
    update_window_title();

    g_main_window->end();
    g_main_window->show(argc, argv);
    return Fl::run();
}
