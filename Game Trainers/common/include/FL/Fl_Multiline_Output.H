//
// Multi line output header file for the Fast Light Tool Kit (FLTK).
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
   Fl_Multiline_Output widget . */

#ifndef Fl_Multiline_Output_H
#define Fl_Multiline_Output_H

#include "Fl_Output.H"

/**
  This widget is a subclass of Fl_Output that displays multiple
  lines of text. It also displays tab characters as whitespace to the
  next column.

  Note that this widget does not support scrollbars, or per-character
  color control.

  If you are presenting large amounts of read-only text
  and need scrollbars, or full color control of characters,
  then use Fl_Text_Display. If you want to display HTML text,
  use Fl_Help_View.

  A caret cursor (^) shows the keyboard navigation mark
  for keyboard selection of the output text, e.g. Arrow Keys
  to move the cursor, Shift + Arrow Keys to create a text
  selection, and '^C' to copy the selected text to the paste buffer.
  The caret cursor can be disabled by disabling the widget's
  "visible focus" using clear_visible_focus(), inherited from the
  Fl_Widget base class. Doing this also disables the widget's
  keyboard navigation.
*/
class FL_EXPORT Fl_Multiline_Output : public Fl_Output {
public:

  /**
    Creates a new Fl_Multiline_Output widget using the given position,
    size, and label string. The default boxtype is FL_DOWN_BOX.

    Inherited destructor destroys the widget and any value associated with it.
  */
  Fl_Multiline_Output(int X,int Y,int W,int H,const char *l = 0);
};

#endif
