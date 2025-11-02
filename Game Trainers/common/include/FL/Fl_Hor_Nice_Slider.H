//
// Horizontal "nice" slider header file for the Fast Light Tool Kit (FLTK).
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
   Fl_Hor_Nice_Slider widget . */

#ifndef Fl_Hor_Nice_Slider_H
#define Fl_Hor_Nice_Slider_H

#include "Fl_Slider.H"

/** \class Fl_Hor_Nice_Slider

 \brief Single thumb tab slider

 \image html fl_hor_nice_slider.png "Fl_Hor_Nice_Slider with various Fl::scheme() values"
 \image latex fl_hor_nice_slider.png "Fl_Hor_Nice_Slider with various Fl::scheme() values" width=6cm

**/

class FL_EXPORT Fl_Hor_Nice_Slider : public Fl_Slider {
public:
  Fl_Hor_Nice_Slider(int X,int Y,int W,int H,const char *L=0);
};

#endif
