//
// Timer header file for the Fast Light Tool Kit (FLTK).
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
   Fl_Timer widget . */

#ifndef Fl_Timer_H
#define Fl_Timer_H

#ifndef Fl_Widget_H
#include "Fl_Widget.H"
#endif

// values for type():
#define FL_NORMAL_TIMER         0
#define FL_VALUE_TIMER          1
#define FL_HIDDEN_TIMER         2

/**
  This is provided only to emulate the Forms Timer widget.  It works by
  making a timeout callback every 1/5 second.  This is wasteful and
  inaccurate if you just want something to happen a fixed time in the
  future.  You should directly call
  Fl::add_timeout() instead.
*/
class FL_EXPORT Fl_Timer : public Fl_Widget {
  static void stepcb(void *);
  void step();
  char on, direction_;
  double delay, total;
  long lastsec,lastusec;
protected:
  void draw() FL_OVERRIDE;
public:
  int handle(int) FL_OVERRIDE;
  Fl_Timer(uchar t,int x,int y,int w,int h, const char *l);
  ~Fl_Timer();
  void value(double);
  /** See void Fl_Timer::value(double)  */
  double value() const {return delay>0.0?delay:0.0;}
  /**
    Gets or sets the direction of the timer.  If the direction is zero
    then the timer will count up, otherwise it will count down from the
    initial value().
  */
  char direction() const {return direction_;}
  /**
    Gets or sets the direction of the timer.  If the direction is zero
    then the timer will count up, otherwise it will count down from the
    initial value().
  */
  void direction(char d) {direction_ = d;}
  /**    Gets or sets whether the timer is suspended.  */
  char suspended() const {return !on;}
  void suspended(char d);
};

#endif

