//
// PNG image header file for the Fast Light Tool Kit (FLTK).
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

/* \file
   Fl_PNG_Image class . */

#ifndef Fl_PNG_Image_H
#define Fl_PNG_Image_H
#  include "Fl_Image.H"

/**
  The Fl_PNG_Image class supports loading, caching,
  and drawing of Portable Network Graphics (PNG) image files. The
  class loads color-mapped and full-color images and handles color-
  and alpha-based transparency.
*/
class FL_EXPORT Fl_PNG_Image : public Fl_RGB_Image {
  friend class Fl_ICO_Image;
public:

  Fl_PNG_Image(const char* filename);
  Fl_PNG_Image (const char *name_png, const unsigned char *buffer, int datasize);
private:
  Fl_PNG_Image(const char *filename, int offset); // used by Fl_ICO_Image
  void load_png_(const char *name_png, int offset, const unsigned char *buffer_png, int datasize);
};

// Support functions to write PNG image files (since 1.4.0)

FL_EXPORT int fl_write_png(const char *filename, Fl_RGB_Image *img);
FL_EXPORT int fl_write_png(const char *filename, const char *pixels, int w, int h, int d=3, int ld=0);
FL_EXPORT int fl_write_png(const char *filename, const unsigned char *pixels, int w, int h, int d=3, int ld=0);

#endif
