//
// Secret input header file for the Fast Light Tool Kit (FLTK).
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
   Fl_Secret_Input widget . */

#ifndef Fl_Secret_Input_H
#define Fl_Secret_Input_H

#include "Fl_Input.H"

/**
  The Fl_Secret_Input class is a subclass of Fl_Input that displays its
  input as a string of placeholders. Depending on the platform this
  placeholder is either the asterisk ('*') or the Unicode bullet
  character (U+2022).

  This subclass is usually used to receive passwords and other "secret" information.
*/
class FL_EXPORT Fl_Secret_Input : public Fl_Input {
public:
  /**
    Creates a new Fl_Secret_Input widget using the given
    position, size, and label string. The default boxtype is FL_DOWN_BOX.

    Inherited destructor destroys the widget and any value associated with it.
  */
  Fl_Secret_Input(int X,int Y,int W,int H,const char *l = 0);
  int handle(int) FL_OVERRIDE;
};

#endif
