//
// Multi browser header file for the Fast Light Tool Kit (FLTK).
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
   Fl_Multi_Browser widget . */

#ifndef Fl_Multi_Browser_H
#define Fl_Multi_Browser_H

#include "Fl_Browser.H"

/**
  The Fl_Multi_Browser class is a subclass of Fl_Browser
  which lets the user select any set of the lines.

  \image html  fl_multi_browser.png "Fl_Multi_Browser"
  \image latex fl_multi_browser.png "Fl_Multi_Browser" width=4cm

  The user interface is Macintosh style: clicking an item turns off all the others and
  selects that one, dragging selects all the items the mouse moves over,
  and ctrl + click (Cmd+click on the Mac OS platform) toggles the items.
  Shift + click extends the selection until the clicked item.
  This is different from how forms did it.
  Normally the callback is done when the user releases the
  mouse, but you can change this with when().
  <P>See Fl_Browser for methods to add and remove lines from the browser.
*/
class FL_EXPORT Fl_Multi_Browser : public Fl_Browser {
public:
  /**
    Creates a new Fl_Multi_Browser widget using the given
    position, size, and label string. The default boxtype is FL_DOWN_BOX.
    The constructor specializes Fl_Browser() by setting the type to FL_MULTI_BROWSER.
    The destructor destroys the widget and frees all memory that has been allocated.
  */
    Fl_Multi_Browser(int X,int Y,int W,int H,const char *L=0);
};

#endif
