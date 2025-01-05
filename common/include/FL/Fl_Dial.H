//
// Dial header file for the Fast Light Tool Kit (FLTK).
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
   Fl_Dial widget . */

#ifndef Fl_Dial_H
#define Fl_Dial_H

#ifndef Fl_Valuator_H
#include "Fl_Valuator.H"
#endif

// values for type():
#define FL_NORMAL_DIAL  0       /**< type() for dial variant with dot */
#define FL_LINE_DIAL    1       /**< type() for dial variant with line */
#define FL_FILL_DIAL    2       /**< type() for dial variant with filled arc */

/**
  The Fl_Dial widget provides a circular dial to control a
  single floating point value.
  \image html dial.png
  \image latex dial.png "Fl_Dial" width=4cm
  Use type() to set the type of the dial to:
  <UL>
  <LI>FL_NORMAL_DIAL - Draws a normal dial with a knob. </LI>
  <LI>FL_LINE_DIAL - Draws a dial with a line. </LI>
  <LI>FL_FILL_DIAL - Draws a dial with a filled arc. </LI>
  </UL>

*/
class FL_EXPORT Fl_Dial : public Fl_Valuator {

  short a1,a2;

protected:

  // these allow subclasses to put the dial in a smaller area:
  void draw(int X, int Y, int W, int H);
  int handle(int event, int X, int Y, int W, int H);
  void draw() FL_OVERRIDE;

public:

  int handle(int) FL_OVERRIDE;
  /**
    Creates a new Fl_Dial widget using the given position, size,
    and label string. The default type is FL_NORMAL_DIAL.
  */
  Fl_Dial(int x,int y,int w,int h, const char *l = 0);
  /**
     Sets Or gets the angles used for the minimum and maximum values.  The default
     values are 45 and 315 (0 degrees is straight down and the angles
     progress clockwise).  Normally angle1 is less than angle2, but if you
     reverse them the dial moves counter-clockwise.
  */
  short angle1() const {return a1;}
  /** See short angle1() const */
  void angle1(short a) {a1 = a;}
  /** See short angle1() const */
  short angle2() const {return a2;}
  /** See short angle1() const */
  void angle2(short a) {a2 = a;}
  /** See short angle1() const */
  void angles(short a, short b) {a1 = a; a2 = b;}

};

#endif
