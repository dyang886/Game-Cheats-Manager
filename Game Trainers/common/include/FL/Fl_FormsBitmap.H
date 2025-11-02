//
// Forms bitmap header file for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2010 by Bill Spitzak and others.
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

/* \file
   Fl_FormsBitmap widget . */

#ifndef Fl_FormsBitmap_H
#define Fl_FormsBitmap_H

#include "Fl_Bitmap.H"

/**
    Forms compatibility Bitmap Image Widget
*/
class FL_EXPORT Fl_FormsBitmap : public Fl_Widget {
    Fl_Bitmap *b;
protected:
    void draw() FL_OVERRIDE;
public:
    Fl_FormsBitmap(Fl_Boxtype, int, int, int, int, const char * = 0);
    void set(int W, int H, const uchar *bits);
    /** Sets a new bitmap. */
    void bitmap(Fl_Bitmap *B) {b = B;}
    /** Gets a the current associated Fl_Bitmap objects. */
    Fl_Bitmap *bitmap() const {return b;}
};

#endif
