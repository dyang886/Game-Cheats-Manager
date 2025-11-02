//
// Menu window header file for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2010 by Bill Spitzak and others.
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
   Fl_Menu_Window widget . */

#ifndef Fl_Menu_Window_H
#define Fl_Menu_Window_H

#include "Fl_Single_Window.H"

/**
  The Fl_Menu_Window widget is a window type used for menus. By
  default the window is drawn in the hardware overlay planes if they are
  available so that the menu don't force the rest of the window to
  redraw.
*/
class FL_EXPORT Fl_Menu_Window : public Fl_Single_Window {
public:
  ~Fl_Menu_Window();
  /** Creates a new Fl_Menu_Window widget using the given size, and label string. */
  Fl_Menu_Window(int W, int H, const char *l = 0);
  /** Creates a new Fl_Menu_Window widget using the given position, size, and label string. */
  Fl_Menu_Window(int X, int Y, int W, int H, const char *l = 0);
};

#endif
