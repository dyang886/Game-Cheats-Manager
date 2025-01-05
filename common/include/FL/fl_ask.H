//
// Standard dialog header file for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2022 by Bill Spitzak and others.
//
// This library is free software. Distribution and use rights are outlined in
// the file "COPYING" which should have been included with this file.  If this
// file is missing or damaged, see the license at:
//
//     https://www.fltk.org/COPYING.php
//
// Please see the following page on how to report bugs and issues:
//
//     https://www.fltk.org/bugs.php
//

/** \file fl_ask.H
  API for common dialogs.
*/

#ifndef _FL_fl_ask_H_
#define _FL_fl_ask_H_

#include <FL/Enumerations.H>
#include <FL/fl_attr.h>

#if (FLTK_USE_STD)
#include <string>
#endif

class Fl_Widget;

/** Defines the different system beeps available.

  Some systems may play different sounds or use different sound volume
  depending on the Fl_Beep value. The implementation is platform dependent.

  \sa fl_beep(int)
*/
enum Fl_Beep {
  FL_BEEP_DEFAULT = 0, ///< Default beep.
  FL_BEEP_MESSAGE,     ///< Message beep.
  FL_BEEP_ERROR,       ///< Error beep.
  FL_BEEP_QUESTION,    ///< Question beep.
  FL_BEEP_PASSWORD,    ///< Password beep.
  FL_BEEP_NOTIFICATION ///< Notification beep.
};

FL_EXPORT void fl_beep(int type = FL_BEEP_DEFAULT);
FL_EXPORT void fl_message(const char *, ...) __fl_attr((__format__(__printf__, 1, 2)));
FL_EXPORT void fl_alert(const char *, ...) __fl_attr((__format__(__printf__, 1, 2)));

// fl_ask() is deprecated since it uses "Yes" and "No" for the buttons,
// which does not conform to the current FLTK Human Interface Guidelines.
// Use fl_choice() or fl_choice_n() with the appropriate verbs instead.

FL_EXPORT int fl_ask(const char *, ...) __fl_attr((__format__(__printf__, 1, 2), __deprecated__));

FL_EXPORT int fl_choice(const char *q, const char *b0, const char *b1, const char *b2, ...)
    __fl_attr((__format__(__printf__, 1, 5)));
FL_EXPORT const char *fl_input(const char *label, const char *deflt = 0, ...) __fl_attr((__format__(__printf__, 1, 3)));
FL_EXPORT const char *fl_password(const char *label, const char *deflt = 0, ...)
    __fl_attr((__format__(__printf__, 1, 3)));

// since FLTK 1.3.8:
// - fl_choice_n()     with extended return value (-2, -1, 0, 1, 2)

FL_EXPORT int fl_choice_n(const char *q, const char *b0, const char *b1, const char *b2, ...)
    __fl_attr((__format__(__printf__, 1, 5)));

// since FLTK 1.4.0: with 'maxchar' to limit input size

FL_EXPORT const char *fl_input(int maxchar, const char *label, const char *deflt = 0, ...)
    __fl_attr((__format__(__printf__, 2, 4)));

FL_EXPORT const char *fl_password(int maxchar, const char *label, const char *deflt = 0, ...)
    __fl_attr((__format__(__printf__, 2, 4)));

// since FLTK 1.4.0 -- only with option FLTK_USE_STD

// - fl_input_str()    with limited input size, returns std::string
// - fl_password_str() with limited input size, returns std::string

#if (FLTK_USE_STD)

FL_EXPORT std::string fl_input_str(int maxchar, const char *label, const char *deflt = 0, ...)
    __fl_attr((__format__(__printf__, 2, 4)));

FL_EXPORT std::string fl_input_str(int &ret, int maxchar, const char *label, const char *deflt = 0, ...)
    __fl_attr((__format__(__printf__, 3, 5)));

FL_EXPORT std::string fl_password_str(int maxchar, const char *label, const char *deflt = 0, ...)
    __fl_attr((__format__(__printf__, 2, 4)));

FL_EXPORT std::string fl_password_str(int &ret, int maxchar, const char *label, const char *deflt = 0, ...)
    __fl_attr((__format__(__printf__, 3, 5)));

#endif

FL_EXPORT Fl_Widget *fl_message_icon();
extern FL_EXPORT Fl_Font fl_message_font_;
extern FL_EXPORT Fl_Fontsize fl_message_size_;
inline void fl_message_font(Fl_Font f, Fl_Fontsize s) {
  fl_message_font_ = f;
  fl_message_size_ = s;
}

FL_EXPORT void fl_message_hotspot(int enable);
FL_EXPORT int fl_message_hotspot(void);

// since FLTK 1.4.0: fl_message_position()

FL_EXPORT void fl_message_position(const int x, const int y, const int center = 0);
FL_EXPORT void fl_message_position(Fl_Widget *widget);
FL_EXPORT int fl_message_position(int *x = 0, int *y = 0);

/** \see fl_message_position(Fl_Widget *widget). */
inline void fl_message_position(Fl_Widget &widget) {
  fl_message_position(&widget);
}

FL_EXPORT void fl_message_title(const char *title);
FL_EXPORT void fl_message_title_default(const char *title);

FL_EXPORT void fl_message_icon_label(const char *str);

// pointers you can use to change FLTK to another language:
extern FL_EXPORT const char *fl_no;
extern FL_EXPORT const char *fl_yes;
extern FL_EXPORT const char *fl_ok;
extern FL_EXPORT const char *fl_cancel;
extern FL_EXPORT const char *fl_close;

#endif // !_FL_fl_ask_H_
