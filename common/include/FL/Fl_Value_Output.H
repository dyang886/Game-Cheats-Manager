//
// Value output header file for the Fast Light Tool Kit (FLTK).
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
   Fl_Value_Output widget . */

#ifndef Fl_Value_Output_H
#define Fl_Value_Output_H

#ifndef Fl_Valuator_H
#include "Fl_Valuator.H"
#endif

/**
  The Fl_Value_Output widget displays a floating point value.
  If step() is not zero, the user can adjust the value by
  dragging the mouse left and right.  The left button moves one step()
  per pixel, the middle by 10 * step(), and the right button by
  100 * step().
  <P>This is much lighter-weight than
  Fl_Value_Input because it contains no text editing code or
  character buffer. </P>
  \image html  Fl_Value_Output.png
  \image latex Fl_Value_Output.png "Fl_Value_Output" width=4cm
*/
class FL_EXPORT Fl_Value_Output : public Fl_Valuator {
  Fl_Font textfont_;
  Fl_Fontsize textsize_;
  uchar soft_;
  Fl_Color textcolor_;

protected:
  void draw() FL_OVERRIDE;

public:
  int handle(int) FL_OVERRIDE;
  Fl_Value_Output(int x,int y,int w,int h,const char *l=0);

  /**
    If "soft" is turned on, the user is allowed to drag the value outside
    the range.  If they drag the value to one of the ends, let go, then
    grab again and continue to drag, they can get to any value.  Default is
    one.
  */
  void soft(uchar s) {soft_ = s;}
  /**
    If "soft" is turned on, the user is allowed to drag the value outside
    the range.  If they drag the value to one of the ends, let go, then
    grab again and continue to drag, they can get to any value.  Default is
    one.
  */
  uchar soft() const {return soft_;}

  /**    Gets the typeface of the text in the value box.  */
  Fl_Font textfont() const {return textfont_;}
  /**    Sets the typeface of the text in the value box.  */
  void textfont(Fl_Font s) {textfont_ = s;}
  /**    Gets the size of the text in the value box.  */
  Fl_Fontsize textsize() const {return textsize_;}
  void textsize(Fl_Fontsize s) {textsize_ = s;}
  /**    Sets the color of the text in the value box.  */
  Fl_Color textcolor() const {return textcolor_;}
  /**    Gets the color of the text in the value box.  */
  void textcolor(Fl_Color s) {textcolor_ = s;}
};

#endif
