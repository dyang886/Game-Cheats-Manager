//

#ifndef FL_TREE_ITEM_H
#define FL_TREE_ITEM_H

#include <FL/Fl.H>
#include <FL/Fl_Widget.H>
#include <FL/Fl_Image.H>
#include <FL/fl_draw.H>

#include <FL/Fl_Tree_Item_Array.H>
#include <FL/Fl_Tree_Prefs.H>

//////////////////////
// FL/Fl_Tree_Item.H
//////////////////////
//
// Fl_Tree -- This file is part of the Fl_Tree widget for FLTK
// Copyright (C) 2009-2010 by Greg Ercolano.
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

///
/// \file
/// \brief This file contains the definitions for Fl_Tree_Item
///

/// \class Fl_Tree_Item
/// \brief Tree widget item.
///
/// This class is a single tree item, and manages all of the item's attributes.
/// Fl_Tree_Item is used by Fl_Tree, which is comprised of many instances of Fl_Tree_Item.
///
/// Fl_Tree_Item is hierarchical; it dynamically manages an Fl_Tree_Item_Array of children
/// that are themselves instances of Fl_Tree_Item. Each item can have zero or more children.
/// When an item has children, close() and open() can be used to hide or show them.
///
/// Items have their own attributes; font size, face, color.
/// Items maintain their own hierarchy of children.
///
/// When you make changes to items, you'll need to tell the tree to redraw()
/// for the changes to show up.
///
/// New 1.3.3 ABI feature:
/// You can define custom items by either adding a custom widget to the item
/// with Fl_Tree_Item::widget(), or override the draw_item_content() method
/// if you want to just redefine how the label is drawn.
///
/// The following shows the Fl_Tree_Item's dimensions, useful when overriding
/// the draw_item_content() method:
///
///   \image html  Fl_Tree_Item-dimensions.png "Fl_Tree_Item's internal dimensions." width=6cm
///   \image latex Fl_Tree_Item-dimensions.png "Fl_Tree_Item's internal dimensions." width=6cm
///
class Fl_Tree;
class FL_EXPORT Fl_Tree_Item {
  Fl_Tree                *_tree;                // parent tree
  const char             *_label;               // label (memory managed)
  Fl_Font                 _labelfont;           // label's font face
  Fl_Fontsize             _labelsize;           // label's font size
  Fl_Color                _labelfgcolor;        // label's fg color
  Fl_Color                _labelbgcolor;        // label's bg color (0xffffffff is 'transparent')
  /// \enum Fl_Tree_Item_Flags
  enum Fl_Tree_Item_Flags {
    OPEN                = 1<<0,         ///> item is open
    VISIBLE             = 1<<1,         ///> item is visible
    ACTIVE              = 1<<2,         ///> item is active
    SELECTED            = 1<<3          ///> item is selected
  };
  unsigned short _flags;                // misc flags
  int                     _xywh[4];             // xywh of this widget (if visible)
  int                     _collapse_xywh[4];    // xywh of collapse icon (if visible)
  int                     _label_xywh[4];       // xywh of label
  Fl_Widget              *_widget;              // item's label widget (optional)
  Fl_Image               *_usericon;            // item's user-specific icon (optional)
  Fl_Image               *_userdeicon;          // deactivated usericon
  Fl_Tree_Item_Array      _children;            // array of child items
  Fl_Tree_Item           *_parent;              // parent item (=0 if root)
  void                   *_userdata;            // user data that can be associated with an item
  Fl_Tree_Item           *_prev_sibling;        // previous sibling (same level)
  Fl_Tree_Item           *_next_sibling;        // next sibling (same level)
  // Protected methods
protected:
  void _Init(const Fl_Tree_Prefs &prefs, Fl_Tree *tree);
  void show_widgets();
  void hide_widgets();
  void draw_vertical_connector(int x, int y1, int y2, const Fl_Tree_Prefs &prefs);
  void draw_horizontal_connector(int x1, int x2, int y, const Fl_Tree_Prefs &prefs);
  void recalc_tree();
  int calc_item_height(const Fl_Tree_Prefs &prefs) const;
  Fl_Color drawfgcolor() const;
  Fl_Color drawbgcolor() const;

public:
  Fl_Tree_Item(const Fl_Tree_Prefs &prefs);     // CTOR -- backwards compatible
  Fl_Tree_Item(Fl_Tree *tree);                  // CTOR -- ABI 1.3.3+
  virtual ~Fl_Tree_Item();                      // DTOR -- ABI 1.3.3+
  Fl_Tree_Item(const Fl_Tree_Item *o);          // COPY CTOR
  /// The item's x position relative to the window
  int x() const { return(_xywh[0]); }
  /// The item's y position relative to the window
  int y() const { return(_xywh[1]); }
  /// The entire item's width to right edge of Fl_Tree's inner width
  /// within scrollbars.
  int w() const { return(_xywh[2]); }
  /// The item's height
  int h() const { return(_xywh[3]); }
  /// The item's label x position relative to the window
  /// \version 1.3.3
  int label_x() const { return(_label_xywh[0]); }
  /// The item's label y position relative to the window
  /// \version 1.3.3
  int label_y() const { return(_label_xywh[1]); }
  /// The item's maximum label width to right edge of Fl_Tree's inner width
  /// within scrollbars.
  /// \version 1.3.3
  int label_w() const { return(_label_xywh[2]); }
  /// The item's label height
  /// \version 1.3.3
  int label_h() const { return(_label_xywh[3]); }
  virtual int draw_item_content(int render);
  void draw(int X, int &Y, int W, Fl_Tree_Item *itemfocus,
            int &tree_item_xmax, int lastchild=1, int render=1);
  void show_self(const char *indent = "") const;
  void label(const char *val);
  const char *label() const;

