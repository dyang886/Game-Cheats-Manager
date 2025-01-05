//
// Menu base class header file for the Fast Light Tool Kit (FLTK).
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
   Fl_Menu_ widget . */

#ifndef Fl_Menu__H
#define Fl_Menu__H

#ifndef Fl_Widget_H
#include "Fl_Widget.H"
#endif
#include "Fl_Menu_Item.H"

/**
  Base class of all widgets that have a menu in FLTK.

  Currently FLTK provides you with Fl_Menu_Button, Fl_Menu_Bar, and Fl_Choice.

  The class contains a pointer to an array of structures of type Fl_Menu_Item.
  The array may either be supplied directly by the user program, or it may
  be "private": a dynamically allocated array managed by the Fl_Menu_.

  When the user clicks a menu item, value() is set to that item
  and then:

    - If the Fl_Menu_Item has a callback set, that callback
      is invoked with any userdata configured for it.
      (The Fl_Menu_ widget's callback is NOT invoked.)

    - For any Fl_Menu_Items that \b don't have a callback set,
      the Fl_Menu_ widget's callback is invoked with any userdata
      configured for it. The callback can determine which item
      was picked using value(), mvalue(), item_pathname(), etc.

  The line spacing between menu items can be controlled with the global setting
  Fl::menu_linespacing().
 \see Fl_Widget::shortcut_label(int)
*/
class FL_EXPORT Fl_Menu_ : public Fl_Widget {

  Fl_Menu_Item *menu_;
  const Fl_Menu_Item *value_;
  const Fl_Menu_Item *prev_value_;

protected:

  uchar alloc;                  // flag indicates if menu_ is a dynamic copy (=1) or not (=0)
  uchar down_box_;
  Fl_Boxtype menu_box_;
  Fl_Font textfont_;
  Fl_Fontsize textsize_;
  Fl_Color textcolor_;

  int item_pathname_(char *name, int namelen, const Fl_Menu_Item *finditem,
                     const Fl_Menu_Item *menu=0) const;
public:
  Fl_Menu_(int,int,int,int,const char * =0);
  ~Fl_Menu_();

  int item_pathname(char *name, int namelen, const Fl_Menu_Item *finditem=0) const;
  const Fl_Menu_Item* picked(const Fl_Menu_Item*);
  const Fl_Menu_Item* find_item(const char *name);
  const Fl_Menu_Item* find_item(Fl_Callback*);
  const Fl_Menu_Item* find_item_with_user_data(void*);
  const Fl_Menu_Item* find_item_with_argument(long);
  int find_index(const char *name) const;
  int find_index(const Fl_Menu_Item *item) const;
  int find_index(Fl_Callback *cb) const;

  /**
    Returns the menu item with the entered shortcut (key value).

    This searches the complete menu() for a shortcut that matches the
    entered key value.  It must be called for a FL_KEYBOARD or FL_SHORTCUT
    event.

    If a match is found, the menu's callback will be called.

    \return matched Fl_Menu_Item or NULL.
  */
  const Fl_Menu_Item* test_shortcut() {return picked(menu()->test_shortcut());}
  void global();

  /**
    Returns a pointer to the array of Fl_Menu_Items.  This will either be
    the value passed to menu(value) or the private copy or an internal
    (temporary) location (see note below).

    \note <b>Implementation details - may be changed in the future.</b>
      All modifications of the menu array are done by copying the entire
      menu array to an internal storage for optimization of memory
      allocations, for instance when using add() or insert(). While this
      is done, menu() returns the pointer to this internal location. The
      entire menu will be copied back to private storage when needed,
      i.e. when \b another Fl_Menu_ is modified. You can force this
      reallocation after you're done with all menu modifications by calling
      Fl_Menu_::menu_end() to make sure menu() returns a permanent pointer
      to private storage (until the menu is modified again).
      Note also that some menu methods (e.g. Fl_Menu_Button::popup()) call
      menu_end() internally to ensure a consistent menu array while the
      menu is open.

    \see size() -- returns the size of the Fl_Menu_Item array.
    \see menu_end() -- finish %menu modifications (optional)

    \b Example: How to walk the array:
    \code
    for ( int t=0; t<menubar->size(); t++ ) {                // walk array of items
        const Fl_Menu_Item &item = menubar->menu()[t];       // get each item
        fprintf(stderr, "item #%d -- label=%s, value=%s type=%s\n",
            t,
            item.label() ? item.label() : "(Null)",          // menu terminators have NULL labels
            (item.flags & FL_MENU_VALUE) ? "set" : "clear",  // value of toggle or radio items
            (item.flags & FL_SUBMENU) ? "Submenu" : "Item"); // see if item is a submenu or actual item
    }
    \endcode

  */
  const Fl_Menu_Item *menu() const {return menu_;}
  const Fl_Menu_Item *menu_end(); // in src/Fl_Menu_add.cxx
  void menu(const Fl_Menu_Item *m);
  void copy(const Fl_Menu_Item *m, void* user_data = 0);
  int insert(int index, const char*, int shortcut, Fl_Callback*, void* = 0, int = 0);
  int  add(const char*, int shortcut, Fl_Callback*, void* = 0, int = 0); // see src/Fl_Menu_add.cxx
  /** See int Fl_Menu_::add(const char* label, int shortcut, Fl_Callback*, void *user_data=0, int flags=0) */
  int  add(const char* a, const char* b, Fl_Callback* c, void* d = 0, int e = 0) {
      return add(a,fl_old_shortcut(b),c,d,e);
  }
  /** See int Fl_Menu_::insert(const char* label, int shortcut, Fl_Callback*, void *user_data=0, int flags=0) */
  int insert(int index, const char* a, const char* b, Fl_Callback* c, void* d = 0, int e = 0) {
      return insert(index,a,fl_old_shortcut(b),c,d,e);
  }
  int  add(const char *);
  int  size() const ;
  void size(int W, int H) { Fl_Widget::size(W, H); }
  void clear();
  int clear_submenu(int index);
  void replace(int,const char *);
  void remove(int);
  /** Change the shortcut of item \p i to \p s. */
  void shortcut(int i, int s) {menu_[i].shortcut(s);}
  /** Set the flags of item i.  For a list of the flags, see Fl_Menu_Item.  */
  void mode(int i,int fl) {menu_[i].flags = fl;}
  /** Get the flags of item i.  For a list of the flags, see Fl_Menu_Item.  */
  int  mode(int i) const {return menu_[i].flags;}

