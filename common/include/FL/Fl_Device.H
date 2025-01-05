//
// Definition of classes Fl_Surface_Device, Fl_Display_Device
// for the Fast Light Tool Kit (FLTK).
//
// Copyright 2010-2021 by Bill Spitzak and others.
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

/** \file Fl_Device.H
 \brief declaration of classes Fl_Surface_Device, Fl_Display_Device, Fl_Device_Plugin.
*/

#ifndef Fl_Device_H
#define Fl_Device_H

#include <FL/Fl_Plugin.H>
#include <FL/platform_types.h>

class Fl_Graphics_Driver;
class Fl_RGB_Image;
class Fl_Widget;
class Fl_Image_Surface;

/**
 A drawing surface that's susceptible to receive graphical output.
 Any FLTK application has at any time a current drawing surface to which all drawing requests are directed.
 The current surface is given by Fl_Surface_Device::surface().
 When main() begins running, the current drawing surface has been set to the computer's display,
 an instance of the Fl_Display_Device class.

 A drawing surface other than the computer's display, is typically used as follows:
 <ol><li> Create \c surface, an object from an Fl_Surface_Device derived class (e.g., Fl_Image_Surface, Fl_Printer, Fl_Copy_Surface).
 <li> Call \c Fl_Surface_Device::push_current(surface); to redirect all graphics requests to
 \c surface which becomes the new current drawing surface (not necessary with classes Fl_Printer / Fl_PostScript_File_Device /
 Fl_PDF_File_Surface because it is done by Fl_Paged_Device::begin_page()).
 <li> At this point all of the \ref fl_drawings (e.g., fl_rect()) or the \ref fl_attributes or \ref drawing_images functions
 (e.g., fl_draw_image(), Fl_Image::draw()) operate on the new current drawing surface.
 It's also possible to draw to the current surface any widget with Fl_Widget_Surface::draw(Fl_Widget*, int, int), a window and its
 titlebar with Fl_Widget_Surface::draw_decorated_window(), or the content of a rectangular zone of a window
 with Fl_Widget_Surface::print_window_part().
 <li> After all drawing requests have been performed, redirect graphics requests back to their previous destination
 with \c Fl_Surface_Device::pop_current(); (not necessary with classes Fl_Printer / Fl_PostScript_File_Device / Fl_PDF_File_Surface).
 <li> Delete \c surface.
 </ol>
 For back-compatibility, it is also possible to use the Fl_Surface_Device::set_current() member function
 to change the current drawing surface, once to the new surface, once to the previous one.
 */
class FL_EXPORT Fl_Surface_Device {
  /** The graphics driver in use by this surface. */
  Fl_Graphics_Driver *pGraphicsDriver;
  static Fl_Surface_Device *surface_; // the surface that currently receives graphics requests
  static Fl_Surface_Device *default_surface(); // create surface if none exists yet
protected:
  /** FLTK calls this each time a surface ceases to be the current drawing surface.
   This member function is mostly of interest to developers of new Fl_Surface_Device derived classes.
   It allows to perform surface-specific operations necessary when this surface ceases to be current.
   Each implementation should end with a call to Fl_Surface_Device::end_current(). */
  virtual void end_current() { surface_ = 0;}
  /** Constructor that sets the graphics driver to use for the created surface. */
  Fl_Surface_Device(Fl_Graphics_Driver *graphics_driver) {pGraphicsDriver = graphics_driver; }
  /** Sets the graphics driver of this drawing surface. */
  inline void driver(Fl_Graphics_Driver *graphics_driver) {pGraphicsDriver = graphics_driver;}
public:
  virtual void set_current(void);
  virtual bool is_current();
  /** \brief Returns the graphics driver of this drawing surface. */
  inline Fl_Graphics_Driver *driver() {return pGraphicsDriver; }
  /** The current drawing surface.
   In other words, the Fl_Surface_Device object that currently receives all graphics requests.
   \note It's possible to transiently remove the GUI scaling factor in place in the current
   drawing surface with \ref fl_override_scale(). */
  static inline Fl_Surface_Device *surface() {
    return surface_ ? surface_ : default_surface();
  }
  /** \brief The destructor. */
  virtual ~Fl_Surface_Device();
  static void push_current(Fl_Surface_Device *new_current);
  static Fl_Surface_Device *pop_current();
};

/**
 The computer's display.
 When FLTK begins to access the computer's display, it creates an object of class Fl_Display_Device and makes
 it the current drawing surface.
 */
class FL_EXPORT Fl_Display_Device : public Fl_Surface_Device {
  Fl_Display_Device(Fl_Graphics_Driver *graphics_driver);
public:
  static Fl_Display_Device *display_device();
};

/**
 This plugin socket allows the integration of new device drivers for special
 window or screen types.
 This class is not intended for use outside the FLTK library.
 It is currently used to provide an automated printing
 service and screen capture for OpenGL windows, if linked with fltk_gl.
 */
class Fl_Device_Plugin : public Fl_Plugin {
public:
  /** \brief The constructor */
  Fl_Device_Plugin(const char *pluginName)
  : Fl_Plugin(klass(), pluginName) { }
  /** \brief Returns the class name */
  virtual const char *klass() { return "fltk:device"; }
  /** \brief Returns the plugin name */
  virtual const char *name() = 0;
  /** \brief Prints a widget */
  virtual int print(Fl_Widget* w) = 0;
  /** Captures a rectangle of a widget as an image
   \return The captured pixels as an RGB image
   */
  virtual Fl_RGB_Image* rectangle_capture(Fl_Widget *widget, int x, int y, int w, int h) = 0;
  /** Returns the OpenGL plugin */
  static Fl_Device_Plugin *opengl_plugin();
};

#endif // Fl_Device_H
