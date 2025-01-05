//
// Hold browser header file for the Fast Light Tool Kit (FLTK).
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
   Fl_Hold_Browser widget . */

#ifndef Fl_Hold_Browser_H
#define Fl_Hold_Browser_H

#include "Fl_Browser.H"

/**
  The Fl_Hold_Browser is a subclass of Fl_Browser which lets the user
  select a single item, or no items by clicking on the empty space.

  \image html  fl_hold_browser.png "Fl_Hold_Browser"
  \image latex fl_hold_browser.png "Fl_Hold_Browser" width=4cm

  As long as the mouse button is held down the item
  pointed to by it is highlighted, and this highlighting remains on when
  the mouse button is released. Normally the callback is done when the
  user releases the mouse, but you can change this with when().
  <P>See Fl_Browser for methods to add and remove lines from the browser.
*/
class FL_EXPORT Fl_Hold_Browser : public Fl_Browser {
public:
  /**
    Creates a new Fl_Hold_Browser widget using the given
    position, size, and label string. The default boxtype is FL_DOWN_BOX.
    The constructor specializes Fl_Browser() by setting the type to FL_HOLD_BROWSER.
    The destructor destroys the widget and frees all memory that has been allocated.
 */
  Fl_Hold_Browser(int X,int Y,int W,int H,const char *L=0);
};

#endif
