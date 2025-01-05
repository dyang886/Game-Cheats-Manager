//
// Shortcut Button header file for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2023 by Bill Spitzak and others.
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

#ifndef Fl_Shortcut_Button_H
#define Fl_Shortcut_Button_H

#include <FL/Fl_Button.H>

class FL_EXPORT Fl_Shortcut_Button : public Fl_Button {
private:
  bool hot_, pre_hot_, default_set_, handle_default_button_;
  Fl_Shortcut pre_esc_;
  Fl_Shortcut default_shortcut_;
protected:
  Fl_Shortcut shortcut_value;
  void do_end_hot_callback();
  int handle(int) FL_OVERRIDE;
  void draw() FL_OVERRIDE;
public:
  Fl_Shortcut_Button(int X,int Y,int W,int H, const char* l = 0);
  void value(Fl_Shortcut shortcut);
  Fl_Shortcut value();
#if 0
  // Default shortcut settings are disabled until successful review of the UI
  void default_value(Fl_Shortcut shortcut);
  Fl_Shortcut default_value();
  void default_clear();
#endif
};

#endif // Fl_Shortcut_Button_H

