//
// Horizontal value slider header file for the Fast Light Tool Kit (FLTK).
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
   Fl_Hor_Value_Slider widget . */

#ifndef Fl_Hor_Value_Slider_H
#define Fl_Hor_Value_Slider_H

#include "Fl_Value_Slider.H"

class FL_EXPORT Fl_Hor_Value_Slider : public Fl_Value_Slider {
public:
  Fl_Hor_Value_Slider(int X,int Y,int W,int H,const char *l=0);
};

#endif
