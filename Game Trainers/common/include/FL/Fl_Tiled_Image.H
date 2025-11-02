//
// Tiled image header file for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2015 by Bill Spitzak and others.
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
   Fl_Tiled_Image widget . */

#ifndef Fl_Tiled_Image_H
#  define Fl_Tiled_Image_H

#  include "Fl_Image.H"


/**
  This class supports tiling of images over a specified area.

  The source (tile) image is \b not copied unless you call the
  color_average(), desaturate(), or inactive() methods.
*/
class FL_EXPORT Fl_Tiled_Image : public Fl_Image {
protected:

  Fl_Image      *image_;                // The image that is tiled
  int           alloc_image_;           // Did we allocate this image?

public:
  Fl_Tiled_Image(Fl_Image *i, int W = 0, int H = 0);
  virtual ~Fl_Tiled_Image();

  Fl_Image *copy(int W, int H) const FL_OVERRIDE;
  Fl_Image *copy() const {
    return Fl_Image::copy();
  }
  void color_average(Fl_Color c, float i) FL_OVERRIDE;
  void desaturate() FL_OVERRIDE;
  void draw(int X, int Y, int W, int H, int cx = 0, int cy = 0) FL_OVERRIDE;
  void draw(int X, int Y) { draw(X, Y, w(), h(), 0, 0); }
  /** Gets The image that is tiled */
  Fl_Image *image() { return image_; }
};

#endif // !Fl_Tiled_Image_H
