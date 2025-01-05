//
// Mac header file for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2018 by Bill Spitzak and others.
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
// include this file if "__APPLE__" is defined.  This is to encourage
// portability of even the system-specific code...
#ifndef FL_DOXYGEN

#if !defined(FL_PLATFORM_H)
#  error "Never use <FL/mac.H> directly; include <FL/platform.H> instead."
#endif // !FL_PLATFORM_H

#ifdef __OBJC__
  @class NSOpenGLContext;
#  ifndef GL_SILENCE_DEPRECATION
#    define GL_SILENCE_DEPRECATION 1
#  endif
#elif defined(__cplusplus)
  class NSOpenGLContext;
#endif /* __OBJC__ */
extern NSOpenGLContext *fl_mac_glcontext(GLContext rc);

#ifdef __OBJC__
@class FLWindow; // a subclass of the NSWindow Cocoa class
typedef FLWindow *Window;
#else
typedef class FLWindow *Window; // pointer to the FLWindow objective-c class
#endif // __OBJC__

#include <FL/Fl_Widget.H> // for Fl_Callback

#if (defined(FL_LIBRARY) || defined(FL_INTERNALS)) // this part must be compiled when building the FLTK libraries

// Standard MacOS C/C++ includes...
#include <ApplicationServices/ApplicationServices.h>
#undef check // because of Fl::check()

#ifndef MAC_OS_X_VERSION_10_4
#define MAC_OS_X_VERSION_10_4 1040
#endif
#ifndef MAC_OS_X_VERSION_10_5
#define MAC_OS_X_VERSION_10_5 1050
#endif
#ifndef MAC_OS_X_VERSION_10_6
#define MAC_OS_X_VERSION_10_6 1060
#endif
#ifndef MAC_OS_X_VERSION_10_7
#define MAC_OS_X_VERSION_10_7 1070
#endif
#ifndef MAC_OS_X_VERSION_10_8
#define MAC_OS_X_VERSION_10_8 1080
#endif
#ifndef MAC_OS_X_VERSION_10_9
#define MAC_OS_X_VERSION_10_9 1090
#endif
#ifndef MAC_OS_X_VERSION_10_10
#define MAC_OS_X_VERSION_10_10 101000
#endif
#ifndef MAC_OS_X_VERSION_10_11
#define MAC_OS_X_VERSION_10_11 101100
#endif
#ifndef MAC_OS_X_VERSION_10_12
#define MAC_OS_X_VERSION_10_12 101200
#endif
#ifndef MAC_OS_X_VERSION_10_13
#define MAC_OS_X_VERSION_10_13 101300
#endif
#ifndef MAC_OS_X_VERSION_10_14
#define MAC_OS_X_VERSION_10_14 101400
#endif
#ifndef MAC_OS_X_VERSION_10_15
#define MAC_OS_X_VERSION_10_15 101500
#endif
#ifndef MAC_OS_X_VERSION_10_16
#define MAC_OS_X_VERSION_10_16 101600
#endif
#ifndef MAC_OS_VERSION_11_0
#define MAC_OS_VERSION_11_0 110000
#endif
#ifndef MAC_OS_VERSION_12_0
#define MAC_OS_VERSION_12_0 120000
#endif
#ifndef MAC_OS_VERSION_13_0
#define MAC_OS_VERSION_13_0 130000
#endif
#ifndef MAC_OS_VERSION_14_0
#define MAC_OS_VERSION_14_0 140000
#endif
#ifndef MAC_OS_VERSION_15_0
#define MAC_OS_VERSION_15_0 150000
#endif


#ifndef NSINTEGER_DEFINED // appears with 10.5 in NSObjCRuntime.h
#if defined(__LP64__) && __LP64__
typedef long NSInteger;
typedef unsigned long NSUInteger;
#else
typedef int NSInteger;
typedef unsigned int NSUInteger;
#endif
#endif

#if MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_4
typedef CGImageAlphaInfo CGBitmapInfo;
#endif

struct flCocoaRegion {
  int count;
  CGRect *rects;
}; // a region is the union of a series of rectangles

#ifndef CGFLOAT_DEFINED //appears with 10.5 in CGBase.h
#if defined(__LP64__) && __LP64__
typedef double CGFloat;
#else
typedef float CGFloat;
#endif
#endif // CGFLOAT_DEFINED

#else

typedef struct CGContext* CGContextRef;

#endif // FL_LIBRARY || FL_INTERNALS

extern CGContextRef fl_gc;

#endif // FL_DOXYGEN
/** \file
 Mac OS X-specific symbols.
 */

/** \defgroup group_macosx Mac OS X-specific symbols
 Mac OS X-specific symbols declared in <FL/platform.H>
 \sa \ref osissues_macos
 \{ */

/**
 Attaches a callback to the "About myprog" item of the system application menu.
 For back-compatibility.
 Equivalent to Fl_Sys_Menu_Bar::about(Fl_Callback *cb, void *user_data).
 */
void fl_mac_set_about(Fl_Callback *cb, void *user_data, int shortcut = 0);

/** Returns the macOS-specific graphics context for the current window */
extern CGContextRef fl_mac_gc();
/** Returns the macOS-specific window reference corresponding to the given Fl_Window object */
extern FLWindow *fl_mac_xid(const Fl_Window *win);
/** Returns the Fl_Window corresponding to the given macOS-specific window reference */
extern Fl_Window *fl_mac_find(FLWindow *);
class Fl_Gl_Window;

/** The version number of the running Mac OS X (e.g., 100604 for 10.6.4, 101300 for 10.13, 140102 for 14.1.2).
 FLTK initializes this global variable before main() begins running. If
 the value is needed in a static initializer, a previous call to Fl::system_driver()
 makes sure \ref fl_mac_os_version has been initialized.
 */
extern int fl_mac_os_version;

struct Fl_Menu_Item;

class Fl_Mac_App_Menu {
public:
  /** Localizable text for the "About xxx" application menu item */
  static const char *about;
  /** Localizable text for the "Print Front Window" application menu item.
   This menu item and next one won't be displayed if Fl_Mac_App_Menu::print
   is set to an empty string.
   */
  static const char *print;
  /** Localizable text for the "Print Front Window" application menu item. */
  static const char *print_no_titlebar;
  /** Localizable text for the "Toggle print titlebar" application menu item. */
  static const char *toggle_print_titlebar;
  /** Localizable text for the "Services" application menu item */
  static const char *services;
  /** Localizable text for the "Hide xxx" application menu item */
  static const char *hide;
  /** Localizable text for the "Hide Others" application menu item */
  static const char *hide_others;
  /** Localizable text for the "Show All" application menu item */
  static const char *show;
  /** Localizable text for the "Quit xxx" application menu item */
  static const char *quit;
  /** Adds custom menu item(s) to the application menu of the system menu bar.
   They are positioned after the "Print Front Window / Toggle printing of titlebar" items, or at their place
   if they were removed with <tt>Fl_Mac_App_Menu::print = ""</tt>.
   \param m zero-ending array of Fl_Menu_Item 's.
   */
  static void custom_application_menu_items(const Fl_Menu_Item *m);
};

/** @} */

