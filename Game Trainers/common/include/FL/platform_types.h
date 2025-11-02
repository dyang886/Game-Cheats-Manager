/*
 * Copyright 2016-2023 by Bill Spitzak and others.
 *
 * This library is free software. Distribution and use rights are outlined in
 * the file "COPYING" which should have been included with this file.  If this
 * file is missing or damaged, see the license at:
 *
 *     https://www.fltk.org/COPYING.php
 *
 * Please see the following page on how to report bugs and issues:
 *
 *     https://www.fltk.org/bugs.php
 */

#ifndef Fl_Platform_Types_H
#define Fl_Platform_Types_H

/** \file
 Definitions of platform-dependent types.
 The exact nature of these types varies with the platform.
 Therefore, portable FLTK applications should not assume these types
 have a specific size, or that they are pointers.
 */

#ifdef FL_DOXYGEN

/** An integral type large enough to store a pointer or a long value.
 A pointer value can be safely cast to fl_intptr_t, and later cast back
 to its initial pointer type without change to the pointer value.
 A variable of type fl_intptr_t can also store a long int value. */
typedef opaque fl_intptr_t;
/** An unsigned integral type large enough to store a pointer or an unsigned long value.
 A pointer value can be safely cast to fl_uintptr_t, and later cast back
 to its initial pointer type without change to the pointer value.
 A variable of type fl_uintptr_t can also store an unsigned long int value. */
typedef opaque fl_uintptr_t;

/**
 Platform-specific value representing an offscreen drawing buffer.
  \note This value can be safely cast to these types on each platform:
  \li X11: Pixmap
  \li Wayland: cairo_t *
  \li Windows: HBITMAP
  \li macOS:  CGContextRef
 */
typedef opaque Fl_Offscreen;

/**
 Pointer to a platform-specific structure representing a collection of rectangles.
  \note This pointer can be safely cast to these types on each platform:
  \li X11: Region as defined by X11
  \li Wayland:  cairo_region_t *
  \li Windows: HRGN
  \li macOS:  struct flCocoaRegion *
 */
typedef struct opaque *Fl_Region;
typedef opaque FL_SOCKET; /**< socket or file descriptor */
/**
 Pointer to a platform-specific structure representing the window's OpenGL rendering context.
  \note This pointer can be safely cast to these types on each platform:
  \li X11: GLXContext
  \li Wayland: EGLContext
  \li Windows: HGLRC
  \li macOS: NSOpenGLContext *
 */
typedef struct opaque *GLContext;

/**
 Platform-specific point in time, used for delta time calculation.
 \note This type may be a struct. sizeof(Fl_Timestamp) may be different on
 different platforms. Fl_Timestamp may change with future ABI changes.
 */
typedef opaque Fl_Timestamp;

#  define FL_COMMAND  opaque   /**< An alias for FL_CTRL on Windows and X11, or FL_META on MacOS X */
#  define FL_CONTROL  opaque   /**< An alias for FL_META on Windows and X11, or FL_CTRL on MacOS X */

#else /* FL_DOXYGEN */

#ifndef FL_PLATFORM_TYPES_H
#define FL_PLATFORM_TYPES_H

#include <FL/fl_config.h>
#include <time.h> // for time_t

/* Platform-dependent types are defined here.
  These types must be defined by any platform:
  FL_SOCKET, struct dirent, fl_intptr_t, fl_uintptr_t

  NOTE: *FIXME* AlbrechtS 13 Apr 2016 (concerning FL_SOCKET)
  ----------------------------------------------------------
    The Fl::add_fd() API is partially inconsistent because some of the methods
    explicitly use 'int', but the callback typedefs use FL_SOCKET. With the
    definition of FL_SOCKET below we can have different data sizes and
    different signedness of socket numbers on *some* platforms.
 */

#ifdef _WIN64

#if defined(_MSC_VER) && (_MSC_VER < 1600)
# include <stddef.h>  /* stdint.h not available before VS 2010 (1600) */
#else
# include <stdint.h>
#endif

typedef intptr_t fl_intptr_t;
typedef uintptr_t fl_uintptr_t;

#else /* ! _WIN64 */

typedef long fl_intptr_t;
typedef unsigned long fl_uintptr_t;

#endif /* _WIN64 */

typedef void *GLContext;
typedef void *Fl_Region;
typedef fl_uintptr_t  Fl_Offscreen;

/* Allows all hybrid combinations except WIN32 + X11 with MSVC */
#if defined(_WIN32) && !defined(__MINGW32__)
  struct dirent {char d_name[1];};
#else
#  include <dirent.h>
#endif

# if defined(_WIN64) && defined(_MSC_VER)
typedef  unsigned __int64 FL_SOCKET;    /* *FIXME* - FL_SOCKET (see above) */
# else
typedef  int FL_SOCKET;
# endif

#include <FL/Fl_Export.H>
extern FL_EXPORT int fl_command_modifier();
extern FL_EXPORT int fl_control_modifier();
#  define FL_COMMAND    fl_command_modifier()
#  define FL_CONTROL    fl_control_modifier()

#endif /* FL_PLATFORM_TYPES_H */

/* This is currently the same for all platforms but may change in the future */
struct Fl_Timestamp_t {
  time_t sec;
  int usec;
};

typedef struct Fl_Timestamp_t Fl_Timestamp;

#endif /* FL_DOXYGEN */

#endif /* Fl_Platform_Types_H */

