//
// Round clock header file for the Fast Light Tool Kit (FLTK).
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
   Fl_Round_Clock widget . */

#ifndef Fl_Round_Clock_H
#define Fl_Round_Clock_H

#include "Fl_Clock.H"

/** A clock widget of type FL_ROUND_CLOCK. Has no box. */
class FL_EXPORT Fl_Round_Clock : public Fl_Clock {
public:
  /** Creates the clock widget, setting his type and box. */
  Fl_Round_Clock(int X,int Y,int W,int H, const char *L = 0);
};

#endif
