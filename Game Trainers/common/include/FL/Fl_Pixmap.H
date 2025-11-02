//
// Pixmap header file for the Fast Light Tool Kit (FLTK).
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
   Fl_Pixmap widget . */

#ifndef Fl_Pixmap_H
#define Fl_Pixmap_H
#  include "Fl_Image.H"

class Fl_Widget;
struct Fl_Menu_Item;

// Older C++ compilers don't support the explicit keyword... :(
#  if defined(__sgi) && !defined(_COMPILER_VERSION)
#    define explicit
#  endif // __sgi && !_COMPILER_VERSION

/**
  The Fl_Pixmap class supports caching and drawing of colormap
  (pixmap) images, including transparency.
*/
class FL_EXPORT Fl_Pixmap : public Fl_Image {
  friend class Fl_Graphics_Driver;
  void copy_data();
  void delete_data();
  void set_data(const char * const *p);

protected:
  void measure();

public:

  int alloc_data; // Non-zero if data was allocated

private:
  // for internal use
  fl_uintptr_t id_;
  fl_uintptr_t mask_;
  int cache_w_, cache_h_; // size of pixmap when cached

public:

  /**    The constructors create a new pixmap from the specified XPM data.  */
  explicit Fl_Pixmap(char * const * D) : Fl_Image(-1,0,1), alloc_data(0), id_(0), mask_(0) {set_data((const char*const*)D); measure();}
  /**    The constructors create a new pixmap from the specified XPM data.  */
  explicit Fl_Pixmap(uchar* const * D) : Fl_Image(-1,0,1), alloc_data(0), id_(0), mask_(0) {set_data((const char*const*)D); measure();}
  /**    The constructors create a new pixmap from the specified XPM data.  */
  explicit Fl_Pixmap(const char * const * D) : Fl_Image(-1,0,1), alloc_data(0), id_(0), mask_(0) {set_data((const char*const*)D); measure();}
  /**    The constructors create a new pixmap from the specified XPM data.  */
  explicit Fl_Pixmap(const uchar* const * D) : Fl_Image(-1,0,1), alloc_data(0), id_(0), mask_(0) {set_data((const char*const*)D); measure();}
  virtual ~Fl_Pixmap();
  Fl_Image *copy(int W, int H) const FL_OVERRIDE;
  Fl_Image *copy() const { return Fl_Image::copy(); }
  void color_average(Fl_Color c, float i) FL_OVERRIDE;
  void desaturate() FL_OVERRIDE;
  void draw(int X, int Y, int W, int H, int cx=0, int cy=0) FL_OVERRIDE;
  void draw(int X, int Y) {draw(X, Y, w(), h(), 0, 0);}
  void label(Fl_Widget*w) FL_OVERRIDE;
  void label(Fl_Menu_Item*m) FL_OVERRIDE;
  void uncache() FL_OVERRIDE;
  int cache_w() {return cache_w_;}
  int cache_h() {return cache_h_;}
};

#endif
