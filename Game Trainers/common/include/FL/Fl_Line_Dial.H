//
// Line dial header file for the Fast Light Tool Kit (FLTK).
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
   Fl_Line_Dial widget . */

#ifndef Fl_Line_Dial_H
#define Fl_Line_Dial_H

#include "Fl_Dial.H"

class FL_EXPORT Fl_Line_Dial : public Fl_Dial {
public:
  Fl_Line_Dial(int X,int Y,int W,int H, const char *L = 0);
};

#endif