  /// Set a user-data value for the item.
  inline void user_data( void* data ) { _userdata = data; }

  /// Retrieve the user-data value that has been assigned to the item.
  inline void* user_data() const { return _userdata; }

  /// Set item's label font face.
  void labelfont(Fl_Font val) {
    _labelfont = val;
    recalc_tree();              // may change tree geometry
  }
  /// Get item's label font face.
  Fl_Font labelfont() const {
    return(_labelfont);
  }
  /// Set item's label font size.
  void labelsize(Fl_Fontsize val) {
    _labelsize = val;
    recalc_tree();              // may change tree geometry
  }
  /// Get item's label font size.
  Fl_Fontsize labelsize() const {
    return(_labelsize);
  }
  /// Set item's label foreground text color.
  void labelfgcolor(Fl_Color val) {
    _labelfgcolor = val;
  }
  /// Return item's label foreground text color.
  Fl_Color labelfgcolor() const {
    return(_labelfgcolor);
  }
  /// Set item's label text color. Alias for labelfgcolor(Fl_Color)).
  void labelcolor(Fl_Color val) {
     labelfgcolor(val);
  }
  /// Return item's label text color. Alias for labelfgcolor() const).
  Fl_Color labelcolor() const {
    return labelfgcolor();
  }
  /// Set item's label background color.
  /// A special case is made for color 0xffffffff which uses the parent tree's bg color.
  void labelbgcolor(Fl_Color val) {
    _labelbgcolor = val;
  }
  /// Return item's label background text color.
  /// If the color is 0xffffffff, the default behavior is the parent tree's
  /// bg color will be used. (An overloaded draw_item_content() can override
  /// this behavior.)
  Fl_Color labelbgcolor() const {
    return(_labelbgcolor);
  }
  /// Assign an FLTK widget to this item.
  void widget(Fl_Widget *val) {
    _widget = val;
    recalc_tree();              // may change tree geometry
  }
  /// Return FLTK widget assigned to this item.
  Fl_Widget *widget() const {
    return(_widget);
  }
  /// Return the number of children this item has.
  int children() const {
    return(_children.total());
  }
  /// Return the child item for the given 'index'.
  Fl_Tree_Item *child(int index) {
    return(_children[index]);
  }
  /// Return the const child item for the given 'index'.
  const Fl_Tree_Item *child(int t) const;
  /// See if this item has children.
  int has_children() const {
    return(children());
  }
  int find_child(const char *name);
  int find_child(Fl_Tree_Item *item);
  int remove_child(Fl_Tree_Item *item);
  int remove_child(const char *new_label);
  void clear_children();
  void swap_children(int ax, int bx);
  int swap_children(Fl_Tree_Item *a, Fl_Tree_Item *b);
  const Fl_Tree_Item *find_child_item(const char *name) const;
        Fl_Tree_Item *find_child_item(const char *name);
  const Fl_Tree_Item *find_child_item(char **arr) const;
        Fl_Tree_Item *find_child_item(char **arr);
  const Fl_Tree_Item *find_item(char **arr) const;
        Fl_Tree_Item *find_item(char **arr);
  //////////////////
  // Adding items
  //////////////////
  Fl_Tree_Item *add(const Fl_Tree_Prefs &prefs,
                    const char *new_label,
                    Fl_Tree_Item *newitem);
  Fl_Tree_Item *add(const Fl_Tree_Prefs &prefs,
                    const char *new_label);
  Fl_Tree_Item *add(const Fl_Tree_Prefs &prefs,
                    char **arr,
                    Fl_Tree_Item *newitem);
  Fl_Tree_Item *add(const Fl_Tree_Prefs &prefs,
                    char **arr);
  Fl_Tree_Item *replace(Fl_Tree_Item *new_item);
  Fl_Tree_Item *replace_child(Fl_Tree_Item *olditem, Fl_Tree_Item *newitem);
  Fl_Tree_Item *insert(const Fl_Tree_Prefs &prefs, const char *new_label, int pos=0);
  Fl_Tree_Item *insert_above(const Fl_Tree_Prefs &prefs, const char *new_label);
  Fl_Tree_Item* deparent(int index);
  int reparent(Fl_Tree_Item *newchild, int index);
  int move(int to, int from);
  int move(Fl_Tree_Item *item, int op=0, int pos=0);
  int move_above(Fl_Tree_Item *item);
  int move_below(Fl_Tree_Item *item);
  int move_into(Fl_Tree_Item *item, int pos=0);
  int depth() const;
  Fl_Tree_Item *prev();
  Fl_Tree_Item *next();
  Fl_Tree_Item *next_sibling();
  Fl_Tree_Item *prev_sibling();
  void update_prev_next(int index);
  Fl_Tree_Item *next_displayed(Fl_Tree_Prefs &prefs);   // deprecated
  Fl_Tree_Item *prev_displayed(Fl_Tree_Prefs &prefs);   // deprecated
  Fl_Tree_Item *next_visible(Fl_Tree_Prefs &prefs);
  Fl_Tree_Item *prev_visible(Fl_Tree_Prefs &prefs);

