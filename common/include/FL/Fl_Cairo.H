//
// Main Cairo support header file for the Fast Light Tool Kit (FLTK).
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

/** \file Fl_Cairo.H
  Cairo is currently supported for the following platforms:
    Windows, macOS, Unix/Linux (X11 + Wayland).

  \note In FLTK 1.3.x this header file (Fl_Cairo.H) included the platform
    specific Cairo headers. This is no longer true since 1.4.0.

  This header file is platform agnostic. If you need platform specific Cairo
  headers you need to \#include them in your source file.

  To use FLTK's builtin Cairo support you need to <tt>\#include \<FL/Fl.H></tt>
  \b before you include any other FLTK header which is officially required anyway.
  Since FLTK 1.4.0 the preprocessor constants \p FLTK_HAVE_CAIRO and/or
  \p FLTK_HAVE_CAIROEXT are defined in \<FL/Fl.H> by including \<FL/fl_config.h>.
*/

#ifndef FL_CAIRO_H
#define FL_CAIRO_H

#include <FL/Fl.H>

# ifdef FLTK_HAVE_CAIRO

# include <cairo.h>

/**
  \addtogroup group_cairo
  @{
*/

/**
  Contains all the necessary info on the current cairo context.
  A private internal & unique corresponding object is created to
  permit cairo context state handling while keeping it opaque.
  For internal use only.
  \note Only available when configure has the --enable-cairo or
       --enable-cairoext option or one or both of the CMake options
       FLTK_OPTION_CAIRO_WINDOW or FLTK_OPTION_CAIRO_EXT is set (ON)
*/
class FL_EXPORT Fl_Cairo_State {
public:
  Fl_Cairo_State()
    : cc_(0)
    , own_cc_(false)
    , autolink_(false)
    , window_(0)
    , gc_(0) {}

  // access attributes
  cairo_t *cc() const { return cc_; }         ///< Gets the current cairo context
  bool autolink() const { return autolink_; } ///< Gets the autolink option. See Fl::cairo_autolink_context(bool)
  /** Sets the current cairo context.

      \p own == \e true (the default) indicates that the cairo context \p c
      will be deleted by FLTK internally when another cc is set later.

      \p own == \e false indicates cc deletion is handled externally
      by the user program.
  */
  void cc(cairo_t *c, bool own = true) {
    if (cc_ && own_cc_)
      cairo_destroy(cc_);
    cc_ = c;
    if (!cc_)
      window_ = 0;
    own_cc_ = own;
  }
  void autolink(bool b);                   ///< Sets the autolink option, only available with --enable-cairoext
  void window(void *w) { window_ = w; }    ///< Sets the window \p w to keep track on
  void *window() const { return window_; } ///< Gets the last window attached to a cc
  void gc(void *c) { gc_ = c; }            ///< Sets the gc \p c to keep track on
  void *gc() const { return gc_; }         ///< Gets the last gc attached to a cc

private:
  cairo_t *cc_;        // contains the unique autoupdated cairo context
  bool own_cc_;        // indicates whether we must delete the cc, useful for internal cleanup
  bool autolink_;      // false by default, prevents the automatic cairo mapping on fltk windows
                       // for custom cairo implementations.
  void *window_, *gc_; // for keeping track internally of last win+gc treated
};

/** @} */

#endif // FLTK_HAVE_CAIRO
#endif // FL_CAIRO_H
