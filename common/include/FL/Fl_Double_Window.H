//
// Double-buffered window header file for the Fast Light Tool Kit (FLTK).
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
   Fl_Double_Window widget . */

#ifndef Fl_Double_Window_H
#define Fl_Double_Window_H

#include "Fl_Window.H"

/**
  The Fl_Double_Window provides a double-buffered window.
  It will draw the window data into an off-screen pixmap, and then
  copy it to the on-screen window.
*/
class FL_EXPORT Fl_Double_Window : public Fl_Window
{
public:
  Fl_Double_Window *as_double_window() FL_OVERRIDE {return this; }
  void show() FL_OVERRIDE;
  /** Same as Fl_Window::show(int a, char **b) */
  void show(int a, char **b) {Fl_Window::show(a,b);}
  void resize(int,int,int,int) FL_OVERRIDE;
  void hide() FL_OVERRIDE;
  void flush() FL_OVERRIDE;
  ~Fl_Double_Window();

  /**
   Creates a new Fl_Double_Window widget using the given
   position, size, and label (title) string.
  */
  Fl_Double_Window(int W, int H, const char *l = 0);

  /**
   See  Fl_Double_Window::Fl_Double_Window(int w, int h, const char *label = 0)
  */
  Fl_Double_Window(int X, int Y, int W, int H, const char *l = 0);

};

#endif
