//
// Return button header file for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2022 by Bill Spitzak and others.
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
   Fl_Return_Button widget . */

#ifndef Fl_Return_Button_H
#define Fl_Return_Button_H
#include "Fl_Button.H"

/**
  The Fl_Return_Button is a subclass of Fl_Button that
  generates a callback when it is pressed or when the user presses the
  Enter key.  A carriage-return symbol is drawn next to the button label.
  \image html Fl_Return_Button.png
  \image latex Fl_Return_Button.png "Fl_Return_Button" width=4cm
*/
class FL_EXPORT Fl_Return_Button : public Fl_Button {
protected:
  void draw() FL_OVERRIDE;
public:
  int handle(int) FL_OVERRIDE;
  /**
    Creates a new Fl_Return_Button widget using the given
    position, size, and label string. The default boxtype is FL_UP_BOX.
    <P> The inherited destructor deletes the button.
  */
  Fl_Return_Button(int X, int Y, int W, int H,const char *l=0);
};

#endif
