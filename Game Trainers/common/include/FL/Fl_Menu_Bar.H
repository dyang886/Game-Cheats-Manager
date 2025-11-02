//
// Menu bar header file for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2017 by Bill Spitzak and others.
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
   Fl_Menu_Bar widget . */

#ifndef Fl_Menu_Bar_H
#define Fl_Menu_Bar_H

#include "Fl_Menu_.H"

/**
  This widget provides a standard menubar interface.  Usually you will
  put this widget along the top edge of your window.  The height of the
  widget should be 30 for the menu titles to draw correctly with the
  default font.

  The items on the bar and the menus they bring up are defined by a
  single Fl_Menu_Item array.
  Because a Fl_Menu_Item array defines a hierarchy, the
  top level menu defines the items in the menubar, while the submenus
  define the pull-down menus. Sub-sub menus and lower pop up to the right
  of the submenus.

  \image html  menubar.png
  \image latex  menubar.png " menubar" width=12cm

  If there is an item in the top menu that is not a title of a
  submenu, then it acts like a "button" in the menubar.  Clicking on it
  will pick it.

  When the user clicks a menu item, value() is set to that item
  and then:

    - The item's callback is done if one has been set; the
      Fl_Menu_Bar is passed as the Fl_Widget* argument,
      along with any userdata configured for the callback.

    - If the item does not have a callback, the Fl_Menu_Bar's callback
      is done instead, along with any userdata configured for the callback.
      The callback can determine which item was picked using
      value(), mvalue(), item_pathname(), etc.

  Submenus will also pop up in response to shortcuts indicated by
  putting a '&' character in the name field of the menu item. If you put a
  '&' character in a top-level "button" then the shortcut picks it.  The
  '&' character in submenus is ignored until the menu is popped up.

  Typing the shortcut() of any of the menu items will cause
  callbacks exactly the same as when you pick the item with the mouse.
 */
class FL_EXPORT Fl_Menu_Bar : public Fl_Menu_ {
  friend class Fl_Sys_Menu_Bar_Driver;
protected:
    void draw() FL_OVERRIDE;
public:
    int handle(int) FL_OVERRIDE;
  /**
    Creates a new Fl_Menu_Bar widget using the given position,
    size, and label string. The default boxtype is FL_UP_BOX.

    The constructor sets menu() to NULL.  See
    Fl_Menu_ for the methods to set or change the menu.

    labelsize(), labelfont(), and labelcolor()
    are used to control how the menubar items are drawn.  They are
    initialized from the Fl_Menu static variables, but you can
    change them if desired.

    label() is ignored unless you change align() to
    put it outside the menubar.

    The destructor removes the Fl_Menu_Bar widget and all of its
    menu items.
  */
  Fl_Menu_Bar(int X, int Y, int W, int H, const char *l=0);
  /** Updates the menu bar after any change to its items.
   This is useful when the menu bar can be an Fl_Sys_Menu_Bar object.
   */
  virtual void update() {}
  /**
   Opens the 1st level submenu of the menubar corresponding to \c item.
   \since 1.4.0
   */
  virtual void play_menu(const Fl_Menu_Item *item);
};

#endif
