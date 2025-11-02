//
// Fl_Scroll header file for the Fast Light Tool Kit (FLTK).
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
   Fl_Scroll widget . */

#ifndef Fl_Scroll_H
#define Fl_Scroll_H

#include "Fl_Group.H"
#include "Fl_Scrollbar.H"

/**
  This container widget lets you maneuver around a set of widgets much
  larger than your window.  If the child widgets are larger than the size
  of this object then scrollbars will appear so that you can scroll over
  to them:
  \image html Fl_Scroll.png
  \image latex  Fl_Scroll.png "Fl_Scroll" width=4cm

  If all of the child widgets are packed together into a solid
  rectangle then you want to set box() to FL_NO_BOX or
  one of the _FRAME types. This will result in the best output.
  However, if the child widgets are a sparse arrangement you must
  set box() to a real _BOX type. This can result in some
  blinking during redrawing, but that can be solved by using a
  Fl_Double_Window.

  The Fl_Scroll widget calculates the bounding box of all its children
  by using their widget positions and sizes (x, y, w, h). Outside labels
  are not considered. If you need outside labels of any widgets or free
  space outside of this bounding box you can add a tiny invisible Fl_Box
  at the relevant corner(s) of the Fl_Scroll widget, for instance:
  \code
    Fl_Scroll scroll(100, 100, 200, 200); // Fl_Scroll at (100, 100)
    Fl_Box(100, 100, 1, 1);               // Fl_Box in top left corner
    Fl_Input(150, 120, 60, 30, "Input:"); // left most widget with label
    // ... more widgets ...
    scroll.end();
  \endcode

  By default you can scroll in both directions, and the scrollbars
  disappear if the data will fit in the area of the scroll.

  Use Fl_Scroll::type() to change this as follows :

  - 0                            - No scrollbars
  - Fl_Scroll::HORIZONTAL        - Only a horizontal scrollbar.
  - Fl_Scroll::VERTICAL          - Only a vertical scrollbar.
  - Fl_Scroll::BOTH              - The default is both scrollbars.
  - Fl_Scroll::HORIZONTAL_ALWAYS - Horizontal scrollbar always on, vertical always off.
  - Fl_Scroll::VERTICAL_ALWAYS   - Vertical scrollbar always on, horizontal always off.
  - Fl_Scroll::BOTH_ALWAYS       - Both always on.

  Use <B> scrollbar.align(int) ( see void Fl_Widget::align(Fl_Align) ) :</B>
  to change what side the scrollbars are drawn on.

  If the FL_ALIGN_LEFT bit is on, the vertical scrollbar is on the left.
  If the FL_ALIGN_TOP bit is on, the horizontal scrollbar is on
  the top. Note that only the alignment flags in scrollbar are
  considered. The flags in hscrollbar however are ignored.

  This widget can also be used to pan around a single child widget
  "canvas".  This child widget should be of your own class, with a
  draw() method that draws the contents.  The scrolling is done by
  changing the x() and y() of the widget, so this child
  must use the x() and y() to position its drawing.
  To speed up drawing it should test fl_not_clipped(int x,int y,int w,int h)
  to find out if a particular area of the widget must be drawn.

  Another very useful child is a single Fl_Pack, which is itself a group
  that packs its children together and changes size to surround them.
  Filling the Fl_Pack with Fl_Tabs groups (and then putting
  normal widgets inside those) gives you a very powerful scrolling list
  of individually-openable panels.

  Fluid lets you create these, but you can only lay out objects that
  fit inside the Fl_Scroll without scrolling.  Be sure to leave
  space for the scrollbars, as Fluid won't show these either.

  <I>You cannot use Fl_Window as a child of this since the
  clipping is not conveyed to it when drawn, and it will draw over the
  scrollbars and neighboring objects.</I>
*/
class FL_EXPORT Fl_Scroll : public Fl_Group {

  int xposition_, yposition_;
  int oldx, oldy;
  int scrollbar_size_;
  static void hscrollbar_cb(Fl_Widget*, void*);
  static void scrollbar_cb(Fl_Widget*, void*);
  static void draw_clip(void*,int,int,int,int);

protected:      //  (STR#1895)

  /// A local struct to manage a region defined by xywh
  typedef struct { int x,y,w,h; } Fl_Region_XYWH;

  /// A local struct to manage a region defined by left/right/top/bottom
  typedef struct {
    int l;              ///< (l)eft "x" position, aka x1
    int r;              ///< (r)ight "x" position, aka x2
    int t;              ///< (t)op "y" position, aka y1
    int b;              ///< (b)ottom "y" position, aka y2
  } Fl_Region_LRTB;

  /// A local struct to manage a scrollbar's xywh region and tab values
  typedef struct {
    int x,y,w,h;
    int pos;            ///< scrollbar tab's "position of first line displayed"
    int size;           ///< scrollbar tab's "size of window in lines"
    int first;          ///< scrollbar tab's "number of first line"
    int total;          ///< scrollbar tab's "total number of lines"
  } Fl_Scrollbar_Data;

  /**
    Structure to manage scrollbar and widget interior sizes.
    This is filled out by recalc_scrollbars() for use in calculations
    that need to know the visible scroll area size, etc.
    \version 1.3.3
  */
  typedef struct {
    int scrollsize;             ///< the effective scrollbar thickness (local or global)
    Fl_Region_XYWH innerbox;    ///< widget's inner box, excluding scrollbars
    Fl_Region_XYWH innerchild;  ///< widget's inner box, including scrollbars
    Fl_Region_LRTB child;       ///< child bounding box: left/right/top/bottom
    int hneeded;                ///< horizontal scrollbar visibility
    int vneeded;                ///< vertical scrollbar visibility
    Fl_Scrollbar_Data hscroll;  ///< horizontal scrollbar region + values
    Fl_Scrollbar_Data vscroll;  ///< vertical scrollbar region + values
  } ScrollInfo;
  void recalc_scrollbars(ScrollInfo &si) const;

protected:

  int on_insert(Fl_Widget*, int) FL_OVERRIDE;
  int on_move(int, int) FL_OVERRIDE;
  void fix_scrollbar_order();
  void bbox(int&,int&,int&,int&) const;
  void draw() FL_OVERRIDE;

public:

  Fl_Scrollbar scrollbar;
  Fl_Scrollbar hscrollbar;

  void resize(int X, int Y, int W, int H) FL_OVERRIDE;
  int handle(int) FL_OVERRIDE;

  Fl_Scroll(int X, int Y, int W, int H, const char *L = 0);
  virtual ~Fl_Scroll();

  enum { // values for type()
    HORIZONTAL = 1,
    VERTICAL = 2,
    BOTH = 3,
    ALWAYS_ON = 4,
    HORIZONTAL_ALWAYS = 5,
    VERTICAL_ALWAYS = 6,
    BOTH_ALWAYS = 7
  };

  /**    Gets the current horizontal scrolling position.  */
  int xposition() const {return xposition_;}
  /**    Gets the current vertical scrolling position.  */
  int yposition() const {return yposition_;}
  void scroll_to(int, int);
  void clear();

  /* delete child n (by index) */
  int delete_child(int n) FL_OVERRIDE;

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
      if ( newSize != scrollbar_size_ ) redraw();
      scrollbar_size_ = newSize;
  }
};

#endif