  /// Return the parent for this item. Returns NULL if we are the root.
  Fl_Tree_Item *parent() {
    return(_parent);
  }
  /// Return the const parent for this item. Returns NULL if we are the root.
  const Fl_Tree_Item *parent() const {
    return(_parent);
  }
  /// Set the parent for this item.
  /// Should only be used by Fl_Tree's internals.
  ///
  void parent(Fl_Tree_Item *val) {
    _parent = val;
  }
  const Fl_Tree_Prefs& prefs() const;
  /// Return the tree for this item.
  /// \version 1.3.3
  const Fl_Tree *tree() const {
    return(_tree);
  }
  /// Return the tree for this item.
  /// \version 1.3.4
  Fl_Tree *tree() {
    return(_tree);
  }
  //////////////////
  // State
  //////////////////
  void open();
  void close();
  /// See if the item is 'open'.
  int is_open() const {
    return(is_flag(OPEN));
  }
  /// See if the item is 'closed'.
  int is_close() const {
    return(is_flag(OPEN)?0:1);
  }
  /// Toggle the item's open/closed state.
  void open_toggle() {
    is_open()?close():open();   // handles calling recalc_tree()
  }
  /// Change the item's selection state to the optionally specified 'val'.
  /// If 'val' is not specified, the item will be selected.
  ///
  void select(int val=1) {
    set_flag(SELECTED, val);
  }
  /// Toggle the item's selection state.
  void select_toggle() {
    if ( is_selected() ) {
      deselect();       // deselect if selected
    } else {
      select();         // select if deselected
    }
  }
  /// Select item and all its children.
  ///     Returns count of how many items were in the 'deselected' state,
  ///     ie. how many items were "changed".
  ///
  int select_all() {
    int count = 0;
    if ( ! is_selected() ) {
      select();
      ++count;
    }
    for ( int t=0; t<children(); t++ ) {
      count += child(t)->select_all();
    }
    return(count);
  }
  /// Disable the item's selection state.
  void deselect() {
    set_flag(SELECTED, 0);
  }
  /// Deselect item and all its children.
  ///     Returns count of how many items were in the 'selected' state,
  ///     ie. how many items were "changed".
  ///
  int deselect_all() {
    int count = 0;
    if ( is_selected() ) {
      deselect();
      ++count;
    }
    for ( int t=0; t<children(); t++ ) {
      count += child(t)->deselect_all();
    }
    return(count);
  }
  /// See if the item is selected.
  char is_selected() const {
    return(is_flag(SELECTED));
  }
  /// Change the item's activation state to the optionally specified 'val'.
  ///
  /// When deactivated, the item will be 'grayed out'; the callback()
  /// won't be invoked if the user clicks on the label. If a widget()
  /// is associated with the item, its activation state will be changed as well.
  ///
  /// If 'val' is not specified, the item will be activated.
  ///
  void activate(int val=1) {
    set_flag(ACTIVE,val);
    if ( _widget && val != (int)_widget->active() ) {
      if ( val ) {
        _widget->activate();
      } else {
        _widget->deactivate();
      }
      _widget->redraw();
    }
  }
  /// Deactivate the item; the callback() won't be invoked when clicked.
  /// Same as activate(0)
  ///
  void deactivate() {
    activate(0);
  }
  /// See if the item is activated.
  char is_activated() const {
    return(is_flag(ACTIVE));
  }
  /// See if the item is activated. Alias for is_activated().
  char is_active() const {
    return(is_activated());
  }
  /// See if the item is visible. Alias for is_visible().
  int visible() const {
    return(is_visible());
  }
  /// See if the item is visible.
  int is_visible() const {
    return(is_flag(VISIBLE));
  }
  /// See if item and all its parents are open() and visible().
  /// Alias for is_visible_r().
  /// \returns
  ///    1 -- item and its parents are open() and visible()
  ///    0 -- item (or one of its parents) are not visible or close()ed.
  ///
  int visible_r() const {
    return(is_visible_r());
  }
  int is_visible_r() const;

