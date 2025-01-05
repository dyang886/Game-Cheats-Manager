//
// Simple counter header file for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2022 by Bill Spitzak and others.
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
   Fl_Simple_Counter widget . */

#ifndef Fl_Simple_Counter_H
#define Fl_Simple_Counter_H

#include "Fl_Counter.H"
/**
  This widget creates a counter with only 2 arrow buttons
  \image html counter.png
  \image latex  counter.png "Fl_Simple_Counter" width=4cm
*/
class FL_EXPORT Fl_Simple_Counter : public Fl_Counter {
public:
  Fl_Simple_Counter(int X,int Y,int W,int H, const char *L = 0);
};

#endif
