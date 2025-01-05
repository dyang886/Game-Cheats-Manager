//
// Menu button header file for the Fast Light Tool Kit (FLTK).
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
   Fl_Menu_Button widget . */

#ifndef Fl_Menu_Button_H
#define Fl_Menu_Button_H

#include "Fl_Menu_.H"

/**
  This is a button that when pushed pops up a menu (or hierarchy of
  menus) defined by an array of
  Fl_Menu_Item objects.
  \image html  menu_button.png
  \image latex  menu_button.png " menu_button" width=5cm
  <P>Normally any mouse button will pop up a menu and it is lined up
  below the button as shown in the picture.  However an Fl_Menu_Button
  may also control a pop-up menu.  This is done by setting the type().
  If type() is zero a normal menu button is produced.
  If it is nonzero then this is a pop-up menu. The bits in type() indicate
  what mouse buttons pop up the menu (see Fl_Menu_Button::popup_buttons). </P>
  <P>The menu will also pop up in response to shortcuts indicated by
  putting a '&' character in the label(). </P>
  <P>Typing the shortcut() of any of the menu items will cause
  callbacks exactly the same as when you pick the item with the mouse.
  The '&' character in menu item names are only looked at when the menu is
  popped up, however. </P>

  When the user clicks a menu item, value() is set to that item
  and then:

  - The item's callback is done if one has been set; the
    Fl_Menu_Button is passed as the Fl_Widget* argument,
    along with any userdata configured for the callback.

  - If the item does not have a callback, the Fl_Menu_Button's callback
    is done instead, along with any userdata configured for it.
    The callback can determine which item was picked using
    value(), mvalue(), item_pathname(), etc.
*/
class FL_EXPORT Fl_Menu_Button : public Fl_Menu_ {
protected:
  void draw() FL_OVERRIDE;
  static Fl_Menu_Button* pressed_menu_button_;
public:
  /**
   \brief indicate what mouse buttons pop up the menu.

   Values for type() used to indicate what mouse buttons pop up the menu.
   Fl_Menu_Button::POPUP3 is usually what you want.
   */
  enum popup_buttons {POPUP1 = 1, /**< pops up with the mouse 1st button. */
    POPUP2,  /**< pops up with the mouse 2nd button. */
    POPUP12, /**< pops up with the mouse 1st or 2nd buttons. */
    POPUP3,   /**< pops up with the mouse 3rd button. */
    POPUP13,  /**< pops up with the mouse 1st or 3rd buttons. */
    POPUP23,  /**< pops up with the mouse 2nd or 3rd buttons. */
    POPUP123 /**< pops up with any mouse button. */
  };
  int handle(int) FL_OVERRIDE;
  const Fl_Menu_Item* popup();
  Fl_Menu_Button(int,int,int,int,const char * =0);
};

#endif