  /// Set the item's user icon to an Fl_Image. Use '0' to disable.
  /// No internal copy is made, caller must manage icon's memory.
  ///
  /// Note, if you expect your items to be deactivated(),
  /// use userdeicon(Fl_Image*) to set up a 'grayed out' version of your icon
  /// to be used for display.
  ///
  /// \see userdeicon(Fl_Image*)
  ///
  void usericon(Fl_Image *val) {
    _usericon = val;
    recalc_tree();              // may change tree geometry
  }
  /// Get the item's user icon as an Fl_Image. Returns '0' if disabled.
  Fl_Image *usericon() const {
    return(_usericon);
  }
  /// Set the usericon to draw when the item is deactivated. Use '0' to disable.
  /// No internal copy is made; caller must manage icon's memory.
  ///
  /// To create a typical 'grayed out' version of your usericon image,
  /// you can do the following:
  ///
  /// \code
  ///      // Create tree + usericon for items
  ///      Fl_Tree *tree = new Fl_Tree(..);
  ///      Fl_Image *usr_icon = new Fl_Pixmap(..); // your usericon
  ///      Fl_Image *de_icon  = usr_icon->copy();  // make a copy, and..
  ///      de_icon->inactive();                    // make it 'grayed out'
  ///      ...
  ///      for ( .. ) {                 // item loop..
  ///        item = tree->add("...");   // create new item
  ///        item->usericon(usr_icon);  // assign usericon to items
  ///        item->userdeicon(de_icon); // assign userdeicon to items
  ///        ..
  ///      }
  /// \endcode
  ///
  /// In the above example, the app should 'delete' the two icons
  /// when they're no longer needed (e.g. after the tree is destroyed)
  ///
  /// \version 1.3.4
  ///
  void userdeicon(Fl_Image* val) {
    _userdeicon = val;
  }
  /// Return the deactivated version of the user icon, if any.
  /// Returns 0 if none.
  Fl_Image* userdeicon() const {
    return _userdeicon;
  }
  //////////////////
  // Events
  //////////////////
  const Fl_Tree_Item* find_clicked(const Fl_Tree_Prefs &prefs, int yonly=0) const;
  Fl_Tree_Item* find_clicked(const Fl_Tree_Prefs &prefs, int yonly=0);
  int event_on_item(const Fl_Tree_Prefs &prefs) const;
  int event_on_collapse_icon(const Fl_Tree_Prefs &prefs) const;
  int event_on_user_icon(const Fl_Tree_Prefs &prefs) const;
  int event_on_label(const Fl_Tree_Prefs &prefs) const;
  /// Is this item the root of the tree?
  int is_root() const {
    return(_parent==0?1:0);
  }

  // Protected methods
  // TODO: move these to top 'protected:' section
protected:
  /// Set a flag to an on or off value. val is 0 or 1.
  inline void set_flag(unsigned short flag,int val) {
    if ( flag==OPEN || flag==VISIBLE ) {
      recalc_tree();            // may change tree geometry
    }
    if ( val ) _flags |= flag; else _flags &= ~flag;
  }
  /// See if flag set. Returns 0 or 1.
  inline int is_flag(unsigned short val) const {
    return(_flags & val ? 1 : 0);
  }

};

#endif /*FL_TREE_ITEM_H*/
