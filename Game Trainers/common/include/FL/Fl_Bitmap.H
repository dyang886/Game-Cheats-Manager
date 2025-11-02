//
// Bitmap header file for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2017 by Bill Spitzak and others.
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
   Fl_Bitmap widget . */

#ifndef Fl_Bitmap_H
#define Fl_Bitmap_H
#include "Fl_Image.H"
#include "Fl_Widget.H" // for fl_uintptr_t

class Fl_Widget;
struct Fl_Menu_Item;

/**
  The Fl_Bitmap class supports caching and drawing of mono-color
  (bitmap) images. Images are drawn using the current color.
*/
class FL_EXPORT Fl_Bitmap : public Fl_Image {
  friend class Fl_Graphics_Driver;
public:
  /** pointer to raw bitmap data */
  const uchar *array;
  /** Non-zero if array points to bitmap data allocated internally */
  int alloc_array;

private:
  /** for internal use */
  fl_uintptr_t id_;
  int cache_w_, cache_h_; // size of bitmap when cached

public:
  /** The constructors create a new bitmap from the specified bitmap data.
   \see Fl_Bitmap(const uchar *bits, int bits_length, int W, int H) */
  Fl_Bitmap(const uchar *bits, int W, int H) :
    Fl_Image(W,H,0), array(bits), alloc_array(0), id_(0), cache_w_(0),cache_h_(0) {data((const char **)&array, 1);}
  /** The constructors create a new bitmap from the specified bitmap data.
   \see Fl_Bitmap(const char *bits, int bits_length, int W, int H) */
  Fl_Bitmap(const char *bits, int W, int H) :
    Fl_Image(W,H,0), array((const uchar *)bits), alloc_array(0), id_(0), cache_w_(0),cache_h_(0) {data((const char **)&array, 1);}
  Fl_Bitmap(const uchar *bits, int bits_length, int W, int H);
  Fl_Bitmap(const char *bits, int bits_length, int W, int H);
  virtual ~Fl_Bitmap();
  Fl_Image *copy(int W, int H) const FL_OVERRIDE;
  Fl_Image *copy() const { return Fl_Image::copy(); }
  void draw(int X, int Y, int W, int H, int cx=0, int cy=0) FL_OVERRIDE;
  void draw(int X, int Y) {draw(X, Y, w(), h(), 0, 0);}
  void label(Fl_Widget*w) FL_OVERRIDE;
  void label(Fl_Menu_Item*m) FL_OVERRIDE;
  void uncache() FL_OVERRIDE;
  int cache_w() {return cache_w_;}
  int cache_h() {return cache_h_;}
};

#endif
