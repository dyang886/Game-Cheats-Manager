//
// XPM image header file for the Fast Light Tool Kit (FLTK).
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

   Fl_XPM_Image class . */

#ifndef Fl_XPM_Image_H
#define Fl_XPM_Image_H
#  include "Fl_Pixmap.H"

/**
  The Fl_XPM_Image class supports loading, caching,
  and drawing of X Pixmap (XPM) images, including transparency.
*/
class FL_EXPORT Fl_XPM_Image : public Fl_Pixmap {

  public:

  Fl_XPM_Image(const char* filename);
};

#endif // !Fl_XPM_Image
