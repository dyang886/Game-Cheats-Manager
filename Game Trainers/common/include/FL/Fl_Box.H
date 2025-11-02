//
// Box header file for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2025 by Bill Spitzak and others.
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
  This widget simply draws its box, and possibly its label.

  Putting it before some other widgets and making it big enough to surround
  them will let you draw a frame around them.
*/
class FL_EXPORT Fl_Box : public Fl_Widget {
protected:
  void draw() FL_OVERRIDE;
public:
  /**
    Creates a new Fl_Box widget with the given coordinates, size, and label.

    This constructor sets box() to FL_NO_BOX, which means it is invisible.

    However such widgets are useful as placeholders or Fl_Group::resizable()
    values. To change the box to something visible, use box(Fl_Boxtype).

    The destructor removes the box from its parent group.

    \param[in] X, Y the position of the widget relative to the enclosing window
    \param[in] W, H size of the widget in pixels
    \param[in] L    optional text for the widget label (default: no label)

    \see Fl_Box(Fl_Boxtype b, int X, int Y, int W, int H, const char *L)
  */
  Fl_Box(int X, int Y, int W, int H, const char *L = 0);

  /**
    Creates a new Fl_Box widget with the given boxtype, coordinates, size, and label.

    This constructor sets box() to the given Fl_Boxtype \c B.

    You must also specify a label but it can be \c nullptr (0, NULL) if you
    don't want or need a visible label.

    The destructor removes the box from its parent group.

    \param[in] B    boxtype of the widget
    \param[in] X, Y the position of the widget relative to the enclosing window
    \param[in] W, H size of the widget in pixels
    \param[in] L    optional text for the widget label (see description)

    \see Fl_Box(int X, int Y, int W, int H, const char *L)
  */
  Fl_Box(Fl_Boxtype B, int X, int Y, int W, int H, const char *L);

  int handle(int) FL_OVERRIDE;
};

#endif
