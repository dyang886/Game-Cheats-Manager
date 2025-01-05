//
// Progress bar widget definitions.
//
// Copyright 2000-2010 by Michael Sweet.
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
   Fl_Progress widget . */

#ifndef _Fl_Progress_H_
#  define _Fl_Progress_H_

//
// Include necessary headers.
//

#include "Fl_Widget.H"


//
// Progress class...
//
/**
    Displays a progress bar for the user.
*/
class FL_EXPORT Fl_Progress : public Fl_Widget {

  float value_,
        minimum_,
        maximum_;

  protected:

  void draw() FL_OVERRIDE;

  public:

  Fl_Progress(int x, int y, int w, int h, const char *l = 0);

  /** Sets the maximum value in the progress widget.  */
  void  maximum(float v) { maximum_ = v; redraw(); }
  /** Gets the maximum value in the progress widget.  */
  float maximum() const { return (maximum_); }

  /** Sets the minimum value in the progress widget.  */
  void  minimum(float v) { minimum_ = v; redraw(); }
  /** Gets the minimum value in the progress widget.  */
  float minimum() const { return (minimum_); }

  /** Sets the current value in the progress widget.  */
  void  value(float v) { value_ = v; redraw(); }
  /** Gets the current value in the progress widget.  */
  float value() const { return (value_); }
};

#endif // !_Fl_Progress_H_
