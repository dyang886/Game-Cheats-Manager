//
// Math header file for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2020 by Bill Spitzak and others.
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

// Xcode on macOS includes files by recursing down into directories.
// This code catches the cycle and directly includes the required file.
#ifdef fl_math_h_cyclic_include
#  include "/usr/include/math.h"
#endif

#ifndef fl_math_h
#  define fl_math_h

#  define fl_math_h_cyclic_include
#  include <math.h>
#  undef fl_math_h_cyclic_include

#  ifndef M_PI
#    define M_PI            3.14159265358979323846
#    define M_PI_2          1.57079632679489661923
#    define M_PI_4          0.78539816339744830962
#    define M_1_PI          0.31830988618379067154
#    define M_2_PI          0.63661977236758134308
#  endif // !M_PI

#  ifndef M_SQRT2
#    define M_SQRT2         1.41421356237309504880
#    define M_SQRT1_2       0.70710678118654752440
#  endif // !M_SQRT2

#  if (defined(_WIN32) || defined(CRAY)) && !defined(__MINGW32__)

inline double rint(double v) {return floor(v+.5);}
inline double copysign(double a, double b) {return b<0 ? -a : a;}

#  endif // (_WIN32 || CRAY) && !__MINGW32__

#endif // !fl_math_h
