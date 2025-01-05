//
// Draw-to-image code for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2016 by Bill Spitzak and others.
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

#ifndef Fl_Image_Surface_H
#define Fl_Image_Surface_H

#include <FL/Fl_Widget_Surface.H>
#include <FL/Fl_Image.H>
#include <FL/Fl_Shared_Image.H>
#include <FL/platform_types.h> // for Fl_Offscreen


/**
 \brief Directs all graphics requests to an Fl_Image.

 After creation of an Fl_Image_Surface object, make it the current drawing
 surface calling Fl_Surface_Device::push_current(), and all
 subsequent graphics requests will be recorded in the image. It's possible to
 draw widgets (using Fl_Image_Surface::draw()) or to use any of the
 \ref fl_drawings or the \ref fl_attributes. Finally, call image() on the object
 to obtain a newly allocated Fl_RGB_Image object.
 Fl_Gl_Window objects can be drawn in the image as well.

 Usage example:
 \code
 // this is the widget that you want to draw into an image
 Fl_Widget *g = ...;

 // create an Fl_Image_Surface object
 Fl_Image_Surface *image_surface = new Fl_Image_Surface(g->w(), g->h());

 // direct all further graphics requests to the image
 Fl_Surface_Device::push_current(image_surface);

 // draw a white background
 fl_color(FL_WHITE);
 fl_rectf(0, 0, g->w(), g->h());

 // draw the g widget in the image
 image_surface->draw(g);

 // get the resulting image
 Fl_RGB_Image* image = image_surface->image();

 // direct graphics requests back to their previous destination
 Fl_Surface_Device::pop_current();

 // delete the image_surface object, but not the image itself
 delete image_surface;
 \endcode
*/
class FL_EXPORT Fl_Image_Surface : public Fl_Widget_Surface {
  friend class Fl_Graphics_Driver;
private:
  class Fl_Image_Surface_Driver *platform_surface;
  Fl_Offscreen get_offscreen_before_delete_();
protected:
  void translate(int x, int y) FL_OVERRIDE;
  void untranslate() FL_OVERRIDE;
public:
  Fl_Image_Surface(int w, int h, int high_res = 0, Fl_Offscreen off = 0);
  ~Fl_Image_Surface();
  void set_current() FL_OVERRIDE;
  bool is_current() FL_OVERRIDE;
  Fl_RGB_Image *image();
  Fl_Shared_Image *highres_image();
  void origin(int *x, int *y) FL_OVERRIDE;
  void origin(int x, int y) FL_OVERRIDE;
  int printable_rect(int *w, int *h) FL_OVERRIDE;
  Fl_Offscreen offscreen();
  void rescale();
  void mask(const Fl_RGB_Image *);
};


/**
 \cond DriverDev
 \addtogroup DriverDeveloper
 \{
 */

/** A base class describing the interface between FLTK and draw-to-image operations.
 This class is only for internal use by the FLTK library.
 A supported platform should implement the virtual methods of this class
 in order to support drawing to an Fl_RGB_Image through class Fl_Image_Surface.
 */
class Fl_Image_Surface_Driver : public Fl_Widget_Surface {
  friend class Fl_Image_Surface;
private:
  Fl_Image_Surface *image_surface_;
protected:
  int width;
  int height;
  Fl_Offscreen offscreen;
  int external_offscreen;
  Fl_Image_Surface_Driver(int w, int h, int /*high_res*/, Fl_Offscreen off) : Fl_Widget_Surface(NULL), width(w), height(h), offscreen(off) {external_offscreen = (off != 0);}
  virtual ~Fl_Image_Surface_Driver() {}
  static void copy_with_mask(Fl_RGB_Image* mask, uchar *dib_dst, uchar *dib_src,
                             int line_size, bool bottom_to_top);
  static Fl_RGB_Image *RGB3_to_RGB1(const Fl_RGB_Image *rgb3, int W, int H);
  void set_current() FL_OVERRIDE = 0;
  void translate(int x, int y) FL_OVERRIDE = 0;
  void untranslate() FL_OVERRIDE = 0;
  int printable_rect(int *w, int *h) FL_OVERRIDE;
  virtual Fl_RGB_Image *image() = 0;
  virtual void mask(const Fl_RGB_Image *) {}
  /** Each platform implements this function its own way.
   It returns an object implementing all virtual functions
   of class Fl_Image_Surface_Driver for the plaform.
   */
  static Fl_Image_Surface_Driver *newImageSurfaceDriver(int w, int h, int high_res, Fl_Offscreen off);
public:
  /** Returns pointer to the associated Fl_Image_Surface object */
  Fl_Image_Surface *image_surface() { return image_surface_; }
};

/**
 \}
 \endcond
 */

#endif // Fl_Image_Surface_H
