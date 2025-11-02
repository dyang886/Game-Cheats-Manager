//
// Scroll bar header file for the Fast Light Tool Kit (FLTK).
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
   Fl_Scrollbar widget . */

#ifndef Fl_Scrollbar_H
#define Fl_Scrollbar_H

#include "Fl_Slider.H"

/**
  The Fl_Scrollbar widget displays a slider with arrow buttons at
  the ends of the scrollbar. Clicking on the arrows move up/left and
  down/right by linesize(). Scrollbars also accept FL_SHORTCUT events:
  the arrows move by linesize(), and vertical scrollbars take Page
  Up/Down (they move by the page size minus linesize()) and Home/End
  (they jump to the top or bottom).

  Scrollbars have step(1) preset (they always return integers). If
  desired you can set the step() to non-integer values. You will then
  have to use casts to get at the floating-point versions of value()
  from Fl_Slider.

  \image html  scrollbar.png
  \image latex scrollbar.png "Fl_Scrollbar" width=4cm
*/
class FL_EXPORT Fl_Scrollbar : public Fl_Slider {

  int linesize_;
  int pushed_;
  static void timeout_cb(void*);
  void increment_cb();
protected:
  void draw() FL_OVERRIDE;

public:

  Fl_Scrollbar(int X,int Y,int W,int H, const char *L = 0);
  ~Fl_Scrollbar();
  int handle(int) FL_OVERRIDE;

  /**
    Gets the integer value (position) of the slider in the scrollbar.
    You can get the floating point value with Fl_Slider::value().

    \see Fl_Scrollbar::value(int p)
    \see Fl_Scrollbar::value(int pos, int size, int first, int total)
  */
  int value() const {return int(Fl_Slider::value());}

  /**
    Sets the value (position) of the slider in the scrollbar.

    \see Fl_Scrollbar::value()
    \see Fl_Scrollbar::value(int pos, int size, int first, int total)
  */
  int value(int p) {return int(Fl_Slider::value((double)p));}

  /**
    Sets the position, size and range of the slider in the scrollbar.
    \param[in] pos   position, first line displayed
    \param[in] windowSize  number of lines displayed
    \param[in] first_line number of first line
    \param[in] total_lines total number of lines

    You should call this every time your window changes size, your data
    changes size, or your scroll position changes (even if in response
    to a callback from this scrollbar).
    All necessary calls to redraw() are done.

    Calls Fl_Slider::scrollvalue(int pos, int size, int first, int total).
  */
  int value(int pos, int windowSize, int first_line, int total_lines) {
    return scrollvalue(pos, windowSize, first_line, total_lines);
  }

  /**
    Get the size of step, in lines, that the arror keys move.
  */
  int linesize() const {return linesize_;}

  /**
    This number controls how big the steps are that the arrow keys do.
    In addition page up/down move by the size last sent to value()
    minus one linesize().  The default is 16.
  */
  void linesize(int i) {linesize_ = i;}

};

#endif