  /** Return a pointer to the last menu item that was picked.  */
  const Fl_Menu_Item *mvalue() const {return value_;}

  /** Return a pointer to the menu item that was picked before the current one was picked.
    This call gives developers additional details how a user changed a choice in the Fl_Choice widget.
  */
  const Fl_Menu_Item *prev_mvalue() const {return prev_value_;}
  // Return the index into the menu() of the last item chosen by the user or -1.
  int value() const;
  // Set the internal value_ of the menu to the given Fl_Menu_Item.
  int value(const Fl_Menu_Item*);
  /**
    Set the value of the menu to index \c i.

    The \e value of the menu is the index into the menu() of the last
    item chosen by the user or -1.

    It is \c -1 initially (if no item has been chosen) or if the chosen
    menu item is part of a submenu addressed by an FL_SUBMENU_POINTER.

    \note All menu items are located in a contiguous array of Fl_Menu_Item's
      unless an item has the FL_SUBMENU_POINTER flag which redirects the
      submenu to an independent submenu array. This submenu array is not
      counted in the size() of the menu, and menu items in this submenu can't
      return a valid index into the \b main menu. Therefore menu items that
      are located in such a submenu return -1 when value() is called.
      This may be changed in a future version.

    You can set the value as an integer or with a pointer to a menu item.
    The integer value is restricted to the main menu array (0..size()-1)
    whereas the menu item can be any menu item, even one in a detached
    submenu (see note about FL_SUBMENU_POINTER above).

    \param[in] i  Index of the menu item in the main menu array.\n
                  Values outside 0..size()-1 are ignored (return 0).

    \return   Whether the new value is different than the old one.
    \retval 0 The value didn't change.
    \retval 1 The value was changed.

    \see int value(const Fl_Menu_Item*)
    \see int value()
    \see const Fl_Menu_Item *mvalue()
  */
  int value(int i) {
    if (!menu_ || i < 0 || i >= size())
      return 0;
    return value(menu_ + i);
  }

  /** Returns the title of the last item chosen.  */
  const char *text() const {return value_ ? value_->text : 0;}
  /** Returns the title of item i.  */
  const char *text(int i) const {return menu_[i].text;}

  /** Gets the current font of menu item labels.  */
  Fl_Font textfont() const {return textfont_;}
  /**  Sets the current font of menu item labels.  */
  void textfont(Fl_Font c) {textfont_=c;}
  /**  Gets the font size of menu item labels.  */
  Fl_Fontsize textsize() const {return textsize_;}
  /**  Sets the font size of menu item labels.  */
  void textsize(Fl_Fontsize c) {textsize_=c;}
  /** Get the current color of menu item labels.  */
  Fl_Color textcolor() const {return textcolor_;}
  /** Sets the current color of menu item labels. */
  void textcolor(Fl_Color c) {textcolor_=c;}

  /**
    This box type is used to surround the currently-selected items in the
    menus.
  */
  Fl_Boxtype down_box() const {return (Fl_Boxtype)down_box_;}
  /** Sets the box type used to surround the currently-selected items in the menus.  */
  void down_box(Fl_Boxtype b) {down_box_ = b;}

  /** Get the box type for the menu popup windows.
   \return the box type, or FL_NO_BOX if Fl_Menu_::box() is to be used instead
   */
  Fl_Boxtype menu_box() const { return menu_box_; }
  /** Set the box type for the menu popup windows.
   If menu_box set to FL_NO_BOX, the menu window will use Fl_Menu_::box() instead.
   \param[in] b new box type or FL_NO_BOX
   */
  void menu_box(Fl_Boxtype b) { menu_box_ = b; }

  /** For back compatibility, same as selection_color() */
  Fl_Color down_color() const {return selection_color();}
  /** For back compatibility, same as selection_color() */
  void down_color(unsigned c) {selection_color(c);}
  void setonly(Fl_Menu_Item* item);
};

#endif
