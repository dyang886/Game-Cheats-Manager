//
// Event and other names header file for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2024 by Bill Spitzak and others.
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

// Thanks to Greg Ercolano for this addition.

/**
 \file names.h
 This file defines arrays of human readable names for FLTK symbolic constants.
*/

#ifndef FL_NAMES_H
#define FL_NAMES_H

/** \defgroup fl_events Events handling functions
    @{
 */

/**
  This is an array of event names you can use to convert event numbers into names.

  The array gets defined inline wherever your '\#include <FL/names.h>' appears.

  \b Example:
  \code
  #include <FL/names.h>         // array will be defined here
  int MyClass::handle(int e) {
      printf("Event was %s (%d)\n", fl_eventnames[e], e);
      // ..resulting output might be e.g. "Event was FL_PUSH (1)"..
      [..]
  }
  \endcode
 */
const char * const fl_eventnames[] =
{
  "FL_NO_EVENT",
  "FL_PUSH",
  "FL_RELEASE",
  "FL_ENTER",
  "FL_LEAVE",
  "FL_DRAG",
  "FL_FOCUS",
  "FL_UNFOCUS",
  "FL_KEYDOWN",
  "FL_KEYUP",
  "FL_CLOSE",
  "FL_MOVE",
  "FL_SHORTCUT",
  "FL_DEACTIVATE",
  "FL_ACTIVATE",
  "FL_HIDE",
  "FL_SHOW",
  "FL_PASTE",
  "FL_SELECTIONCLEAR",
  "FL_MOUSEWHEEL",
  "FL_DND_ENTER",
  "FL_DND_DRAG",
  "FL_DND_LEAVE",
  "FL_DND_RELEASE",
  "FL_SCREEN_CONFIGURATION_CHANGED",
  "FL_FULLSCREEN",
  "FL_ZOOM_GESTURE",
  "FL_ZOOM_EVENT",
  "FL_EVENT_28", // not yet defined, just in case it /will/ be defined ...
  "FL_EVENT_29", // not yet defined, just in case it /will/ be defined ...
  "FL_EVENT_30"  // not yet defined, just in case it /will/ be defined ...
};

/**
  This is an array of font names you can use to convert font numbers into names.

  The array gets defined inline wherever your '\#include <FL/names.h>' appears.

  \b Example:
  \code
  #include <FL/names.h>         // array will be defined here
  int MyClass::my_callback(Fl_Widget *w, void*) {
      int fnum = w->labelfont();
      // Resulting output might be e.g. "Label's font is FL_HELVETICA (0)"
      printf("Label's font is %s (%d)\n", fl_fontnames[fnum], fnum);
      // ..resulting output might be e.g. "Label's font is FL_HELVETICA (0)"..
      [..]
  }
  \endcode
 */
const char * const fl_fontnames[] =
{
  "FL_HELVETICA",
  "FL_HELVETICA_BOLD",
  "FL_HELVETICA_ITALIC",
  "FL_HELVETICA_BOLD_ITALIC",
  "FL_COURIER",
  "FL_COURIER_BOLD",
  "FL_COURIER_ITALIC",
  "FL_COURIER_BOLD_ITALIC",
  "FL_TIMES",
  "FL_TIMES_BOLD",
  "FL_TIMES_ITALIC",
  "FL_TIMES_BOLD_ITALIC",
  "FL_SYMBOL",
  "FL_SCREEN",
  "FL_SCREEN_BOLD",
  "FL_ZAPF_DINGBATS",
};

/**
 This is an array of callback reason names you can use to convert callback reasons into names.

 The array gets defined inline wherever your '\#include <FL/names.h>' appears.
 */
const char * const fl_callback_reason_names[] =
{
  "FL_REASON_UNKNOWN",
  "FL_REASON_SELECTED",
  "FL_REASON_DESELECTED",
  "FL_REASON_RESELECTED",
  "FL_REASON_OPENED",
  "FL_REASON_CLOSED",
  "FL_REASON_DRAGGED",
  "FL_REASON_CANCELLED",
  "FL_REASON_CHANGED",
  "FL_REASON_GOT_FOCUS",
  "FL_REASON_LOST_FOCUS",
  "FL_REASON_RELEASED",
  "FL_REASON_ENTER_KEY",
  NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  "FL_REASON_USER", "FL_REASON_USER+1", "FL_REASON_USER+2", "FL_REASON_USER+3",
};

/** @} */

#endif /* FL_NAMES_H */
