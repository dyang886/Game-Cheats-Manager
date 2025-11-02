//
// Common browser header file for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2016 by Bill Spitzak and others.
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
   Fl_Browser_ widget . */

// Yes, I know this should be a template...

#ifndef Fl_Browser__H
#define Fl_Browser__H

#ifndef Fl_Group_H
#include "Fl_Group.H"
#endif
#include "Fl_Scrollbar.H"
#include <FL/Fl.H>              // Fl::scrollbar_size()

#define FL_NORMAL_BROWSER       0       /**< type() of Fl_Browser */
#define FL_SELECT_BROWSER       1       /**< type() of FL_Select_Browser */
#define FL_HOLD_BROWSER         2       /**< type() of Fl_Hold_Browser */
#define FL_MULTI_BROWSER        3       /**< type() of Fl_Multi_Browser */

#define FL_SORT_ASCENDING       0       /**< sort browser items in ascending alphabetic order. */
#define FL_SORT_DESCENDING      1       /**< sort in descending order */
#define FL_SORT_CASEINSENSITIVE 0x2     /**< sort case insensitively */

/**
  This is the base class for browsers.  To be useful it must be
  subclassed and several virtual functions defined.  The Forms-compatible
  browser and the file chooser's browser are subclassed off of this.

  This has been designed so that the subclass has complete control
  over the storage of the data, although because next() and
  prev() functions are used to index, it works best as a linked list
  or as a large block of characters in which the line breaks must be
  searched for.

  A great deal of work has been done so that the "height" of a data
  object does not need to be determined until it is drawn.  This is
  useful if actually figuring out the size of an object requires
  accessing image data or doing stat() on a file or doing some
  other slow operation.

  Callbacks are called when the value changes with \p FL_REASON_CHANGED.
  If \p FL_WHEN_RELEASE is set, callbacks are called when the mouse button is
  released with \p FL_REASON_CHANGED or \p FL_REASON_RESELECTED if the selection
  did not change. If \p FL_WHEN_ENTER_KEY is set, callbacks are also called when
  key presses or double clicks change the selection.

Keyboard navigation of browser items
------------------------------------

  The keyboard navigation of browser items is only possible if
  visible_focus() is enabled. If disabled, the widget rejects keyboard focus;
  Tab and Shift-Tab focus navigation will skip the widget.

  In 'Select' and 'Normal' mode, the widget rejects keyboard focus;
  no navigation keys are supported (other than scrollbar positioning).

  In 'Hold' mode, the widget accepts keyboard focus, and Up/Down arrow
  keys can navigate the selected item.

  In 'Multi' mode, the widget accepts keyboard focus, and Up/Down arrow
  keys navigate the focus box; Space toggles the current item's selection,
  Enter selects only the current item (deselects all others).  If Shift
  (or Ctrl) is combined with Up/Down arrow keys, the current item's
  selection state is extended to the next item. In this way one can
  extend a selection or de-selection.
*/
class FL_EXPORT Fl_Browser_ : public Fl_Group {
  int position_;        // where user wants it scrolled to
  int real_position_;   // the current vertical scrolling position
  int hposition_;       // where user wants it panned to
  int real_hposition_;  // the current horizontal scrolling position
  int offset_;          // how far down top_ item the real_position is
  int max_width;        // widest object seen so far
  uchar has_scrollbar_; // which scrollbars are enabled
  Fl_Font textfont_;
  Fl_Fontsize textsize_;
  Fl_Color textcolor_;
  void* top_;           // which item scrolling position is in
  void* selection_;     // which is selected (except for FL_MULTI_BROWSER)
  void *redraw1,*redraw2; // minimal update pointers
  void* max_width_item; // which item has max_width_
  int scrollbar_size_;  // size of scrollbar trough
  int linespacing_;

  void update_top();

protected:

  // All of the following must be supplied by the subclass:
  /**
    This method must be provided by the subclass
    to return the first item in the list.
    \see item_first(), item_next(), item_last(), item_prev()
   */
  virtual void *item_first() const = 0;
  /**
    This method must be provided by the subclass
    to return the item in the list after \p item.
    \see item_first(), item_next(), item_last(), item_prev()
   */
  virtual void *item_next(void *item) const = 0;
  /**
    This method must be provided by the subclass
    to return the item in the list before \p item.
    \see item_first(), item_next(), item_last(), item_prev()
   */
  virtual void *item_prev(void *item) const = 0;
  /**
    This method must be provided by the subclass
    to return the last item in the list.
    \see item_first(), item_next(), item_last(), item_prev()
   */
  virtual void *item_last() const { return 0L; }
  /**
    This method must be provided by the subclass to return
    the height of \p item in pixels.
    Allow for two additional pixels for the list selection box.
    \param[in] item The item whose height is returned.
    \returns The height of the specified \p item in pixels.
    \see item_height(), item_width(), item_quick_height()
  */
  virtual int item_height(void *item) const = 0;
  /**
    This method must be provided by the subclass to return the width of the
    \p item in pixels.  Allow for two additional pixels for the list
    selection box.
    \param[in] item The item whose width is returned.
    \returns The width of the item in pixels.
  */
  virtual int item_width(void *item) const = 0;
  virtual int item_quick_height(void *item) const ;
  /**
    This method must be provided by the subclass to draw the \p item
    in the area indicated by \p X, \p Y, \p W, \p H.
  */
  virtual void item_draw(void *item,int X,int Y,int W,int H) const = 0;
  /**
    This optional method returns a string (label) that may be used for sorting.
    \param[in] item The item whose label text is returned.
    \returns The item's text label. (Can be NULL if blank)
   */
  virtual const char *item_text(void *item) const { (void)item; return 0L; }
  /**
    This optional method should be provided by the subclass
    to efficiently swap browser items \p a and \p b, such as for sorting.
    \param[in] a,b The two items to be swapped.
   */
  virtual void item_swap(void *a,void *b) { (void)a; (void)b; }
  /**
    This method must be provided by the subclass
    to return the item for the specified \p index.
    \param[in] index The \p index of the item to be returned
    \returns The item at the specified \p index.
   */
  virtual void *item_at(int index) const { (void)index; return 0L; }
  // you don't have to provide these but it may help speed it up:
  virtual int full_width() const ;      // current width of all items
  virtual int full_height() const ;     // current height of all items
  virtual int incr_height() const ;     // average height of an item
  // These only need to be done by subclass if you want a multi-browser:
  virtual void item_select(void *item,int val=1);
  virtual int item_selected(void *item) const ;

  // things the subclass may want to call:
  /**
    Returns the item that appears at the top of the list.
   */
  void *top() const { return top_; }
  /**
    Returns the item currently selected, or NULL if there is no selection.

    For multiple selection browsers this call returns the currently focused item,
    even if it is not selected. To find all selected items, call
    Fl_Multi_Browser::selected() for every item in question.
  */
  void *selection() const { return selection_; }
  void new_list(); // completely clobber all data, as though list replaced
  void deleting(void *item); // get rid of any pointers to item
  void replacing(void *a,void *b); // change a pointers to b
  void swapping(void *a,void *b); // exchange pointers a and b
  void inserting(void *a,void *b); // insert b near a
  int displayed(void *item) const ; // true if this item is visible
  void redraw_line(void *item); // minimal update, no change in size
  /**
    This method will cause the entire list to be redrawn.
    \see redraw_lines(), redraw_line()
   */
  void redraw_lines() { damage(FL_DAMAGE_SCROLL); } // redraw all of them
  void bbox(int &X,int &Y,int &W,int &H) const;
  int leftedge() const; // x position after scrollbar & border
  void *find_item(int ypos); // item under mouse

