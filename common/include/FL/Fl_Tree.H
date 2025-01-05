//

#ifndef FL_TREE_H
#define FL_TREE_H

#include <FL/Fl.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Scrollbar.H>
#include <FL/fl_draw.H>

#include <FL/Fl_Tree_Item.H>
#include <FL/Fl_Tree_Prefs.H>

//////////////////////
// FL/Fl_Tree.H
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
/// \brief This file contains the definitions of the Fl_Tree class
///

/** \class Fl_Tree

 \brief Tree widget.

 \image html tree-simple.png "Fl_Tree example program"
 \image latex tree-simple.png "Fl_Tree example program" width=4cm

 \code
 Fl_Tree                                         // Top level widget
    |--- Fl_Tree_Item                            // Items in the tree
    |--- Fl_Tree_Prefs                           // Preferences for the tree
              |--- Fl_Tree_Connector (enum)      // Connection modes
              |--- Fl_Tree_Select (enum)         // Selection modes
              |--- Fl_Tree_Sort (enum)           // Sort behavior
 \endcode

 Similar to Fl_Browser, Fl_Tree is a browser of Fl_Tree_Item's arranged
 in a parented hierarchy, or 'tree'. Subtrees can be expanded or closed.
 Items can be added, deleted, inserted, sorted and re-ordered.

 The tree items may also contain other FLTK widgets, like buttons, input fields,
 or even "custom" widgets.

 The callback() is invoked depending on the value of when():

 - FL_WHEN_RELEASE -- callback invoked when left mouse button is released on an item
 - FL_WHEN_CHANGED -- callback invoked when left mouse changes selection state

 The simple way to define a tree:
 \par
 \code
 #include <FL/Fl_Tree.H>
 [..]
 Fl_Tree tree(X,Y,W,H);
 tree.begin();
   tree.add("Flintstones/Fred");
   tree.add("Flintstones/Wilma");
   tree.add("Flintstones/Pebbles");
   tree.add("Simpsons/Homer");
   tree.add("Simpsons/Marge");
   tree.add("Simpsons/Bart");
   tree.add("Simpsons/Lisa");
 tree.end();
 \endcode

 \par FEATURES
 Items can be added with add(),<BR>
 removed with remove(),<BR>
 completely cleared with clear(),<BR>
 inserted with insert() and insert_above(),<BR>
 selected/deselected with select() and deselect(),<BR>
 open/closed with open() and close(),<BR>
 positioned on the screen with show_item_top(), show_item_middle() and
 show_item_bottom(),<BR>
 item children can be swapped around with Fl_Tree_Item::swap_children(),<BR>
 items can be moved around with Fl_Tree_Item::move(),<BR>
 an item's children can be walked with Fl_Tree_Item::first() and Fl_Tree_Item::next(),
 an item's children can be indexed directly with Fl_Tree_Item::child()
 and Fl_Tree_Item::children(),<BR>
 items can be moved from one subtree to another with Fl_Tree_Item::deparent()
 and Fl_Tree_Item::reparent(),<BR>
 sorting can be controlled when items are add()ed via sortorder().<BR>
 You can walk the entire tree with first() and next().<BR>
 You can walk visible items with first_visible_item()
 and next_visible_item().<BR>
 You can walk selected items with first_selected_item() and
 next_selected_item().<BR>
 Items can be found by their pathname using find_item(const char*),
 and an item's pathname can be found with item_pathname().<BR>
 The selected items' colors are controlled by selection_color()
 (inherited from Fl_Widget).<BR>
 A hook is provided to allow you to redefine how item's labels are drawn
 via Fl_Tree::item_draw_callback().<BR>
 Items can be interactively dragged using FL_TREE_SELECT_SINGLE_DRAGGABLE.

 \par SELECTION OF ITEMS
 The tree can have different selection behaviors controlled by selectmode().
 The background color used for selected items is the Fl_Tree::selection_color().
 The foreground color for selected items is controlled internally with fl_contrast().

 \par CHILD WIDGETS
 FLTK widgets (including custom widgets) can be assigned to tree items via
 Fl_Tree_Item::widget().
 \par
 When an Fl_Tree_Item::widget() is defined, the default behavior is for the
 widget() to be shown in place of the item's label (if it has one).
 Only the widget()'s width will be used; the widget()'s x() and y() position
 will be managed by the tree, and the h() will track the item's height.
 This default behavior can be altered (ABI 1.3.1):
 Setting Fl_Tree::item_draw_mode()'s FL_TREE_ITEM_DRAW_LABEL_AND_WIDGET flag
 causes the label + widget to be displayed together in that order, and
 adding the FL_TREE_ITEM_HEIGHT_FROM_WIDGET flag causes widget's height
 to define the widget()'s height.

 \par ICONS
 The tree's open/close icons can be redefined with
 Fl_Tree::openicon(), Fl_Tree::closeicon(). User icons
 can either be changed globally with Fl_Tree::usericon(),
 or on a per-item basis with Fl_Tree_Item::usericon().
 \par
 Various default preferences can be globally manipulated via Fl_Tree_Prefs,
 including colors, margins, icons, connection lines, etc.

 \par FONTS AND COLORS
 When adding new items to the tree, the new items get the
 defaults for fonts and colors from:
 \par
 - Fl_Tree::item_labelfont() -- The default item label font (default: FL_HELVETICA)
 - Fl_Tree::item_labelsize() -- The default item label size (default: FL_NORMAL_SIZE)
 - Fl_Tree::item_labelfgcolor() -- The default item label foreground color (default: FL_FOREGROUND_COLOR)
 - Fl_Tree::item_labelbgcolor() -- The default item label background color (default: 0xffffffff, which tree uses as 'transparent')
 \par
 Each item (Fl_Tree_Item) inherits a copy of these font/color attributes when created,
 and each item has its own methods to let the app change these values on a per-item basis
 using methods of the same name:
 \par
 - Fl_Tree_Item::labelfont() -- The item's label font (default: FL_HELVETICA)
 - Fl_Tree_Item::labelsize() -- The item's label size (default: FL_NORMAL_SIZE)
 - Fl_Tree_Item::labelfgcolor() -- The item's label foreground color (default: FL_FOREGROUND_COLOR)
 - Fl_Tree_Item::labelbgcolor() -- The item's label background color (default: 0xffffffff, which uses the tree's own bg color)

 \par CALLBACKS
 The tree's callback() will be invoked when items change state or are open/closed.
 when() controls when mouse/keyboard events invoke the callback.
 callback_item() and callback_reason() can be used to determine the cause of the callback. e.g.
 \par
 \code
 void MyTreeCallback(Fl_Widget *w, void *data) {
   Fl_Tree      *tree = (Fl_Tree*)w;
   Fl_Tree_Item *item = (Fl_Tree_Item*)tree->callback_item();   // get selected item
   switch ( tree->callback_reason() ) {
     case FL_TREE_REASON_SELECTED: [..]
     case FL_TREE_REASON_DESELECTED: [..]
     case FL_TREE_REASON_RESELECTED: [..]
     case FL_TREE_REASON_OPENED: [..]
     case FL_TREE_REASON_CLOSED: [..]
   }
   :
 }
 \endcode

 \par SIMPLE EXAMPLES
 To find all the selected items:
 \par
 \code
 for ( Fl_Tree_Item *i=first_selected_item(); i; i=next_selected_item(i) )
   printf("Item %s is selected\n", i->label());
 \endcode
 \par
     To get an item's full menu pathname, use Fl_Tree::item_pathname(), e.g.
 \par
 \code
 [..]
 char pathname[256] = "???";
 tree->item_pathname(pathname, sizeof(pathname), item);         // eg. "Parent/Child/Item"
 [..]
 \endcode
 \par
 To walk all the items of the tree from top to bottom:
 \par
 \code
 // Walk all the items in the tree, and print their labels
 for ( Fl_Tree_Item *item = tree->first(); item; item = tree->next(item) ) {
     printf("Item: %s\n", item->label());
 }
 \endcode
 \par
 To recursively walk all the children of a particular item,
 define a function that uses recursion:
 \par
 \code
 // Find all of the item's children and print an indented report of their labels
 void my_print_all_children(Fl_Tree_Item *item, int indent=0) {
     for ( int t=0; t<item->children(); t++ ) {
         printf("%*s Item: %s\n", indent, "", item->child(t)->label());
         my_print_all_children(item->child(t), indent+4);   // recurse
     }
 }
 \endcode
 \par
 To change the default label font and color when creating new items:
 \par
 \code
 tree = new Fl_Tree(..);
 tree->item_labelfont(FL_COURIER);      // Use Courier font for all new items
 tree->item_labelfgcolor(FL_RED);       // Use red color for labels of all new items
 [..]
 // Now create the items in the tree using the above defaults.
 tree->add("Aaa");
 tree->add("Bbb");
 \endcode
 \par
 To change the font and color of all existing items in the tree:
 \par
 \code
 // Change the font and color of all items currently in the tree
 for ( Fl_Tree_Item *item = tree->first(); item; item = tree->next(item) ) {
     item->labelfont(FL_COURIER);
     item->labelcolor(FL_RED);
 }
 \endcode

 \par DISPLAY DESCRIPTION
 The following image shows the tree's various visual elements
 and the methods that control them:
 \par
 \image html tree-elements.png
 \image latex tree-elements.png "Fl_Tree elements" width=6cm
 \par
 The following shows the protected dimension variables 'tree inner' (tix..)
 and 'tree outer' (tox..):
 \image html tree-dimensions.png "Fl_Tree inner/outer dimensions" width=6cm
 \image latex tree-dimensions.png "Fl_Tree inner/outer dimensions" width=6cm

 \par KEYBOARD BINDINGS
 The following table lists keyboard bindings for navigating the tree:
 \par
 Keyboard                | FL_TREE_SELECT_MULTI        | FL_TREE_SELECT_SINGLE       | FL_TREE_SELECT_NONE         |
 ------------------------|-----------------------------|-----------------------------|-----------------------------|
 Ctrl-A (Linux/Windows)  | Select all items            | N/A                         | N/A                         |
 Command-A (Mac)         | Select all items            | N/A                         | N/A                         |
 Space                   | Selects item                | Selects item                | N/A                         |
 Ctrl-Space              | Toggle item                 | Toggle item                 | N/A                         |
 Shift-Space             | Extends selection           | Selects item                | N/A                         |
 Enter                   | Toggles open/close          | Toggles open/close          | Toggles open/close          |
 Ctrl-Enter              | Toggles open/close          | Toggles open/close          | Toggles open/close          |
 Shift-Enter             | Toggles open/close          | Toggles open/close          | Toggles open/close          |
 Right / Left            | Open/Close item             | Open/Close item             | Open/Close item             |
 Up / Down               | Move focus box up/down      | Move focus box up/down      | N/A                         |
 Shift-Up / Shift-Down   | Extend selection up/down    | Move focus up/down          | N/A                         |
 Home / End              | Move to top/bottom of tree  | Move to top/bottom of tree  | Move to top/bottom of tree  |
 PageUp / PageDown       | Page up/down                | Page up/down                | Page up/down                |

*/

