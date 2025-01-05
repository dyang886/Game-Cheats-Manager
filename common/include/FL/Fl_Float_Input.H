//
// Floating point input header file for the Fast Light Tool Kit (FLTK).
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
   Fl_Float_Input widget . */

#ifndef Fl_Float_Input_H
#define Fl_Float_Input_H

#include "Fl_Input.H"

/**
  The Fl_Float_Input class is a subclass of Fl_Input
  that only allows the user to type floating point numbers (sign,
  digits, decimal point, more digits, 'E' or 'e', sign, digits).
*/
class FL_EXPORT Fl_Float_Input : public Fl_Input {
public:
  /**
    Creates a new Fl_Float_Input widget using the given position,
    size, and label string. The default boxtype is FL_DOWN_BOX.

    Inherited destructor destroys the widget and any value associated with it.
  */
  Fl_Float_Input(int X,int Y,int W,int H,const char *l = 0);
};

#endif
