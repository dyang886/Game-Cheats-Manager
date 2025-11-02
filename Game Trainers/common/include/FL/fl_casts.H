//
// Experimental inline "cast functions" for the Fast Light Toolkit (FLTK).
// See also issue #109: "VS2017 warnings when building fltk 1.4.x"
//
// Copyright 1998-2021 by Bill Spitzak and others.
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

#ifndef _FL_fl_casts_H_
#define _FL_fl_casts_H_

#include <FL/platform_types.h>

inline char fl_char(void *v)             { return (char)(fl_intptr_t)v; }
inline int  fl_int(void *v)              { return (int)(fl_intptr_t)v; }
inline long fl_long(void *v)             { return (long)(fl_intptr_t)v; }

inline unsigned char fl_uchar(void *v)   { return (unsigned char)(fl_uintptr_t)v; }
inline unsigned int  fl_uint(void *v)    { return (unsigned int)(fl_uintptr_t)v; }
inline unsigned long fl_ulong(void *v)   { return (unsigned long)(fl_uintptr_t)v; }

// the following conversions can be used to silence MSVC warning C4312:
// 'type cast': conversion from '<type>' to 'void *' of greater size

inline void *fl_voidptr(int v)           { return (void *)(fl_intptr_t)v; }
inline void *fl_voidptr(unsigned int v)  { return (void *)(fl_uintptr_t)v; }
inline void *fl_voidptr(long v)          { return (void *)(fl_intptr_t)v; }
inline void *fl_voidptr(unsigned long v) { return (void *)(fl_uintptr_t)v; }

#endif /* _FL_fl_casts_H_ */
