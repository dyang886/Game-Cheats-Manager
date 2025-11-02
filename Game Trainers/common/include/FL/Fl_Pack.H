//
// Pack header file for the Fast Light Tool Kit (FLTK).
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

/* \file
   Fl_Pack widget . */

#ifndef Fl_Pack_H
#define Fl_Pack_H

#include <FL/Fl_Group.H>

/**
  This widget was designed to add the functionality of compressing and
  aligning widgets.

  If type() is Fl_Pack::HORIZONTAL all the children are
  resized to the height of the Fl_Pack, and are moved next to
  each other horizontally. If type() is not Fl_Pack::HORIZONTAL
  then the children are resized to the width and are stacked below each
  other.  Then the Fl_Pack resizes itself to surround the child widgets.

  You may want to put the Fl_Pack inside an Fl_Scroll.

  The \p 'resizable()' for Fl_Pack is set to NULL by default. Its behavior
  is slightly different than in a normal Fl_Group widget: only if the
  resizable() widget is the last widget in the group it is extended to take
  the full available width or height, respectively, of the Fl_Pack group.

  \note You can nest Fl_Pack widgets or put them inside Fl_Scroll widgets
    or inside other group widgets but their behavior can sometimes be
    <i>"surprising"</i>. This is partly due to the fact that Fl_Pack widgets
    resize themselves during their draw() operation, trying to react on
    their child widgets resizing themselves during \b their draw() operations
    which can be confusing. If you want to achieve special resize behavior
    of nested group widgets it can sometimes be easier to derive your own
    specialized group widget than to try to make nested Fl_Pack widgets
    behave as expected.

  \see Fl_Group::resizable()
*/
class FL_EXPORT Fl_Pack : public Fl_Group {
  int spacing_;

public:
  enum { // values for type(int)
    VERTICAL = 0,
    HORIZONTAL = 1
  };

protected:
  void draw() FL_OVERRIDE;

public:
  Fl_Pack(int X, int Y, int W, int H, const char *L = 0);

  /**
    Gets the number of extra pixels of blank space that are added
    between the children.
  */
  int spacing() const {return spacing_;}

  /**
    Sets the number of extra pixels of blank space that are added
    between the children.
  */
  void spacing(int i) {spacing_ = i;}

  /** Returns non-zero if Fl_Pack alignment is horizontal.

    \returns non-zero if Fl_Pack alignment is horizontal (Fl_Pack::HORIZONTAL)

    \note Currently the return value is the same as Fl_Group::type(), but
      this may change in the future. Do not set any other values than the
      following with Fl_Pack::type():
      - Fl_Pack::VERTICAL (Default)
      - Fl_Pack::HORIZONTAL

    See class Fl_Pack documentation for details.
  */
  uchar horizontal() const {return type();}

  void resize(int X, int Y, int W, int H) FL_OVERRIDE;
  /** Deletes all child widgets with Fl_Group::clear().
  And  sets to NULL the resizable() widget. */
  void clear() { Fl_Group::clear(); resizable(NULL); }
};

#endif
