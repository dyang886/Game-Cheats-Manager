//
// Tile header file for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2023 by Bill Spitzak and others.
//
// This library is free software. Distribution and use rights are outlined in
// the file "COPYING" which should have been included with this file. If this
// file is missing or damaged, see the license at:
//
//     https://www.fltk.org/COPYING.php
//
// Please see the following page on how to report bugs and issues:
//
//     https://www.fltk.org/bugs.php
//

#ifndef Fl_Tile_H
#define Fl_Tile_H

#include "Fl_Group.H"

/*
  The Fl_Tile class lets you resize its children by dragging
  the border between them.
*/

class FL_EXPORT Fl_Tile : public Fl_Group {
public:
  int handle(int event) FL_OVERRIDE;
  Fl_Tile(int X, int Y, int W, int H, const char *L=0);
  ~Fl_Tile() FL_OVERRIDE;
  void resize(int X, int Y, int W, int H) FL_OVERRIDE;
  virtual void move_intersection(int oldx, int oldy, int newx, int newy);
  virtual void drag_intersection(int oldx, int oldy, int newx, int newy);
  FL_DEPRECATED("since 1.4.0 - use move_intersection(p) instead",
  void position(int oldx, int oldy, int newx, int newy)) { move_intersection(oldx, oldy, newx, newy); }
  void position(int x, int y) { Fl_Group::position(x, y); }
  void size_range(int index, int minw, int minh, int maxw=0x7FFFFFFF, int maxh=0x7FFFFFFF);
  void size_range(Fl_Widget *w , int minw, int minh, int maxw=0x7FFFFFFF, int maxh=0x7FFFFFFF);
  void init_size_range(int default_min_w = -1, int default_min_h = -1);

protected:
  int cursor_;                  ///< current cursor index (0..3)
  Fl_Cursor *cursors_;          ///< points at the array of 4 cursors (may be overridden)

  /** Returns the cursor for cursor index n.
    \see Fl_Tile::set_cursor(int)
  */

  Fl_Cursor cursor(int n) {
    return cursors_[n];
  }

  void set_cursor(int n);       // set one of n (0..3) cursors

  typedef struct { int minw, minh, maxw, maxh; } Size_Range;

  Size_Range *size_range_;
  int size_range_size_, size_range_capacity_;
  int default_min_w_, default_min_h_;
  void request_shrink_l(int old_l, int &new_l, Fl_Rect *final_size);
  void request_shrink_r(int old_r, int &new_r, Fl_Rect *final_size);
  void request_shrink_t(int old_t, int &new_t, Fl_Rect *final_size);
  void request_shrink_b(int old_b, int &new_b, Fl_Rect *final_size);
  void request_grow_l(int old_l, int &new_l, Fl_Rect *final_size);
  void request_grow_r(int old_r, int &new_r, Fl_Rect *final_size);
  void request_grow_t(int old_t, int &new_t, Fl_Rect *final_size);
  void request_grow_b(int old_b, int &new_b, Fl_Rect *final_size);

  int on_insert(Fl_Widget*, int) FL_OVERRIDE;
  int on_move(int, int) FL_OVERRIDE;
  void on_remove(int) FL_OVERRIDE;
};

#endif
