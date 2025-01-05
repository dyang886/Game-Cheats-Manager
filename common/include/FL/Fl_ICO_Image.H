//
// ICO image header file for the Fast Light Tool Kit (FLTK).
//
// Copyright 2022-2023 by Bill Spitzak and others.
//
// This library is free software. Distribution and use rights are outlined in
// the file "COPYING" which should have been included with this file.  If this
// file is missing or damaged, see the license at:
//
//     http://www.fltk.org/COPYING.php
//
// Please report all bugs and problems on the following page:
//
//     http://www.fltk.org/str.php
//

// https://en.wikipedia.org/wiki/ICO_(file_format)
// http://www.daubnet.com/en/file-format-ico

#ifndef Fl_ICO_Image_H
#  define Fl_ICO_Image_H
#  include "Fl_BMP_Image.H"

/**
 The Fl_ICO_Image class supports loading, caching, and drawing of Windows icon (.ico) files.
 */
class FL_EXPORT Fl_ICO_Image : public Fl_BMP_Image {

public:
  /** Windows ICONDIRENTRY structure  */
   struct IconDirEntry {
    int bWidth;         ///<  Image width
    int bHeight;        ///<  Image height
    int bColorCount;    ///<  Number of colors (0 if  ≥ 8bpp)
    int bReserved;      ///<  Reserved
    int wPlanes;        ///<  Color Planes
    int wBitCount;      ///<  Bits per pixel
    int dwBytesInRes;   ///<  Resource size in bytes
    int dwImageOffset;  ///<  Offset to the image
  };

  Fl_ICO_Image(const char *filename, int id = -1, const unsigned char *data = NULL, const size_t datasize = 0);
  ~Fl_ICO_Image();

  /** Returns the number of icons of various resolutions present in the ICO object. */
  int idcount() const { return idcount_; }

  /** Returns the array of idcount() loaded IconDirEntry structures. */
  const IconDirEntry * icondirentry() const { return icondirentry_; }

private:
  int idcount_;
  struct IconDirEntry *icondirentry_;
  void load_ico_(class Fl_Image_Reader &rdr, int id);
};

#endif // Fl_ICO_Image_H