  void draw() FL_OVERRIDE;
  Fl_Browser_(int X,int Y,int W,int H,const char *L=0);

public:

  /**
    Vertical scrollbar. Public, so that it can be accessed directly.

    Use `scrollbar_left()` or `scrollbar_right()` to change what side
    the vertical scrollbar is drawn on.

    Use `scrollbar.align(int)` (see `Fl_Widget::align(Fl_Align)`)
    to change what side either of the scrollbars is drawn on.

    If the `FL_ALIGN_LEFT` bit is on, the vertical scrollbar is on
    the left. If the `FL_ALIGN_TOP` bit is on, the horizontal
    scrollbar is on the top. Note that only the alignment flags in
    scrollbar are considered. The flags in hscrollbar however
    are ignored.
   */
  Fl_Scrollbar scrollbar;
  /**
    Horizontal scrollbar. Public, so that it can be accessed directly.
   */
  Fl_Scrollbar hscrollbar;

  int handle(int event) FL_OVERRIDE;
  void resize(int X,int Y,int W,int H) FL_OVERRIDE;

  int select(void *item,int val=1,int docallbacks=0);
  int select_only(void *item,int docallbacks=0);
  int deselect(int docallbacks=0);
  /**
    Gets the vertical scroll position of the list as a pixel position \p pos.
    The position returned is how many pixels of the list are scrolled off the top edge
    of the screen.  Example: A position of '3' indicates the top 3 pixels of
    the list are scrolled off the top edge of the screen.
    \see position(), hposition()
  */
  int vposition() const { return position_; }
  FL_DEPRECATED("since 1.4.0 - use vposition() instead",
  int position() const) { return vposition(); }

  void vposition(int pos); // scroll to here
  FL_DEPRECATED("since 1.4.0 - use vposition(pos) instead",
  void position(int pos)) { return vposition(pos); }
  void position(int x, int y) { Fl_Group::position(x, y); }

  /**
    Gets the horizontal scroll position of the list as a pixel position \p pos.
    The position returned is how many pixels of the list are scrolled off the left edge
    of the screen. Example: A position of '18' indicates the left 18 pixels of
    the list are scrolled off the left edge of the screen.
    \see position(), hposition()
  */
  int hposition() const { return hposition_; }
  void hposition(int); // pan to here
  void display(void *item); // scroll so this item is shown

  /**
    Values for has_scrollbar().
   */
  /** Anonymous enum bit flags for has_scrollbar().
     -  bit 0: horizontal
     -  bit 1: vertical
     -  bit 2: 'always' (to be combined with bits 0 and 1)
     -  bit 3-31: reserved for future use
   */
  enum { // values for has_scrollbar()
    HORIZONTAL = 1,             ///< Only show horizontal scrollbar.
    VERTICAL = 2,               ///< Only show vertical scrollbar.
    BOTH = 3,                   ///< Show both scrollbars. (default)
    ALWAYS_ON = 4,              ///< Specified scrollbar(s) should 'always' be shown (to be used with HORIZONTAL/VERTICAL)
    HORIZONTAL_ALWAYS = 5,      ///< Horizontal scrollbar always on.
    VERTICAL_ALWAYS = 6,        ///< Vertical scrollbar always on.
    BOTH_ALWAYS = 7             ///< Both scrollbars always on.
  };
  /**
    Returns the current scrollbar mode, see Fl_Browser_::has_scrollbar(uchar)
   */
  uchar has_scrollbar() const { return has_scrollbar_; }
  /**
    Sets whether the widget should have scrollbars or not (default Fl_Browser_::BOTH).
    By default you can scroll in both directions, and the scrollbars
    disappear if the data will fit in the widget.
    has_scrollbar() changes this based on the value of \p mode:

    - 0 - No scrollbars.

    - Fl_Browser_::HORIZONTAL - Only a horizontal scrollbar.

    - Fl_Browser_::VERTICAL - Only a vertical scrollbar.

    - Fl_Browser_::BOTH - The default is both scrollbars.

    - Fl_Browser_::HORIZONTAL_ALWAYS - Horizontal scrollbar always on,
      vertical always off.

    - Fl_Browser_::VERTICAL_ALWAYS - Vertical scrollbar always on,
      horizontal always off.

    - Fl_Browser_::BOTH_ALWAYS - Both always on.
  */
  void has_scrollbar(uchar mode) { has_scrollbar_ = mode; }

