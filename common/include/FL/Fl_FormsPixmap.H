//
// Forms pixmap header file for the Fast Light Tool Kit (FLTK).
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
   Fl_FormsPixmap widget . */

#ifndef Fl_FormsPixmap_H
#define Fl_FormsPixmap_H

#include "Fl_Pixmap.H"

/**
  \class Fl_FormsPixmap
  \brief Forms pixmap drawing routines
*/
class FL_EXPORT Fl_FormsPixmap : public Fl_Widget {
    Fl_Pixmap *b;
protected:
    void draw() FL_OVERRIDE;
public:
    Fl_FormsPixmap(Fl_Boxtype t, int X, int Y, int W, int H, const char *L= 0);

    void set(/*const*/char * const * bits);

    /**
      Set the internal pixmap pointer to an existing pixmap.
      \param[in] B existing pixmap
    */
    void Pixmap(Fl_Pixmap *B) {b = B;}

    /** Get the internal pixmap pointer. */
    Fl_Pixmap *Pixmap() const {return b;}
};

#endif