/// \enum Fl_Tree_Reason
/// The reason the callback was invoked.
///
enum Fl_Tree_Reason {
  FL_TREE_REASON_NONE       = FL_REASON_UNKNOWN,        ///< unknown reason
  FL_TREE_REASON_SELECTED   = FL_REASON_SELECTED,       ///< an item was selected
  FL_TREE_REASON_DESELECTED = FL_REASON_DESELECTED,     ///< an item was de-selected
  FL_TREE_REASON_RESELECTED = FL_REASON_RESELECTED,     ///< an item was re-selected (double-clicked).
                                ///< See ::Fl_Tree_Item_Reselect_Mode to enable this.
  FL_TREE_REASON_OPENED     = FL_REASON_OPENED,         ///< an item was opened
  FL_TREE_REASON_CLOSED     = FL_REASON_CLOSED,         ///< an item was closed
  FL_TREE_REASON_DRAGGED    = FL_REASON_DRAGGED         ///< an item was dragged into a new place
};

class FL_EXPORT Fl_Tree : public Fl_Group {
  friend class Fl_Tree_Item;
  Fl_Tree_Item  *_root;                         // can be null!
  Fl_Tree_Item  *_item_focus;                   // item that has focus box
  Fl_Tree_Item  *_callback_item;                // item invoked during callback (can be NULL)
  Fl_Tree_Reason _callback_reason;              // reason for the callback
  Fl_Tree_Prefs  _prefs;                        // all the tree's settings
  int            _scrollbar_size;               // size of scrollbar trough
  Fl_Tree_Item  *_lastselect;                   // last selected item
  char           _lastpushed;                   // FL_PUSH occurred on: 0=nothing, 1=open/close, 2=usericon, 3=label
  void fix_scrollbar_order();

protected:
  Fl_Scrollbar *_vscroll;       ///< Vertical scrollbar
  Fl_Scrollbar *_hscroll;       ///< Horizontal scrollbar
  int _tox,_toy,_tow,_toh;      ///< Tree widget outer xywh dimension: outside scrollbars, inside widget border
  int _tix,_tiy,_tiw,_tih;      ///< Tree widget inner xywh dimension: inside borders + scrollbars

