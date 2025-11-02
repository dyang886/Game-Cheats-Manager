/*
 * Simple "C"-style types for the Fast Light Tool Kit (FLTK).
 *
 * Copyright 1998-2020 by Bill Spitzak and others.
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

/** \file
 *  This file contains simple "C"-style type definitions.
 */

#ifndef FL_TYPES_H
#define FL_TYPES_H

#include "fl_attr.h"

/** \name       Miscellaneous */
/**@{*/  /* group: Miscellaneous */

/** unsigned char */
typedef unsigned char uchar;
/** unsigned long */
typedef unsigned long ulong;

/** 16-bit Unicode character + 8-bit indicator for keyboard flags.

  \note This \b should be 24-bit Unicode character + 8-bit indicator for
    keyboard flags. The upper 8 bits are currently unused but reserved.

  Due to compatibility issues this type and all FLTK \b shortcuts can only
  be used with 16-bit Unicode characters (<tt>U+0000 .. U+FFFF</tt>) and
  not with the full range of unicode characters (<tt>U+0000 .. U+10FFFF</tt>).

  This is caused by the bit flags \c FL_SHIFT, \c FL_CTRL, \c FL_ALT, and
  \c FL_META being all in the range <tt>0x010000 .. 0x400000</tt>.

  \todo Discuss and decide whether we can "shift" these special keyboard
    flags to the upper byte to enable full 21-bit Unicode characters
    (<tt>U+0000 .. U+10FFFF</tt>) plus the keyboard indicator bits as this
    was originally intended. This would be possible if we could rely on \b all
    programs being coded with symbolic names and not hard coded bit values.

  \internal Can we do the move for 1.4 or, if not, for any later version
    that is allowed to break the ABI?
*/
typedef unsigned int Fl_Shortcut;

/**@}*/  /* group: Miscellaneous */

#endif
