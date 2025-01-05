//
// Box header file for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2023 by Bill Spitzak and others.
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

/** \file FL/Fl_Box.H
  \brief Fl_Box widget.
*/

#ifndef Fl_Box_H
#define Fl_Box_H

#ifndef Fl_Widget_H
#include "Fl_Widget.H"
#endif

/**
  This widget simply draws its box, and possibly its label.  Putting it
  before some other widgets and making it big enough to surround them
  will let you draw a frame around them.
*/
class FL_EXPORT Fl_Box : public Fl_Widget {
protected:
  void draw() FL_OVERRIDE;
public:
  /**
    - The first constructor sets box() to FL_NO_BOX, which means it is invisible.
      However such widgets are useful as placeholders or Fl_Group::resizable()
      values. To change the box to something visible, use box(n).
    - The second form of the constructor sets the box to the specified box type.

    The destructor removes the box.
  */
  Fl_Box(int X, int Y, int W, int H, const char *l=0);

  /**    See Fl_Box::Fl_Box(int x, int y, int w, int h, const char * = 0)   */
  Fl_Box(Fl_Boxtype b, int X, int Y, int W, int H, const char *l);

  int handle(int) FL_OVERRIDE;
};

#endif
