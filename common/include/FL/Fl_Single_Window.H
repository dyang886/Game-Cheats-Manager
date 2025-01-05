//
// Single-buffered window header file for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2024 by Bill Spitzak and others.
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
   Fl_Single_Window class . */

#ifndef Fl_Single_Window_H
#define Fl_Single_Window_H

#include "Fl_Window.H"

/**
  This is the same as Fl_Window.  However, it is possible that
  some implementations will provide double-buffered windows by default.
  This subclass can be used to force single-buffering.  This may be
  useful for modifying existing programs that use incremental update, or
  for some types of image data, such as a movie flipbook.
*/
class FL_EXPORT Fl_Single_Window : public Fl_Window {
public:
  void show() FL_OVERRIDE;
  /** Same as Fl_Window::show(int argc, char **argv) */
  void show(int argc, char **argv) { Fl_Window::show(argc, argv);}

   /**
    Creates a new Fl_Single_Window widget using the given
    size, and label (title) string.
  */
  Fl_Single_Window(int W, int H, const char *l=0);

  /**
    Creates a new Fl_Single_Window widget using the given
    position, size, and label (title) string.
  */
  Fl_Single_Window(int X, int Y, int W, int H, const char *l=0);

  /**
    Same as Fl_Window::make_current()
  */
  void make_current() { Fl_Window::make_current(); }

  /**
    Same as Fl_Window::flush()
  */
  void flush() FL_OVERRIDE { Fl_Window::flush(); }
};

#endif
