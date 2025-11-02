//
// Drivers code for the Fast Light Tool Kit (FLTK).
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

#ifndef Fl_Widget_Surface_h
#define Fl_Widget_Surface_h

#include <FL/Fl_Device.H>
#include <FL/Fl_Window.H>

/** A surface on which any FLTK widget can be drawn.
 */
class FL_EXPORT Fl_Widget_Surface : public Fl_Surface_Device {
private:
  void traverse(Fl_Widget *widget); // finds subwindows of widget and prints them
protected:
  /** \brief horizontal offset to the origin of graphics coordinates */
  int x_offset;
  /** \brief vertical offset to the origin of graphics coordinates */
  int y_offset;
  Fl_Widget_Surface(Fl_Graphics_Driver *d);
public:
  virtual void translate(int x, int y);
  virtual void untranslate();
  void draw(Fl_Widget* widget, int delta_x = 0, int delta_y = 0);
  void draw_decorated_window(Fl_Window *win, int x_offset = 0, int y_offset = 0);
  void print_window_part(Fl_Window *win, int x, int y, int w, int h, int delta_x = 0, int delta_y = 0);
  virtual int printable_rect(int *w, int *h);
  virtual void origin(int x, int y);
  virtual void origin(int *x, int *y);
};

#endif /* Fl_Widget_Surface_h */
