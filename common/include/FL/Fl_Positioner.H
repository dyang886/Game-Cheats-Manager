//
// Positioner header file for the Fast Light Tool Kit (FLTK).
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
   Fl_Positioner widget . */

#ifndef Fl_Positioner_H
#define Fl_Positioner_H

#ifndef Fl_Widget_H
#include "Fl_Widget.H"
#endif

/**
  This class is provided for Forms compatibility.  It provides 2D input.
  It would be useful if this could be put atop another widget so that the
  crosshairs are on top, but this is not implemented.  The color of the
  crosshairs is selection_color().
  \image html  positioner.png
  \image latex positioner.png "Fl_Positioner" width=4cm
*/
class FL_EXPORT Fl_Positioner : public Fl_Widget {

  double xmin, ymin;
  double xmax, ymax;
  double xvalue_, yvalue_;
  double xstep_, ystep_;

protected:

  // these allow subclasses to put the dial in a smaller area:
  void draw(int, int, int, int);
  int handle(int, int, int, int, int);
  void draw() FL_OVERRIDE;

public:

  int handle(int) FL_OVERRIDE;
  /**
    Creates a new Fl_Positioner widget using the given position,
    size, and label string. The default boxtype is FL_NO_BOX.
  */
  Fl_Positioner(int x,int y,int w,int h, const char *l=0);
  /** Gets the X axis coordinate.*/
  double xvalue() const {return xvalue_;}
  /** Gets the Y axis coordinate.*/
  double yvalue() const {return yvalue_;}
  int xvalue(double);
  int yvalue(double);
  int value(double,double);
  void xbounds(double, double);
  /** Gets the X axis minimum */
  double xminimum() const {return xmin;}
  /** Same as xbounds(a, xmaximum()) */
  void xminimum(double a) {xbounds(a,xmax);}
  /** Gets the X axis maximum */
  double xmaximum() const {return xmax;}
  /** Same as xbounds(xminimum(), a) */
  void xmaximum(double a) {xbounds(xmin,a);}
  void ybounds(double, double);
  /** Gets the Y axis minimum */
  double yminimum() const {return ymin;}
  /** Same as ybounds(a, ymaximum()) */
  void yminimum(double a) {ybounds(a, ymax);}
  /** Gets the Y axis maximum */
  double ymaximum() const {return ymax;}
  /** Same as ybounds(ymininimum(), a) */
  void ymaximum(double a) {ybounds(ymin, a);}
  /** Sets the stepping value for the X axis.*/
  void xstep(double a) {xstep_ = a;}
  /** Sets the stepping value for the Y axis.*/
  void ystep(double a) {ystep_ = a;}
};

#endif