  /**
    Gets the default text font for the lines in the browser.
    \see textfont(), textsize(), textcolor()
  */
  Fl_Font textfont() const { return textfont_; }
  /**
    Sets the default text font for the lines in the browser to \p font.
  */
  void textfont(Fl_Font font) { textfont_ = font; }

  /**
    Gets the default text size (in pixels) for the lines in the browser.
  */
  Fl_Fontsize textsize() const { return textsize_; }
  /**
    Sets the default text size (in pixels) for the lines in the browser to \p size.
  */
  void textsize(Fl_Fontsize newSize) { textsize_ = newSize; }

  /**
    Gets the default text color for the lines in the browser.
  */
  Fl_Color textcolor() const { return textcolor_; }
  /**
    Sets the default text color for the lines in the browser to color \p col.
  */
  void textcolor(Fl_Color col) { textcolor_ = col; }

  /**
    Gets the current size of the scrollbars' troughs, in pixels.

    If this value is zero (default), this widget will use the
    Fl::scrollbar_size() value as the scrollbar's width.

    \returns Scrollbar size in pixels, or 0 if the global Fl::scrollbar_size() is being used.
    \see Fl::scrollbar_size(int)
  */
  int scrollbar_size() const {
      return(scrollbar_size_);
  }
  /**
    Sets the pixel size of the scrollbars' troughs to \p newSize, in pixels.

    Normally you should not need this method, and should use
    Fl::scrollbar_size(int) instead to manage the size of ALL
    your widgets' scrollbars. This ensures your application
    has a consistent UI, is the default behavior, and is normally
    what you want.

    Only use THIS method if you really need to override the global
    scrollbar size. The need for this should be rare.

    Setting \p newSize to the special value of 0 causes the widget to
    track the global Fl::scrollbar_size(), which is the default.

    \param[in] newSize Sets the scrollbar size in pixels.\n
                    If 0 (default), scrollbar size tracks the global Fl::scrollbar_size()
    \see Fl::scrollbar_size()
  */
  void scrollbar_size(int newSize) {
      scrollbar_size_ = newSize;
  }
  /**
    Returns the global value Fl::scrollbar_size().
   \deprecated Use scrollbar_size() instead.
    \todo This method should eventually be removed in 1.4+
  */
  int scrollbar_width() const {
      return(Fl::scrollbar_size());
  }
  /**
    Sets the global Fl::scrollbar_size(), and forces this instance of the widget to use it.
   \deprecated Use scrollbar_size() instead.
    \todo This method should eventually be removed in 1.4+
  */
  void scrollbar_width(int width) {
      Fl::scrollbar_size(width);
      scrollbar_size_ = 0;
  }
  /**
    Moves the vertical scrollbar to the righthand side of the list.
    For back compatibility.
  */
  void scrollbar_right() { scrollbar.align(FL_ALIGN_RIGHT); }
  /**
    Moves the vertical scrollbar to the lefthand side of the list.
    For back compatibility.
  */
  void scrollbar_left() { scrollbar.align(FL_ALIGN_LEFT); }
  void sort(int flags=0);

  /**
   Add some space between browser lines.
   \param[in] pixels number of additional pixels between lines.
   */
  void linespacing(int pixels) { linespacing_ = pixels; }

  /** Return the height of additional spacing between browser lines.
   \return spacing height in pixel units.
   */
  int linespacing() const { return linespacing_; }
};

#endif