  /// the calculated width of the entire tree hierarchy. See calc_tree()
  int _tree_w;
  /// the calculated height of the entire tree hierarchy. See calc_tree()
  int _tree_h;
  void item_clicked(Fl_Tree_Item* val);
  void do_callback_for_item(Fl_Tree_Item* item, Fl_Tree_Reason reason);

  // next_visible_item() and extend_selection() moved to 'public' in ABI 1.3.3
  // undocmented draw_tree() dropped -- draw() does all the work now

  // draw() has to be protected per FLTK convention (was public in 1.3.x)
  void draw() FL_OVERRIDE;

public:
  Fl_Tree(int X, int Y, int W, int H, const char *L=0);
  ~Fl_Tree();
  int handle(int e) FL_OVERRIDE;
  void show_self();
  void resize(int,int,int,int) FL_OVERRIDE;

  ///////////////////////
  // root methods
  ///////////////////////
  void root_label(const char *new_label);
  Fl_Tree_Item* root();
  void root(Fl_Tree_Item *newitem);
  const Fl_Tree_Prefs& prefs() const { return _prefs; }

  ////////////////////////////////
  // Item creation/removal methods
  ////////////////////////////////
  Fl_Tree_Item *add(const char *path, Fl_Tree_Item *newitem=0);
  Fl_Tree_Item* add(Fl_Tree_Item *parent_item, const char *name);
  Fl_Tree_Item *insert_above(Fl_Tree_Item *above, const char *name);
  Fl_Tree_Item* insert(Fl_Tree_Item *item, const char *name, int pos);
  int remove(Fl_Tree_Item *item);
  void clear();
  void clear_children(Fl_Tree_Item *item);

