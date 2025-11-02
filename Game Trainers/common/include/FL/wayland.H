//
// Wayland/X11 hybrid platform header file for the Fast Light Tool Kit (FLTK).
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

#if !defined(FL_PLATFORM_H)
#  error "Never use <FL/wayland.H> directly; include <FL/platform.H> instead."
#endif // !FL_PLATFORM_H

/** \file
 \anchor wayland_H Definitions of functions specific to the Wayland platform.
*/

// ***********  for Wayland component ***********

typedef struct _cairo cairo_t;

/** Returns the Wayland display in use */
extern FL_EXPORT struct wl_display *fl_wl_display();
/** Returns the wl_surface associated to a shown window */
extern FL_EXPORT struct wl_surface *fl_wl_surface(struct wld_window *xid);
/** Returns a platform-specific reference associated to a shown window */
extern FL_EXPORT struct wld_window *fl_wl_xid(const Fl_Window *win);
/** Returns the Fl_Window corresponding to a given the platform-specific window reference */
extern FL_EXPORT Fl_Window *fl_wl_find(struct wld_window *);
/** Returns the cairo context associated to the current window or Fl_Image_Surface */
extern FL_EXPORT cairo_t *fl_wl_gc();
/** Returns the wl_compositor of the current Wayland session.
 This allows, for example, to create a wl_surface with
 \code
 struct wl_surface *my_wl_surface = wl_compositor_create_surface(fl_wl_compositor());
 \endcode
 */
extern FL_EXPORT struct wl_compositor *fl_wl_compositor();
/** Returns the current buffer scaling factor for \p window. */
extern FL_EXPORT int fl_wl_buffer_scale(Fl_Window *window);
typedef void *EGLContext;
/** Returns the EGLContext corresponding to the given GLContext */
extern FL_EXPORT EGLContext fl_wl_glcontext(GLContext rc);

#ifndef FL_DOXYGEN

#  ifdef FLTK_USE_X11
//  *********** for X11 component ***********
#    include "x11.H"
#  else
      typedef struct wld_window *Window;
#  endif

#endif // FL_DOXYGEN
