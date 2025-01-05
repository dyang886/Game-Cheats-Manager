//
// An input/chooser widget.
//            ______________  ____
//           |              || __ |
//           | input area   || \/ |
//           |______________||____|
//
// Copyright 2004 by Greg Ercolano.
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
   Fl_Input_Choice widget . */

#ifndef Fl_Input_Choice_H
#define Fl_Input_Choice_H

#include <FL/Fl.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Menu_Button.H>

/*
  A combination of the input widget and a menu button.

  The user can either type into the input area, or use the
  menu button chooser on the right to choose an item which loads
  the input area with the selected text.

  Note: doxygen docs in src/Fl_Input_Choice.cxx
*/

class FL_EXPORT Fl_Input_Choice : public Fl_Group {

  // Private class to handle slightly 'special' behavior of menu button
  class InputMenuButton : public Fl_Menu_Button {
    void draw() FL_OVERRIDE;
    const Fl_Menu_Item* popup();
  public:
    InputMenuButton(int X, int Y, int W, int H, const char *L=0);
    int handle(int e) FL_OVERRIDE;
  };

  Fl_Input *inp_;
  InputMenuButton *menu_;

  // note: this is used by the Fl_Input_Choice ctor.
  static void menu_cb(Fl_Widget*, void *data);

  // note: this is used by the Fl_Input_Choice ctor.
  static void inp_cb(Fl_Widget*, void *data);

protected:
  // Custom resize behavior -- input stretches, menu button doesn't

  /** The methods inp_x(), inp_y(), inp_w() and inp_h() return the
      desired position and size of the internal Fl_Input widget.
      These can be overridden by a subclass to redefine positioning.
      See code example in the Description for subclassing details.
  */
  virtual int inp_x() const { return(x() + Fl::box_dx(box())); }
  /** See inp_x() for info. */
  virtual int inp_y() const { return(y() + Fl::box_dy(box())); }
  /** See inp_x() for info. */
  virtual int inp_w() const { return(w() - Fl::box_dw(box()) - menu_w()); }
  /** See inp_x() for info. */
  virtual int inp_h() const { return(h() - Fl::box_dh(box())); }

  /** The methods menu_x(), menu_y(), menu_w() and menu_h() return the
      desired position and size of the internal Fl_Menu_Button widget.
      These can be overridden by a subclass to redefine positioning.
      See code example in the Description for subclassing details.
  */
  virtual int menu_x() const { return x() + w() - menu_w() - Fl::box_dx(box()); }
  /** See menu_x() for info. */
  virtual int menu_y() const { return y() + Fl::box_dy(box()); }
  /** See menu_x() for info. */
  virtual int menu_w() const { return 20; }
  /** See menu_x() for info. */
  virtual int menu_h() const { return h() - Fl::box_dh(box()); }

  void draw() FL_OVERRIDE;

public:

  Fl_Input_Choice(int X, int Y, int W, int H, const char *L=0);

  void resize(int X, int Y, int W, int H) FL_OVERRIDE;

  /** Adds an item to the menu.
    When any item is selected, the Fl_Input_Choice callback() is invoked,
    which can do something with the selected item.

    You can access the more complex Fl_Menu_Button::add() methods
    (setting item-specific callbacks, userdata, etc), via menubutton(). Example:
    \code
    Fl_Input_Choice *choice = new Fl_Input_Choice(100,10,120,25,"Fonts");
    Fl_Menu_Button *mb = choice->menubutton();             // use Fl_Input_Choice's Fl_Menu_Button
    mb->add("Helvetica", 0, MyFont_CB,     (void*)mydata); // use Fl_Menu_Button's add() methods
    mb->add("Courier",   0, MyFont_CB,     (void*)mydata);
    mb->add("More..",    0, FontDialog_CB, (void*)mydata);
    \endcode
  */
  void add(const char *s) { menu_->add(s); }

  /** Returns the combined changed() state of the input and menu button widget. */
  int changed() const { return inp_->changed() | Fl_Widget::changed(); }

  // Clears the changed() state of both input and menu button widgets.
  void clear_changed();

  // Sets the changed() state of both input and menu button widgets.
  void set_changed();

  /** Removes all items from the menu. */
  void clear() { menu_->clear(); }

  /** Gets the box type of the menu button */
  Fl_Boxtype down_box() const { return (menu_->down_box()); }

  /** Sets the box type of the menu button */
  void down_box(Fl_Boxtype b) { menu_->down_box(b); }

  /** Gets the Fl_Menu_Item array used for the menu. */
  const Fl_Menu_Item *menu() { return (menu_->menu()); }

  /** Sets the Fl_Menu_Item array used for the menu. */
  void menu(const Fl_Menu_Item *m) { menu_->menu(m); }

  /// Gets the Fl_Input text field's text color.
  Fl_Color textcolor() const { return (inp_->textcolor());}

  /// Sets the Fl_Input text field's text color to \p c.
  void textcolor(Fl_Color c) { inp_->textcolor(c);}

  /// Gets the Fl_Input text field's font style.
  Fl_Font textfont() const { return (inp_->textfont());}

  /// Sets the Fl_Input text field's font style to \p f.
  void textfont(Fl_Font f) { inp_->textfont(f);}

  /// Gets the Fl_Input text field's font size
  Fl_Fontsize textsize() const { return (inp_->textsize()); }

  /// Sets the Fl_Input text field's font size to \p s.
  void textsize(Fl_Fontsize s) { inp_->textsize(s); }

  /// Returns the Fl_Input text field's current contents.
  const char* value() const { return (inp_->value()); }

  /** Sets the Fl_Input text field's contents to \p val.

    Note it is possible to set the value() to one that is not
    in the menubutton's list of choices.

    Setting the value() does NOT affect the menubutton's selection.
    If that's needed, call update_menubutton() after setting value().

    \see void value(int val), update_menubutton()
  */
  void value(const char *val) { inp_->value(val); }

  /* Chooses item# \p val in the menu, and sets the Fl_Input text field
    to that value. Any previous text is cleared. */
  void value(int val);

  int update_menubutton();

  /** Returns a pointer to the internal Fl_Menu_Button widget.
    This can be used to access any of the methods of the menu button, e.g.
    \code
    Fl_Input_Choice *choice = new Fl_Input_Choice(100,10,120,25,"Choice:");
    [..]
    // Print all the items in the choice menu
    for ( int t=0; t<choice->menubutton()->size(); t++ ) {
       const Fl_Menu_Item &item = choice->menubutton()->menu()[t];
       printf("item %d -- label=%s\n", t, item.label() ? item.label() : "(Null)");
    }
    \endcode
  */
  Fl_Menu_Button *menubutton() { return menu_; }

  /** Returns a pointer to the internal Fl_Input widget.
    This can be used to directly access all of the Fl_Input widget's methods.
  */
  Fl_Input *input() { return inp_; }
};

#endif // !Fl_Input_Choice_H
