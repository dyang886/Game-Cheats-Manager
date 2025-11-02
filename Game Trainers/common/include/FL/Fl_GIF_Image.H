//
// GIF image header file for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2024 by Bill Spitzak and others.
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
   Fl_GIF_Image widget . */

#ifndef Fl_GIF_Image_H
#define Fl_GIF_Image_H
#  include "Fl_Pixmap.H"

/**
 The Fl_GIF_Image class supports loading, caching,
 and drawing of Compuserve GIF<SUP>SM</SUP> images. The class
 loads the first image and supports transparency.
 */
class FL_EXPORT Fl_GIF_Image : public Fl_Pixmap {

public:

  Fl_GIF_Image(const char* filename);
  // deprecated constructor w/o length (for backwards compatibility)
  Fl_GIF_Image(const char* imagename, const unsigned char *data);
  // constructor with length (since 1.4.0)
  Fl_GIF_Image(const char* imagename, const unsigned char *data, const size_t length);

  static bool is_animated(const char *name_);
  /** Sets how the shared image core routine should treat animated GIF files.
   The default is to treat them as ordinary GIF's e.g. it creates a Fl_GIF_Image object.
   If this variable is set, then an animated GIF object Fl_Anim_GIF_Image is created.
   */
  static bool animate;

protected:

  // Protected constructors needed for animated GIF support through Fl_Anim_GIF_Image.
  Fl_GIF_Image(const char* filename, bool anim);
  Fl_GIF_Image(const char* imagename, const unsigned char *data, const size_t length, bool anim);
  // Protected default constructor needed for Fl_Anim_GIF_Image.
  Fl_GIF_Image();

  void load_gif_(class Fl_Image_Reader &rdr, bool anim=false);

  void load(const char* filename, bool anim);
  void load(const char* imagename, const unsigned char *data, const size_t length, bool anim);

  // Internal structure to "glue" animated GIF support into Fl_GIF_Image.
  // This data is passed during decoding to the Fl_Anim_GIF_Image class.
  struct GIF_FRAME {
    int ifrm, width, height, x, y, w, h,
        clrs, bkgd, trans,
        dispose, delay;
    const uchar *bptr;
    const struct CPAL {
      uchar r, g, b;
    } *cpal;
    GIF_FRAME(int frame, uchar *data) : ifrm(frame), bptr(data) {}
    GIF_FRAME(int frame, int W, int H, int fx, int fy, int fw, int fh, uchar *data) :
      ifrm(frame), width(W), height(H), x(fx), y(fy), w(fw), h(fh), bptr(data) {}
    void disposal(int mode, int time) { dispose = mode; this->delay = time; }
    void colors(int nclrs, int bg, int tp) { clrs = nclrs; bkgd = bg; trans = tp; }
  };

  // Internal virtual methods, which are called during decoding to pass data
  // to the Fl_Anim_GIF_Image class.
  virtual void on_frame_data(GIF_FRAME &) {}
  virtual void on_extension_data(GIF_FRAME &) {}

private:

  void lzw_decode(Fl_Image_Reader &rdr, uchar *Image, int Width, int Height, int CodeSize, int ColorMapSize, int Interlace);
};

#endif