  ////////////////////////
  // Item lookup methods
  ////////////////////////
  Fl_Tree_Item *find_item(const char *path);
  const Fl_Tree_Item *find_item(const char *path) const;
  int item_pathname(char *pathname, int pathnamelen, const Fl_Tree_Item *item) const;
  const Fl_Tree_Item* find_clicked(int yonly=0) const;
  Fl_Tree_Item* find_clicked(int yonly=0);
  Fl_Tree_Item *item_clicked();
  Fl_Tree_Item *first();
  Fl_Tree_Item *first_visible();                // deprecated in ABI 10303
  Fl_Tree_Item *first_visible_item();
  Fl_Tree_Item *next(Fl_Tree_Item *item=0);
  Fl_Tree_Item *prev(Fl_Tree_Item *item=0);
  Fl_Tree_Item *last();
  Fl_Tree_Item *last_visible();                 // deprecated in ABI 10303
  Fl_Tree_Item *last_visible_item();
  Fl_Tree_Item *next_visible_item(Fl_Tree_Item *start, int dir);        // made public in 1.3.3 ABI
  Fl_Tree_Item *first_selected_item();
  Fl_Tree_Item *last_selected_item();
  Fl_Tree_Item *next_item(Fl_Tree_Item *item, int dir=FL_Down, bool visible=false);
  Fl_Tree_Item *next_selected_item(Fl_Tree_Item *item=0, int dir=FL_Down);
  int get_selected_items(Fl_Tree_Item_Array &items);

  //////////////////////////
  // Item open/close methods
  //////////////////////////
  int open(Fl_Tree_Item *item, int docallback=1);
  int open(const char *path, int docallback=1);
  void open_toggle(Fl_Tree_Item *item, int docallback=1);
  int close(Fl_Tree_Item *item, int docallback=1);
  int close(const char *path, int docallback=1);
  int is_open(Fl_Tree_Item *item) const;
  int is_open(const char *path) const;
  int is_close(Fl_Tree_Item *item) const;
  int is_close(const char *path) const;

