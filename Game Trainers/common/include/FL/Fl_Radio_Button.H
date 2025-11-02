//
// Radio button header file for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2014 by Bill Spitzak and others.
//
// This library is free software. Distribution and use rights are outlined in
// the file "COPYING" which should have been included with this file. If this
// file is missing or damaged, see the license at:
//
//     https://www.fltk.org/COPYING.php
//
// Please see the following page on how to report bugs and issues:
//
//     https://www.fltk.org/bugs.php
//

/* \file
   Fl_Radio_Button widget . */

#ifndef Fl_Radio_Button_H
#define Fl_Radio_Button_H

#include "Fl_Button.H"

class FL_EXPORT Fl_Radio_Button : public Fl_Button {
public:
  Fl_Radio_Button(int X,int Y,int W,int H,const char *L=0);
};

#endif
