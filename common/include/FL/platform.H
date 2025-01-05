//
// Platform header file for the Fast Light Tool Kit (FLTK).
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

// These are FLTK symbols that are necessary or useful for calling
// platform specific functions. This file #include's certain platform
// specific system header files that are necessary to declare platform
// specific FLTK functions, for instance "Windows.h" under Windows.

// You should include this file if (and ONLY if) you need to call
// platform specific functions directly.

// See FLTK documentation: chapter "Operating System Issues" on when
// you need to #include <FL/platform.H>

#if !defined(FL_PLATFORM_H) && !defined(FL_DOXYGEN)
#  define FL_PLATFORM_H

#   include <FL/Fl_Export.H>
#   include <FL/platform_types.h> // will bring in FL/fl_config.h
#   include <FL/fl_types.h> // for uchar
class Fl_Window;

#  ifdef _WIN32
#    include "win32.H"
#  elif defined(FLTK_USE_WAYLAND)
#    include "wayland.H"
#  elif defined(FLTK_USE_X11)
#    include "x11.H"
#  elif defined(__APPLE__)
#    include "mac.H"
#  endif // _WIN32

//
// cross-platform declarations
//
#if defined(FL_LIBRARY) || defined(FL_INTERNALS)
#  include <FL/Fl_Window.H>

class FL_EXPORT Fl_X {
public:
  fl_uintptr_t xid;
  Fl_Window* w;
  Fl_Region region;
  Fl_X *next;
  // static variables, static functions and member functions
  static Fl_X* first;
  static Fl_X* flx(const Fl_Window* w) {return w ? (Fl_X*)w->flx_ : 0;}
#  if defined(FLTK_USE_X11) && FLTK_USE_X11 // for backward compatibility
  static void make_xid(Fl_Window*, XVisualInfo* =fl_visual, Colormap=fl_colormap);
  static Fl_X* set_xid(Fl_Window*, Window);
  static inline Fl_X* i(const Fl_Window* w) {return flx(w);}
#  endif
};

inline Window fl_xid(const Fl_Window* w) { Fl_X *xTemp = Fl_X::flx(w); return xTemp ? (Window)xTemp->xid : 0; }
#else
extern FL_EXPORT Window fl_xid_(const Fl_Window* w);
#  define fl_xid(w) fl_xid_(w)
#endif // FL_LIBRARY || FL_INTERNALS

extern FL_EXPORT Fl_Window* fl_find(Window xid);
extern FL_EXPORT void fl_open_display();
extern FL_EXPORT void fl_close_display();
extern FL_EXPORT Window fl_window;
extern FL_EXPORT int fl_parse_color(const char* p, uchar& r, uchar& g, uchar& b);
extern FL_EXPORT void fl_open_callback(void (*)(const char *));

#endif // !FL_PLATFORM_H
