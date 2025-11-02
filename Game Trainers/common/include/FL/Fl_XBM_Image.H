//
// XBM image header file for the Fast Light Tool Kit (FLTK).
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
   Fl_XBM_Image class . */

#ifndef Fl_XBM_Image_H
#define Fl_XBM_Image_H
#  include "Fl_Bitmap.H"

/**
  The Fl_XBM_Image class supports loading, caching,
  and drawing of X Bitmap (XBM) bitmap files.
*/
class FL_EXPORT Fl_XBM_Image : public Fl_Bitmap {

  public:

  Fl_XBM_Image(const char* filename);
};

#endif // !Fl_XBM_Image_H
