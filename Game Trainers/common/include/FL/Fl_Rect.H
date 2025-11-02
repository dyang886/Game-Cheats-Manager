//
// Fl_Rect header file for the Fast Light Tool Kit (FLTK).
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

#ifndef Fl_Rect_H
#define Fl_Rect_H

#include <FL/Fl_Widget.H>       // for c'tor based on Fl_Widget

/**
  Rectangle with standard FLTK coordinates (X, Y, W, H).

  This may be used internally, for overloaded widget constructors and other
  overloaded methods like fl_measure(), fl_text_extents(), fl_rect(),
  fl_rectf(), and maybe more.
*/

class FL_EXPORT Fl_Rect {

  int x_;
  int y_;
  int w_;
  int h_;

public:

  /** The default constructor creates an empty rectangle (x = y = w = h = 0). */
  Fl_Rect()
          : x_(0), y_(0), w_(0), h_(0) {}

  /** This constructor creates a rectangle with x = y = 0 and
    the given width and height. */
  Fl_Rect(int W, int H)
          : x_(0), y_(0), w_(W), h_(H) {}

  /** This constructor creates a rectangle with the given x,y coordinates
    and the given width and height. */
  Fl_Rect(int X, int Y, int W, int H)
          : x_(X), y_(Y), w_(W), h_(H) {}

  /** This constructor creates a rectangle with the given x,y coordinates
    and the given width and height reduced by the box frame size.

    This is the same as using the constructor w/o \p bt and subsequently
    calling inset(\p bt).
  */
  Fl_Rect(int X, int Y, int W, int H, Fl_Boxtype bt)
          : x_(X), y_(Y), w_(W), h_(H) {
    inset(bt);
  }

  /** This constructor creates a rectangle based on a widget's position and size. */
  Fl_Rect (const Fl_Widget& widget)
          : x_(widget.x()), y_(widget.y()), w_(widget.w()), h_(widget.h()) {}

  /** This constructor creates a rectangle based on a widget's position and size. */
  Fl_Rect (const Fl_Widget* const widget)
          : x_(widget->x()), y_(widget->y()), w_(widget->w()), h_(widget->h()) {}

  int x() const { return x_; }          ///< gets the x coordinate (left edge)
  int y() const { return y_; }          ///< gets the y coordinate (top edge)
  int w() const { return w_; }          ///< gets the width
  int h() const { return h_; }          ///< gets the height

  /** gets the right edge (x + w).
    \note r() and b() are coordinates \b outside the area of the rectangle.
  */
  int r() const { return x_ + w_; }
  /** gets the bottom edge (y + h).
    \note r() and b() are coordinates \b outside the area of the rectangle.
  */
  int b() const { return y_ + h_; }

  void x(int X) { x_ = X; }             ///< sets the x coordinate (left edge)
  void y(int Y) { y_ = Y; }             ///< sets the y coordinate (top edge)
  void w(int W) { w_ = W; }             ///< sets the width
  void h(int H) { h_ = H; }             ///< sets the height

  void r(int R) { w_ = R - x_; }        ///< sets the width based on R and x
  void b(int B) { h_ = B - y_; }        ///< sets the height based on B and y

  /** Move all edges in by \p d.

    Shrinks the rectangle by \p d at all sides keeping the center of the
    rectangle at the same spot.

    If \p d is negative, the rectangle is enlarged.

    If \p d \>= w() or h() the result is undefined, i.e. an
    invalid or empty rectangle.
  */
  void inset(int d) {
    x_ += d;
    y_ += d;
    w_ -= 2 * d;
    h_ -= 2 * d;
  }

  /** Move all edges in by the frame size of box type \p bt.

    Shrinks the rectangle at all sides by the frame width or height of the
    given box type \p bt.

    This method uses the frame sizes given by the box type \p bt using
    - Fl::box_dx(bt)
    - Fl::box_dy(bt)
    - Fl::box_dw(bt)
    - Fl::box_dh(bt)

    If the rectangle is smaller than the frame sizes the result is undefined,
    i.e. an invalid or empty rectangle.
  */
  void inset(Fl_Boxtype bt) {
    x_ += Fl::box_dx(bt);
    y_ += Fl::box_dy(bt);
    w_ -= Fl::box_dw(bt);
    h_ -= Fl::box_dh(bt);
  }

  /** Move all edges in by \p left, \p top, \p right, \p bottom.

    Shrinks the rectangle on all sides keeping the center of the
    rectangle at the same spot.

    If any value is negative, the rectangle is enlarged.

    Values are not range checked; it is possible to create
    an invalid or empty rectangle.
  */
  void inset(int left, int top, int right, int bottom) {
    x_ += left;
    y_ += top;
    w_ -= (left + right);
    h_ -= (top + bottom);
  }

  friend bool operator==(const Fl_Rect& lhs, const Fl_Rect& rhs) {
    return (lhs.x_==rhs.x_) && (lhs.y_==rhs.y_) && (lhs.w_==rhs.w_) && (lhs.h_==rhs.h_);
  }

  friend bool operator!=(const Fl_Rect& lhs, const Fl_Rect& rhs) {
    return !(lhs==rhs);
  }

}; // class Fl_Rect

#endif // Fl_Rect_H