  /////////////////////////
  // Item selection methods
  /////////////////////////
  int select(Fl_Tree_Item *item, int docallback=1);
  int select(const char *path, int docallback=1);
  void select_toggle(Fl_Tree_Item *item, int docallback=1);
  int deselect(Fl_Tree_Item *item, int docallback=1);
  int deselect(const char *path, int docallback=1);
  int deselect_all(Fl_Tree_Item *item=0, int docallback=1);
  int select_only(Fl_Tree_Item *selitem, int docallback=1);
  int select_all(Fl_Tree_Item *item=0, int docallback=1);
  int extend_selection_dir(Fl_Tree_Item *from,
                           Fl_Tree_Item *to,
                           int dir,
                           int val,
                           bool visible);
  int extend_selection(Fl_Tree_Item *from,
                       Fl_Tree_Item *to,
                       int val=1,
                       bool visible=false);
  void set_item_focus(Fl_Tree_Item *item);
  Fl_Tree_Item *get_item_focus() const;
  int is_selected(Fl_Tree_Item *item) const;
  int is_selected(const char *path);

  /////////////////////////////////
  // Item attribute related methods
  /////////////////////////////////
  Fl_Font     item_labelfont() const;
  void        item_labelfont(Fl_Font val);
  Fl_Fontsize item_labelsize() const;
  void        item_labelsize(Fl_Fontsize val);
  Fl_Color    item_labelfgcolor(void) const;
  void        item_labelfgcolor(Fl_Color val);
  Fl_Color    item_labelbgcolor(void) const;
  void        item_labelbgcolor(Fl_Color val);
  Fl_Color connectorcolor() const;
  void connectorcolor(Fl_Color val);
  int marginleft() const;
  void marginleft(int val);
  int margintop() const;
  void margintop(int val);
  int marginbottom() const;
  void marginbottom(int val);
  int linespacing() const;
  void linespacing(int val);
  int openchild_marginbottom() const;
  void openchild_marginbottom(int val);
  int usericonmarginleft() const;
  void usericonmarginleft(int val);
  int labelmarginleft() const;
  void labelmarginleft(int val);
  int widgetmarginleft() const;
  void widgetmarginleft(int val);
  int connectorwidth() const;
  void connectorwidth(int val);
  Fl_Image* usericon() const;
  void usericon(Fl_Image *val);
  Fl_Image* openicon() const;
  void openicon(Fl_Image *val);
  Fl_Image* closeicon() const;
  void closeicon(Fl_Image *val);
  int showcollapse() const;
  void showcollapse(int val);
  int showroot() const;
  void showroot(int val);
  Fl_Tree_Connector connectorstyle() const;
  void connectorstyle(Fl_Tree_Connector val);
  Fl_Tree_Sort sortorder() const;
  void sortorder(Fl_Tree_Sort val);
  Fl_Boxtype selectbox() const;
  void selectbox(Fl_Boxtype val);
  Fl_Tree_Select selectmode() const;
  void selectmode(Fl_Tree_Select val);
  Fl_Tree_Item_Reselect_Mode item_reselect_mode() const;
  void item_reselect_mode(Fl_Tree_Item_Reselect_Mode mode);
  Fl_Tree_Item_Draw_Mode item_draw_mode() const;
  void item_draw_mode(Fl_Tree_Item_Draw_Mode mode);
  void item_draw_mode(int mode);
  void calc_dimensions();
  void calc_tree();
  void recalc_tree();
  int displayed(Fl_Tree_Item *item);
  void show_item(Fl_Tree_Item *item, int yoff);
  void show_item(Fl_Tree_Item *item);
  void show_item_top(Fl_Tree_Item *item);
  void show_item_middle(Fl_Tree_Item *item);
  void show_item_bottom(Fl_Tree_Item *item);
  void display(Fl_Tree_Item *item);
  int  vposition() const;
  void vposition(int pos);
  int  hposition() const;
  void hposition(int pos);

  int is_scrollbar(Fl_Widget *w);
  int scrollbar_size() const;
  void scrollbar_size(int size);
  int is_vscroll_visible() const;
  int is_hscroll_visible() const;

  ///////////////////////
  // callback related
  ///////////////////////
  void callback_item(Fl_Tree_Item* item);
  Fl_Tree_Item* callback_item();
  void callback_reason(Fl_Tree_Reason reason);
  Fl_Tree_Reason callback_reason() const;

  /// Load FLTK preferences
  void load(class Fl_Preferences&);
};

#endif /*FL_TREE_H*/
