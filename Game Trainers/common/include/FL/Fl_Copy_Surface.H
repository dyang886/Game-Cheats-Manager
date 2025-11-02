//
// Copy-to-clipboard code for the Fast Light Tool Kit (FLTK).
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

#ifndef Fl_Copy_Surface_H
#define Fl_Copy_Surface_H

#include <FL/Fl_Widget_Surface.H>

/** Supports copying of graphical data to the clipboard.

 <br> After creation of an Fl_Copy_Surface object, make it the current drawing surface calling Fl_Surface_Device::push_current(), and all subsequent graphics requests
 will be recorded in the clipboard. It's possible to draw widgets (using Fl_Copy_Surface::draw()
 ) or to use any of the \ref fl_drawings or the \ref fl_attributes.
 Finally, delete the Fl_Copy_Surface object to load the clipboard with the graphical data.
 <br> Fl_Gl_Window 's can be copied to the clipboard as well.
 <br> Usage example:
 \code
 Fl_Widget *g = ...; // a widget you want to copy to the clipboard
 Fl_Copy_Surface *copy_surf = new Fl_Copy_Surface(g->w(), g->h()); // create an Fl_Copy_Surface object
 Fl_Surface_Device::push_current(copy_surf); // direct graphics requests to the clipboard
 fl_color(FL_WHITE); fl_rectf(0, 0, g->w(), g->h()); // draw a white background
 copy_surf->draw(g); // draw the g widget in the clipboard
 Fl_Surface_Device::pop_current();  // direct graphics requests back to their previous destination
 delete copy_surf; // after this, the clipboard is loaded
 \endcode
 Platform details:
 \li Windows: Transparent RGB images copy without transparency.
 The graphical data are copied to the clipboard in two formats: 1) as an 'enhanced metafile';
 2) as a color bitmap. Applications to which the clipboard content is pasted can use the format
 that suits them best.
 \li Mac OS: The graphical data are copied to the clipboard (a.k.a. pasteboard) in two 'flavors':
 1) in vectorial form as PDF data; 2) in bitmap form as a TIFF image.
 Applications to which the clipboard content is pasted can use the flavor that suits them best.
 \li X11 and Wayland: the graphical data are copied to the clipboard as an image in BMP format.
*/
class FL_EXPORT Fl_Copy_Surface : public Fl_Widget_Surface {
private:
  class Fl_Copy_Surface_Driver *platform_surface;
protected:
  void translate(int x, int y) FL_OVERRIDE;
  void untranslate() FL_OVERRIDE;
public:
  Fl_Copy_Surface(int w, int h);
  ~Fl_Copy_Surface();
  void set_current() FL_OVERRIDE;
  bool is_current() FL_OVERRIDE;
  /** Returns the pixel width of the copy surface */
  int w();
  /** Returns the pixel height of the copy surface */
  int h();
  void origin(int *x, int *y) FL_OVERRIDE;
  void origin(int x, int y) FL_OVERRIDE;
  int printable_rect(int *w, int *h) FL_OVERRIDE;
};


/**
 \cond DriverDev
 \addtogroup DriverDeveloper
 \{
 */

/**
 A base class describing the interface between FLTK and draw-to-clipboard operations.

 This class is only for internal use by the FLTK library.

 A supported platform should implement the virtual methods of this class
 in order to support drawing to the clipboard through class Fl_Copy_Surface.
 */
class Fl_Copy_Surface_Driver : public Fl_Widget_Surface {
  friend class Fl_Copy_Surface;
protected:
  int width;
  int height;
  Fl_Copy_Surface_Driver(int w, int h) : Fl_Widget_Surface(NULL), width(w), height(h) {}
  virtual ~Fl_Copy_Surface_Driver() {}
  void set_current() FL_OVERRIDE = 0;
  void translate(int x, int y) FL_OVERRIDE = 0;
  void untranslate() FL_OVERRIDE = 0;
  int printable_rect(int *w, int *h) FL_OVERRIDE;
  /** Each platform implements this function its own way.
   It returns an object implementing all virtual functions
   of class Fl_Copy_Surface_Driver for the platform.
   */
  static Fl_Copy_Surface_Driver *newCopySurfaceDriver(int w, int h);
};

/**
 \}
 \endcond
 */

#endif // Fl_Copy_Surface_H
