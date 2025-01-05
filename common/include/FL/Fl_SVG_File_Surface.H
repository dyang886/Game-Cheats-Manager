//
// Declaration of Fl_SVG_File_Surface in the Fast Light Tool Kit (FLTK).
//
// Copyright 2020 by Bill Spitzak and others.
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

#ifndef Fl_SVG_File_Surface_H
#define Fl_SVG_File_Surface_H

#include <FL/Fl_Widget_Surface.H>
#include <stdio.h>

/** A drawing surface producing a Scalable Vector Graphics (SVG) file.
 This drawing surface allows to store any FLTK graphics in vectorial form in a "Scalable Vector Graphics" file.
 \n Usage example:
 \code
   Fl_Window *win = ...// Window to draw to a .svg file
   int ww = win->decorated_w();
   int wh = win->decorated_h();
   FILE *svg = fl_fopen("/path/to/mywindow.svg", "w");
   if (svg) {
     Fl_SVG_File_Surface *surface = new Fl_SVG_File_Surface(ww, wh, svg);
     Fl_Surface_Device::push_current(surface);
     fl_color(FL_WHITE);
     fl_rectf(0, 0, ww, wh);
     surface->draw_decorated_window(win);
     Fl_Surface_Device::pop_current();
     delete surface; // the .svg file is not complete until the destructor was run
     fclose(svg);
   }
 \endcode
 \note FLTK uses the PNG and JPEG libraries to encode images to the SVG format.
 For this reason, class Fl_SVG_File_Surface is placed in the fltk_images library.
 If JPEG is not available at application build time, PNG is enough (but produces a quite larger output).
 If PNG isn't available either, images don't appear in the SVG output.
*/
class FL_EXPORT Fl_SVG_File_Surface : public Fl_Widget_Surface {
  int width_, height_;
  int (*closef_)(FILE*);
public:
  /**
  Constructor of the SVG drawing surface.
  \param width,height Width and height of the graphics area in FLTK drawing units
  \param svg A writable FILE pointer where the SVG data are to be sent. The resulting SVG data are not complete until after destruction of the Fl_SVG_File_Surface object or after calling close().
  \param closef If not NULL,  the destructor and close() will call \p closef(svg) after all
    SVG data has been sent. If NULL, \p fclose(svg) is called instead. This allows to close the FILE
    pointer by, e.g., \p pclose, or, using a function such as \p "int keep_open(FILE*){return 0;}", to keep it open after
    completion of all output to \p svg. Function \p closef should return non zero to indicate an error.
   */
  Fl_SVG_File_Surface(int width, int height, FILE *svg, int (*closef)(FILE*) = NULL);
  /**
   Destructor.
   The underlying FILE pointer is processed as by close().
   */
  ~Fl_SVG_File_Surface();
  /** Returns the underlying FILE pointer */
  FILE *file();
  void origin(int x, int y) FL_OVERRIDE;
  void origin(int *x, int *y) FL_OVERRIDE;
  void translate(int x, int y) FL_OVERRIDE;
  void untranslate() FL_OVERRIDE;
  int printable_rect(int *w, int *h) FL_OVERRIDE;
  /** Closes the FILE pointer where SVG data is output.
  The underlying FILE is closed by function fclose() unless another function was set at object's construction time.
  The only operation possible after this on the Fl_SVG_File_Surface object is its destruction.
  \return The value returned by the closing function call. */
  int close();
};

#endif /* Fl_SVG_File_Surface_H */
