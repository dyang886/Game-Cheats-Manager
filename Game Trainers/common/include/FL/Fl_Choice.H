//
// Choice header file for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2024 by Bill Spitzak and others.
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
   Fl_Choice widget . */

#ifndef Fl_Choice_H
#define Fl_Choice_H

#include "Fl_Menu_.H"

/**
  \class Fl_Choice
  \brief A button that is used to pop up a menu.

  \image html choice.png
  \image latex choice.png  "Fl_Choice" width=4cm

  This is a button that, when pushed, pops up a menu (or hierarchy of menus)
  defined by an array of Fl_Menu_Item objects.
  Motif calls this an OptionButton.

  The only difference between this and a Fl_Menu_Button is that the name of
  the most recent chosen menu item is displayed inside the box, while the
  label is displayed outside the box. However, since the use of this is most
  often to control a single variable rather than do individual callbacks,
  some of the Fl_Menu_Button methods are redescribed here in those terms.

  When the user clicks a menu item, value() is set to that item
  and then:

      - The item's callback is done if one has been set; the
        Fl_Choice is passed as the Fl_Widget* argument,
        along with any userdata configured for the callback.

      - If the item does not have a callback, the Fl_Choice widget's
        callback is done instead, along with any userdata configured
        for it.  The callback can determine which item was picked using
        value(), mvalue(), item_pathname(), etc.

  All three mouse buttons pop up the menu. The Forms behavior of the first
  two buttons to increment/decrement the choice is not implemented.  This
  could be added with a subclass, however.

  The menu will also pop up in response to shortcuts indicated by putting
  a '\&' character in the label().  See Fl_Button::shortcut(int s) for a
  description of this.

  Typing the shortcut() of any of the items will do exactly the same as when
  you pick the item with the mouse.  The '\&' character in item names are
  only looked at when the menu is popped up, however.

  The inherited Fl_Widget::changed() and related methods can be used as follows:
  \li <tt>int Fl_Widget::changed() const</tt>
      This value is true when the user picks a different value. <em>It is turned
      off by value() and just before doing a callback (the callback can turn
      it back on if desired).</em>
  \li <tt>void Fl_Widget::set_changed()</tt>
      This method sets the changed() flag.
  \li <tt>void Fl_Widget::clear_changed()</tt>
      This method clears the changed() flag.

 The inherited Fl_Menu_::down_box() methods can be used as follows:
  \li <tt>Fl_Boxtype Fl_Menu_::down_box() const</tt>
      Gets the current down box, which is used when the menu is popped up.
      The default down box type is \c FL_DOWN_BOX.
  \li <tt>void Fl_Menu_::down_box(Fl_Boxtype b)</tt>
      Sets the current down box type to \p b.

 Simple example:
 \par
 \code
   #include <FL/Fl.H>
   #include <FL/Fl_Window.H>
   #include <FL/Fl_Choice.H>
   int main() {
     Fl_Window *win = new Fl_Window(300,200);
     Fl_Choice *choice = new Fl_Choice(100,10,100,25,"Choice:");
     choice->add("Zero");
     choice->add("One");
     choice->add("Two");
     choice->add("Three");
     choice->value(2);    // make "Two" selected by default (zero based!)
     win->end();
     win->show();
     return Fl::run();
   }
 \endcode

 */
class FL_EXPORT Fl_Choice : public Fl_Menu_ {
protected:
  void draw() FL_OVERRIDE;
public:
  int handle(int) FL_OVERRIDE;

  Fl_Choice(int X, int Y, int W, int H, const char *L = 0);

  /**
    Gets the index of the last item chosen by the user.
    The index is -1 initially.
   */
  int value() const {return Fl_Menu_::value();}

  int value(int v);

  int value(const Fl_Menu_Item* v);
};

#endif
