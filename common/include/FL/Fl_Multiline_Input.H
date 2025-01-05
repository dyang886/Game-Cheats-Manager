//
// Multiline input header file for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2011 by Bill Spitzak and others.
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
   Fl_Multiline_Input widget . */

#ifndef Fl_Multiline_Input_H
#define Fl_Multiline_Input_H

#include "Fl_Input.H"

/**
  This input field displays '\\n' characters as new lines rather than ^J,
  and accepts the Return, Tab, and up and down arrow keys.  This is for
  editing multiline text.

  This is far from the nirvana of text editors, and is probably only
  good for small bits of text, 10 lines at most. Note that this widget
  does not support scrollbars or per-character color control.

  If you are presenting large amounts of text and need scrollbars
  or full color control of characters, you probably want Fl_Text_Editor
  instead.

  In FLTK 1.3.x, the default behavior of the 'Tab' key was changed
  to support consistent focus navigation. To get the older FLTK 1.1.x
  behavior, set Fl_Input_::tab_nav() to 0. Newer programs should consider using
  Fl_Text_Editor.
*/
class FL_EXPORT Fl_Multiline_Input : public Fl_Input {
public:
  /**
    Creates a new Fl_Multiline_Input widget using the given
    position, size, and label string. The default boxtype is FL_DOWN_BOX.

    Inherited destructor destroys the widget and any value associated with it.
  */
  Fl_Multiline_Input(int X,int Y,int W,int H,const char *l = 0);
};

#endif
