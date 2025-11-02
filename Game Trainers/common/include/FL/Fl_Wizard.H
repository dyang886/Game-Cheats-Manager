//
// Fl_Wizard widget definitions.
//
// Copyright 1999-2010 by Easy Software Products.
// Copyright 2011-2020 by Bill Spitzak and others.
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

/* \file
   Fl_Wizard widget . */

//
// Include necessary header files...
//

#ifndef _Fl_Wizard_H_
#define _Fl_Wizard_H_

#include <FL/Fl_Group.H>

/**
  This widget is based off the Fl_Tabs widget, but instead of
  displaying tabs it only changes "tabs" under program control.

  Its primary purpose is to support "wizards" that step a user
  through configuration or troubleshooting tasks.

  As with Fl_Tabs, wizard panes are composed of child (usually
  Fl_Group) widgets. Navigation buttons must be added separately.
*/
class FL_EXPORT Fl_Wizard : public Fl_Group {

  Fl_Widget *value_;

protected:

  void draw() FL_OVERRIDE;

public:

  Fl_Wizard(int, int, int, int, const char * = 0);

  void next();
  void prev();
  Fl_Widget *value();
  void value(Fl_Widget *);
};

#endif // !_Fl_Wizard_H_
