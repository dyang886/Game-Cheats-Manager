//
// PNM image header file for the Fast Light Tool Kit (FLTK).
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
   Fl_PNM_Image class . */

#ifndef Fl_PNM_Image_H
#define Fl_PNM_Image_H
#  include "Fl_Image.H"

/**
  The Fl_PNM_Image class supports loading, caching,
  and drawing of Portable Anymap (PNM, PBM, PGM, PPM) image files. The class
  loads bitmap, grayscale, and full-color images in both ASCII and
  binary formats.
*/
class FL_EXPORT Fl_PNM_Image : public Fl_RGB_Image {

  public:

  Fl_PNM_Image(const char* filename);
};

#endif
