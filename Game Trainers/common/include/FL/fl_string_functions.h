/*
 * Platform agnostic string portability functions for the Fast Light Tool Kit (FLTK).
 *
 * Copyright 2020-2022 by Bill Spitzak and others.
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

/**
  \file fl_string_functions.h
  Public header for FLTK's platform-agnostic string handling.
*/

#ifndef _FL_fl_string_functions_h_
#define _FL_fl_string_functions_h_

#include "Fl_Export.H"

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>  // size_t

/** \defgroup fl_string  String handling functions
 String handling functions declared in <FL/fl_string_functions.h>
    @{
*/

FL_EXPORT char* fl_strdup(const char *s);

FL_EXPORT size_t fl_strlcpy(char *, const char *, size_t);

/** @} */

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* _FL_fl_string_functions_h_ */
