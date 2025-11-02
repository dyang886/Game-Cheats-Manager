//
// BMP image header file for the Fast Light Tool Kit (FLTK).
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
   Fl_BMP_Image widget . */

#ifndef Fl_BMP_Image_H
#define Fl_BMP_Image_H
#  include "Fl_Image.H"

/**
 The Fl_BMP_Image class supports loading, caching,
 and drawing of Windows Bitmap (BMP) image files.
 */
class FL_EXPORT Fl_BMP_Image : public Fl_RGB_Image {

public:

  Fl_BMP_Image(const char* filename);
  Fl_BMP_Image(const char* imagename, const unsigned char *data, const long length = -1);

protected:

  void load_bmp_(class Fl_Image_Reader &rdr, int ico_height = 0, int ico_width = 0);

};

#endif
