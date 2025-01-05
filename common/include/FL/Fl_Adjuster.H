//
// Adjuster widget header file for the Fast Light Tool Kit (FLTK).
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
   Fl_Adjuster widget . */

// 3-button "slider", made for Nuke

#ifndef Fl_Adjuster_H
#define Fl_Adjuster_H

#ifndef Fl_Valuator_H
#include "Fl_Valuator.H"
#endif

/**
  The Fl_Adjuster widget was stolen from Prisms, and has proven
  to be very useful for values that need a large dynamic range.
  \image html adjuster1.png
  \image latex adjuster1.png "Fl_Adjuster" width=4cm
  <P>When you  press a button and drag to the right the value increases.
  When you drag  to the left it decreases.  The largest button adjusts by
  100 *  step(), the next by 10 * step() and that
  smallest button  by step().  Clicking on the buttons
  increments by 10 times the  amount dragging by a pixel does. Shift +
  click decrements by 10 times  the amount.
*/
class FL_EXPORT Fl_Adjuster : public Fl_Valuator {
  int drag;
  int ix;
  int soft_;
protected:
  void draw() FL_OVERRIDE;
  int handle(int) FL_OVERRIDE;
  void value_damage() FL_OVERRIDE;
public:
  Fl_Adjuster(int X,int Y,int W,int H,const char *l=0);
  /**
    If "soft" is turned on, the user is allowed to drag the value outside
    the range.  If they drag the value to one of the ends, let go, then
    grab again and continue to drag, they can get to any value.  Default is
    one.
  */
  void soft(int s) {soft_ = s;}
  /**
    If "soft" is turned on, the user is allowed to drag the value outside
    the range.  If they drag the value to one of the ends, let go, then
    grab again and continue to drag, they can get to any value.  Default is
    one.
  */
  int soft() const {return soft_;}
};

#endif
