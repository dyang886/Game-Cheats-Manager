//
// File_Input header file for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2022 by Bill Spitzak and others.
// Original version Copyright 1998 by Curtis Edwards.
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
   Fl_File_Input widget . */

#ifndef Fl_File_Input_H
#  define Fl_File_Input_H

#  include <FL/Fl_Input.H>

/**
  \class Fl_File_Input
  \brief This widget displays a pathname in a text input field.

  A navigation bar located above the input field allows the user to
  navigate upward in the directory tree.
  You may want to handle FL_WHEN_CHANGED events for tracking text changes
  and also FL_WHEN_RELEASE for button release when changing to parent dir.
  FL_WHEN_RELEASE callback won't be called if the directory clicked
  is the same as the current one.

  \image html Fl_File_Input.png
  \image latex Fl_File_Input.png "Fl_File_Input"  width=6cm

  \note As all Fl_Input derived objects, Fl_File_Input may call its callback
  when losing focus (see FL_UNFOCUS) to update its state like its cursor shape.
  One resulting side effect is that you should call clear_changed() early in your callback
  to avoid reentrant calls if you plan to show another window or dialog box in the callback.
*/
class FL_EXPORT Fl_File_Input : public Fl_Input {

  char          ok_entry_;
  uchar         down_box_;
  short         buttons_[200];
  short         pressed_;

  void          draw_buttons();
  int           handle_button(int event);
  void          update_buttons();

public:

  Fl_File_Input(int X, int Y, int W, int H, const char *L=0);

  int handle(int event) FL_OVERRIDE;

protected:
  void draw() FL_OVERRIDE;

public:
  /** Gets the box type used for the navigation bar. */
  Fl_Boxtype    down_box() const { return (Fl_Boxtype)down_box_; }
  /** Sets the box type to use for the navigation bar.  */
  void          down_box(Fl_Boxtype b) { down_box_ = b; }

  /**
    Gets the current error color.

    Returns \p FL_RED since FLTK 1.4.0 (default in 1.3.x).
    Retained for backwards compatibility.

    \deprecated Will be removed in FLTK 1.5.0 or higher.
    \todo Remove Fl_File_Input::errorcolor() in FLTK 1.5.0 or higher.
  */
  Fl_Color      errorcolor() const { return FL_RED; }

  /**
    Sets the current error color to \p c.

    Does nothing since FLTK 1.4.0. Retained for backwards compatibility.

    \deprecated Will be removed in FLTK 1.5.0 or higher.
    \todo Remove Fl_File_Input::errorcolor(Fl_Color) in FLTK 1.5.0 or higher.
    */
  void          errorcolor(Fl_Color c) {(void)c;}

  int   value(const char *str);
  int   value(const char *str, int len);

  /**
    Returns the current value, which is a pointer to an internal buffer
    and is valid only until the next event is handled.
  */
  const char    *value() { return Fl_Input_::value(); }
};

#endif // !Fl_File_Input_H
