//
// Value Slider header file for the Fast Light Tool Kit (FLTK).
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
   Fl_Value_Slider widget . */

#ifndef Fl_Value_Slider_H
#define Fl_Value_Slider_H

#include "Fl_Slider.H"

/**
  The Fl_Value_Slider widget is a Fl_Slider widget
  with a box displaying the current value.
  \image html  value_slider.png
  \image latex value_slider.png "Fl_Value_Slider" width=4cm
*/
class FL_EXPORT Fl_Value_Slider : public Fl_Slider {
  Fl_Font textfont_;
  Fl_Fontsize textsize_;
  Fl_Color textcolor_;
  short value_width_;
  short value_height_;

protected:
  void draw() FL_OVERRIDE;

public:
  int handle(int) FL_OVERRIDE;
  Fl_Value_Slider(int x, int y, int w, int h, const char *l = 0);

  /** Gets the typeface of the text in the value box. */
  Fl_Font textfont() const { return textfont_; }

  /** Sets the typeface of the text in the value box. */
  void textfont(Fl_Font s) { textfont_ = s; }

  /** Gets the size of the text in the value box. */
  Fl_Fontsize textsize() const { return textsize_; }

  /** Sets the size of the text in the value box. */
  void textsize(Fl_Fontsize s) { textsize_ = s; }

  /** Gets the color of the text in the value box. */
  Fl_Color textcolor() const { return textcolor_; }

  /** Sets the color of the text in the value box. */
  void textcolor(Fl_Color s) { textcolor_ = s; }

  /** Sets the width of the value box in pixels (horizontal mode only).

    Limited range checking is applied but drawing errors may occur if
    the size \p s is set too high or too low, particularly if the widget
    is resized (later).

    The programmer is responsible for setting sensible values and
    widget sizes.

    The default value set by the constructor is 35.

    \param[in]  s  new width of the value box

    \since 1.4.0
  */
  void value_width(int s) {
    if (s > w() - 10)
      s = w() - 10;
    if (s < 10)
      s = 10;
    value_width_ = (short)s;
  }

  /** Gets the width of the value box in pixels (horizontal mode only).

    \since 1.4.0
  */
  int value_width() const { return (value_width_); }

  /** Sets the height of the value box in pixels (vertical mode only).

    Limited range checking is applied but drawing errors may occur if
    the size \p s is set too high or too low, particularly if the widget
    is resized (later).

    The programmer is responsible for setting sensible values and
    widget sizes.

    The default value set by the constructor is 25.

    \param[in]  s  new height of the value box

    \since 1.4.0
  */
  void value_height(int s) {
    if (s > h() - 10)
      s = h() - 10;
    if (s < 10)
      s = 10;
    value_height_ = (short)s;
  }
  /** Gets the height of the value box in pixels (vertical mode only).

    \since 1.4.0
  */
  int value_height() const { return (value_height_); }
};

#endif
