//
// Group header file for the Fast Light Tool Kit (FLTK).
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

/** \file
  \brief Fl_Group and Fl_End classes.
*/

#ifndef Fl_Group_H
#define Fl_Group_H

#include "Fl_Widget.H"

// Don't #include Fl_Rect.H because this would introduce lots
// of unnecessary dependencies on Fl_Rect.H
class Fl_Rect;


/**
  The Fl_Group class is the main FLTK container widget. It maintains
  an array of child widgets. These children can themselves be any widget
  including Fl_Group. The most important subclass of Fl_Group
  is Fl_Window, however groups can also be used to control radio buttons
  or to enforce resize behavior.

  The tab and arrow keys are used to move the focus between widgets of
  this group, and to other groups. The only modifier grabbed is shift
  (for shift-tab), so that ctrl-tab, alt-up, and such are free
  for the app to use as shortcuts.

  To remove a widget from the group and destroy it, in 1.3.x (and up)
  you can simply use:
  \code
  delete some_widget;
  \endcode
  ..and this will trigger proper scheduling of the widget's removal
  from its parent group.

  If used as a child of \p Fl_Tabs, setting \p when(FL_WHEN_CLOSED) will
  enable the Close button in the corresponding tab. If the user clicks the
  Close button, the callback of this group will be called with the callback
  reason \p FL_REASON_CLOSED.
*/
class FL_EXPORT Fl_Group : public Fl_Widget {

  union {
    Fl_Widget** array_; // used if group has two or more children or NULL
    Fl_Widget* child1_; // used if group has one child or NULL
  };
  Fl_Widget* savedfocus_;
  Fl_Widget* resizable_;
  int children_;
  Fl_Rect *bounds_; // remembered initial sizes of children
  int *sizes_; // remembered initial sizes of children (FLTK 1.3 compat.)

  int navigation(int);
  static Fl_Group *current_;

  // unimplemented copy ctor and assignment operator
  Fl_Group(const Fl_Group&);
  Fl_Group& operator=(const Fl_Group&);

protected:
  void draw() FL_OVERRIDE;
  void draw_child(Fl_Widget& widget) const;
  void draw_children();
  void draw_outside_label(const Fl_Widget& widget) const ;
  void update_child(Fl_Widget& widget) const;
  Fl_Rect *bounds();
  int  *sizes(); // FLTK 1.3 compatibility
  virtual int on_insert(Fl_Widget*, int);
  virtual int on_move(int, int);
  virtual void on_remove(int);

public:

  int handle(int) FL_OVERRIDE;
  void begin();
  void end();
  static Fl_Group *current();
  static void current(Fl_Group *g);

  /**
    Returns how many child widgets the group has.
  */
  int children() const { return children_; }

  /**
    Returns the n'th child.

    Returns \c NULL if \c n is out of range (since FLTK 1.4.0).

    <i>No range checking was done in FLTK 1.3 and older versions!</i>

    \param[in]  n   index of child (0 .. children() - 1)
    \return     pointer to the n'th child or NULL if out of range
  */
  Fl_Widget *child(int n) const {
    if (n < 0 || n > children() - 1) return NULL;
    return array()[n];
  }

  int find(const Fl_Widget*) const;
  /**
    See int Fl_Group::find(const Fl_Widget *w) const
  */
  int find(const Fl_Widget& o) const {return find(&o);}
  Fl_Widget* const* array() const;

  void resize(int,int,int,int) FL_OVERRIDE;
  /**
    Creates a new Fl_Group widget using the given position, size,
    and label string. The default boxtype is FL_NO_BOX.
  */
  Fl_Group(int,int,int,int, const char * = 0);
  virtual ~Fl_Group();
  void add(Fl_Widget&);
  /**
    See void Fl_Group::add(Fl_Widget &w)
  */
  void add(Fl_Widget* o) {add(*o);}
  void insert(Fl_Widget&, int i);
  /**
    This does insert(w, find(before)).  This will append the
    widget if \p before is not in the group.
  */
  void insert(Fl_Widget& o, Fl_Widget* before) {insert(o,find(before));}
  void remove(int index);
  void remove(Fl_Widget&);
  /**
    Removes the widget \p o from the group.
    \sa void remove(Fl_Widget&)
  */
  void remove(Fl_Widget* o) {remove(*o);}
  void clear();

  /* delete child n (by index) */
  virtual int delete_child(int n);

