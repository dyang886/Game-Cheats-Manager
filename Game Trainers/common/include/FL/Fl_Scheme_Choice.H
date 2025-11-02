//
// Scheme Choice header for the Fast Light Tool Kit (FLTK).
//
// Copyright 2022-2023 by Bill Spitzak and others.
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

#ifndef FL_Fl_Scheme_Choice_H_
#define FL_Fl_Scheme_Choice_H_

#include <FL/Fl.H>
#include <FL/Fl_Scheme.H>
#include <FL/Fl_Choice.H>

class FL_EXPORT Fl_Scheme_Choice : public Fl_Choice {

protected:
  static void scheme_cb_(Fl_Widget *w, void *);

public:
  Fl_Scheme_Choice(int X, int Y, int W, int H, const char *L = 0);
  int handle(int event) FL_OVERRIDE;

  // set the current value according to the active scheme
  virtual void init_value();

}; // class Fl_Scheme_Choice

#endif // FL_Fl_Scheme_Choice_H_
