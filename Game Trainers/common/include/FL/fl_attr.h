/*
 * Function attribute declarations for the Fast Light Tool Kit (FLTK).
 *
 * Copyright 1998-2024 by Bill Spitzak and others.
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
 \file fl_attr.h
 This file defines compiler-specific macros
*/

#ifndef _FL_fl_attr_h_
#define _FL_fl_attr_h_


/**
 This section lists macros for Doxygen documentation only. The next section
 will define the actual macros based on the compile used and based on the
 capabilities of the version of that compiler.
 */
#ifdef FL_DOXYGEN


/** To be used in prototypes with a variable list of arguments.
 This macro helps detection of mismatches between format string and
 argument list at compilation time.

 Usage example: FL/fl_ask.H
 */
#define __fl_attr(x)

/**
  This macro makes it safe to use the C++11 keyword \c override with
  older compilers.
*/
#define FL_OVERRIDE override

/**
 Enclosing a function or method in FL_DEPRECATED marks it as no longer
 recommended. This macro syntax can not be used if the return type contains
 a comma, which is not the case in FLTK.

 \code
 FL_DEPRECATED("Outdated, don't use", int position()) { return position_; }
 \endcode
 */
#define FL_DEPRECATED(msg, func) \
  /##*##* \deprecated msg *##/ \
  func


#else /* FL_DOXYGEN */

// If FL_NO_DEPRECATED is defined FLTK 1.4 can compile 1.3.x code without
// issuing several "deprecated" warnings (1.3 "compatibility" mode).
// FL_DEPRECATED will be defined as a no-op.

// If FL_NO_DEPRECATED is not defined (default) FLTK 1.4 will issue several
// "deprecated" warnings depending on the compiler in use: FL_DEPRECATED
// will be defined according to the capabilities of the compiler (below).
// The definition below this comment must match the one at the end of this file.

#if defined(FL_NO_DEPRECATED)
#define FL_DEPRECATED(msg, func) func
#endif

#ifdef __cplusplus

/*
  Declare macros specific to Visual Studio.

  Visual Studio defines __cplusplus = '199711L' in all its versions which is
  not helpful for us here. For VS version number encoding see:
  https://learn.microsoft.com/en-us/cpp/preprocessor/predefined-macros

  This document specifies that the macro _MSVC_LANG is defined since
  "Visual Studio 2015 Update 3" as 201402L (default) and undefined in
  earlier versions. It can be used to determine the C++ standard as
  specified by the /std:c++ compiler option:

  - /std:c++14      201402L  (also if /std:c++ is not used)
  - /std:c++17      201703L
  - /std:c++20      202002L
  - /std:c++latest  a "higher, unspecified value" (docs of VS 2022)

  As of this writing (02/2023) _MSVC_LANG is not yet used in this file
  but it is documented for future use.
*/

#if defined(_MSC_VER)

#if (_MSC_VER >= 1900) // Visual Studio 2015 (14.0)
#ifndef FL_OVERRIDE
#define FL_OVERRIDE override
#endif
#endif // Visual Studio 2015 (14.0)

#if (_MSC_VER >= 1400) // Visual Studio 2005 (8.0)
#ifndef FL_DEPRECATED
#define FL_DEPRECATED(msg, func) __declspec(deprecated(msg)) func
#endif
#endif // Visual Studio 2005 (8.0)

#if (_MSC_VER >= 1310) // Visual Studio .NET 2003 (7.1)
#ifndef FL_DEPRECATED
#define FL_DEPRECATED(msg, func) __declspec(deprecated) func
#endif
#endif // Visual Studio .NET 2003 (7.1)

#endif // Visual Studio


/*
 Declare macros specific to the C++ standard used.

 Macros may have been declared already in previous sections.
 */
#if (__cplusplus >= 202002L) // C++20
#endif // C++20

#if (__cplusplus >= 201703L) // C++17
#endif // C++17

#if (__cplusplus >= 201402L) // C++14
#ifndef FL_DEPRECATED
#define FL_DEPRECATED(msg, func) [[deprecated(msg)]] func
#endif
#endif // C++14

#if (__cplusplus >= 201103L) // C++11
#ifndef FL_OVERRIDE
#define FL_OVERRIDE override
#endif
#endif // C+11

#if (__cplusplus >= 199711L) // C++89
#endif // C++89

#endif // __cplusplus

/*
 Declare macros specific to clang

 Macros may have been declared already in previous sections.
 */
#if defined(__clang__)

#define FL_CLANG_VERSION (__clang_major__ * 10000 + __clang_minor__ * 100 + __clang_patchlevel__)

// -- nothing yet --

#endif /* __clang__ */


/*
 Declare macros specific to gcc.

 Macros may have been declared already in previous sections.
 */
#if defined(__GNUC__)

#define FL_GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)

#ifndef __fl_attr
#define __fl_attr(x) __attribute__ (x)
#endif

#if FL_GCC_VERSION > 40500 /* gcc 4.5.0 */
#ifndef FL_DEPRECATED
#define FL_DEPRECATED(msg, func) func __attribute__((deprecated(msg)))
#endif
#endif /* gcc 4.5.0 */

#if FL_GCC_VERSION >= 30400 /* gcc 3.4.0 */
#ifndef FL_DEPRECATED
#define FL_DEPRECATED(msg, func) func __attribute__((deprecated))
#endif
#endif /* gcc 3.4.0 */

#endif /* __GNUC__ */


/*
 If a macro was not defined in any of the sections above, set it to no-op here.
 */

#ifndef __fl_attr
#define __fl_attr(x)
#endif

#ifndef FL_OVERRIDE
#define FL_OVERRIDE
#endif

#ifndef FL_DEPRECATED
#define FL_DEPRECATED(msg, func) func
#endif


#endif /* FL_DOXYGEN */


#endif /* !_FL_fl_attr_h_ */