  /**
    Sets the group's resizable widget.
    See void Fl_Group::resizable(Fl_Widget *o)
  */
  void resizable(Fl_Widget& o) {resizable_ = &o;}
  /**
    The resizable widget defines both the resizing box and the resizing
    behavior of the group and its children.

    If the resizable is NULL the group's size is fixed and all of the widgets
    in the group remain a fixed size and distance from the top-left corner.
    This is the default for groups derived from Fl_Window and Fl_Pack.

    The resizable may be set to the group itself, in which case all of the
    widgets that are its direct children are resized proportionally.
    This is the default value for Fl_Group.

    The resizable widget defines the resizing box for the group, which could
    be the group itself or one of the group's direct children. When the
    group is resized it calculates a new size and position for all of its
    children. Widgets that are horizontally or vertically inside the
    dimensions of the box are scaled to the new size. Widgets outside the
    box are moved.

    \note The resizable of a group \b must be one of
      - NULL
      - the group itself
      - a direct child of the group.
    \note If you set any other widget that is not a direct child of the group as
      its resizable then the behavior is undefined. This is \b not checked by
      Fl_Group for historical reasons.

    In these examples the gray area is the resizable:

    \image html resizebox1.png

    <br>

    \image html resizebox2.png

    \image latex resizebox1.png "before resize"  width=4cm

    \image latex resizebox2.png "after resize"   width=4.85cm

    It is possible to achieve any type of resize behavior by using an
    invisible Fl_Box as the resizable and/or by using a hierarchy of
    Fl_Group widgets, each with their own resizing strategies.

    See the \ref resize chapter for more examples and detailed explanation.

    \note The resizable() widget of a window can also affect the window's
      resizing behavior if Fl_Window::size_range() is not called. Please
      see Fl_Window::default_size_range() for more information on how the
      default size range is calculated.

    \see Fl_Window::size_range()
    \see Fl_Window::default_size_range()
  */
  void resizable(Fl_Widget* o) {resizable_ = o;}
  /**
    Returns the group's resizable widget.
    See void Fl_Group::resizable(Fl_Widget *o)
  */
  Fl_Widget* resizable() const {return resizable_;}
  /**
    Adds a widget to the group and makes it the resizable widget.
  */
  void add_resizable(Fl_Widget& o) {resizable_ = &o; add(o);}
  void init_sizes();

  /**
    Controls whether the group widget clips the drawing of
    child widgets to its bounding box.

    Set \p c to 1 if you want to clip the child widgets to the
    bounding box.

    The default is to not clip (0) the drawing of child widgets.
  */
  void clip_children(int c) { if (c) set_flag(CLIP_CHILDREN); else clear_flag(CLIP_CHILDREN); }
  /**
    Returns the current clipping mode.

    \return true, if clipping is enabled, false otherwise.

    \see void Fl_Group::clip_children(int c)
  */
  unsigned int clip_children() { return (flags() & CLIP_CHILDREN) != 0; }

  // Note: Doxygen docs in Fl_Widget.H to avoid redundancy.
  Fl_Group* as_group() FL_OVERRIDE { return this; }
  Fl_Group const* as_group() const FL_OVERRIDE { return this; }

  // back compatibility functions:

  /**
    \deprecated This is for backwards compatibility only. You should use
    \e W->%take_focus() instead.
    \sa Fl_Widget::take_focus();
  */
  void focus(Fl_Widget* W) {W->take_focus();}

  /** This is for forms compatibility only */
  Fl_Widget* & _ddfdesign_kludge() {return resizable_;}

  /** This is for forms compatibility only */
  void forms_end();
};

// dummy class used to end child groups in constructors for complex
// subclasses of Fl_Group:
/**
  This is a dummy class that allows you to end a Fl_Group in a constructor list of a
  class:
  \code
  class MyClass {
    Fl_Group group;
    Fl_Button button_in_group;
    Fl_End end;
    Fl_Button button_outside_group;
    MyClass();
  };
  MyClass::MyClass() :
    group(10,10,100,100),
    button_in_group(20,20,60,30),
    end(),
    button_outside_group(10,120,60,30) {
    [..ctor code..]
  }
  \endcode
*/
class FL_EXPORT Fl_End {
public:
  /** All it does is calling Fl_Group::current()->end() */
  Fl_End() {Fl_Group::current()->end();}
};

#endif
