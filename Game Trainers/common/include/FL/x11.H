//
// X11 platform header file for the Fast Light Tool Kit (FLTK).
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

// Do not directly include this file, instead use <FL/platform.H>.  It will
// include this file if FLTK_USE_X11 is defined.  This is to encourage
// portability of even the system-specific code...

#ifdef FL_DOXYGEN

/** \file
 Definitions of functions specific to the X11 platform.
*/

/** Returns the X11 Display in use */
extern Display *fl_x11_display();
/** Have FLTK use a pre-established X11 connection.
 This function must be called before FLTK attempts to open its own X11 connection,
 that is, as long as fl_x11_display() returns NULL.
 \param d the X11 Display* value representing a valid, pre-established X11 connection
 */
extern void fl_x11_use_display(Display *d);
/** Returns the Window reference for the given Fl_Window, or zero if not \c shown().  */
extern Window fl_x11_xid(const Fl_Window *win);
/** Returns the Fl_Window corresponding to the given Window reference.  */
extern Fl_Window *fl_x11_find(Window xid);
/** Returns the X11-specific currently active graphics context.  */
extern GC fl_x11_gc();
/** Returns the Cairo-specific currently active graphics context (FLTK_GRAPHICS_CAIRO=On) */
extern cairo_t* fl_cairo_gc();
#else // ! FL_DOXYGEN

#ifndef FL_PLATFORM_H
#  error "Never use <FL/x11.H> directly; include <FL/platform.H> instead."
#endif // !FL_PLATFORM_H

#include <FL/Enumerations.H>

#if defined(_ABIN32) || defined(_ABI64) // fix for broken SGI Irix X .h files
#  pragma set woff 3322
#endif
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#if defined(_ABIN32) || defined(_ABI64)
#  pragma reset woff 3322
#endif
#include <X11/Xatom.h>

typedef struct __GLXcontextRec *GLXContext;
extern GLXContext fl_x11_glcontext(GLContext rc);

// constant info about the X server connection:
extern FL_EXPORT Display *fl_display;
extern FL_EXPORT Display *fl_x11_display();
extern FL_EXPORT void fl_x11_use_display(Display *);
extern FL_EXPORT Window fl_x11_xid(const Fl_Window *win);
extern FL_EXPORT Fl_Window *fl_x11_find(Window);
extern FL_EXPORT int fl_screen;
extern FL_EXPORT XVisualInfo *fl_visual;
extern FL_EXPORT Colormap fl_colormap;

// drawing functions:
extern FL_EXPORT GC fl_gc;
#if FLTK_USE_CAIRO
  typedef struct _cairo cairo_t;
  extern FL_EXPORT cairo_t* fl_cairo_gc();
#endif
extern FL_EXPORT GC fl_x11_gc();
FL_EXPORT ulong fl_xpixel(Fl_Color i);
FL_EXPORT ulong fl_xpixel(uchar r, uchar g, uchar b);

// feed events into fltk:
FL_EXPORT int fl_handle(const XEvent&);

// you can use these in Fl::add_handler() to look at events:
extern FL_EXPORT const XEvent* fl_xevent;
extern FL_EXPORT ulong fl_event_time;

#if defined(FL_LIBRARY) || defined(FL_INTERNALS)
extern FL_EXPORT Window fl_message_window;
extern FL_EXPORT void *fl_xftfont;

// access to core fonts:
// This class provides a "smart pointer" that returns a pointer to an XFontStruct.
// The global variable fl_xfont can be called wherever a bitmap "core" font is
// needed, e.g. when rendering to a GL context under X11.
// With Xlib / X11 fonts, fl_xfont will return the current selected font.
// With XFT / X11 fonts, fl_xfont will attempt to return the bitmap "core" font most
// similar to (usually the same as) the current XFT font.
class FL_EXPORT Fl_XFont_On_Demand
{
public:
  Fl_XFont_On_Demand(XFontStruct* p = NULL) : ptr(p) { }
  Fl_XFont_On_Demand& operator=(const Fl_XFont_On_Demand& x)
  { ptr = x.ptr;  return *this; }
  Fl_XFont_On_Demand& operator=(XFontStruct* p)
  { ptr = p;  return *this; }
  XFontStruct* value();
  operator XFontStruct*() { return value(); }
  XFontStruct& operator*() { return *value(); }
  XFontStruct* operator->() { return value(); }
  bool operator==(const Fl_XFont_On_Demand& x) { return ptr == x.ptr; }
  bool operator!=(const Fl_XFont_On_Demand& x) { return ptr != x.ptr; }
private:
  XFontStruct *ptr;
};
extern FL_EXPORT Fl_XFont_On_Demand fl_xfont;

extern FL_EXPORT char fl_override_redirect; // hack into Fl_X::make_xid()

#endif // FL_LIBRARY || FL_INTERNALS

#endif // FL_DOXYGEN
