//
// Fl_Grid widget header for the Fast Light Tool Kit (FLTK).
//
// Copyright 2021-2022 by Albrecht Schlosser.
// Copyright 2022-2023 by Bill Spitzak and others.
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

#ifndef _FL_FL_GRID_H_
#define _FL_FL_GRID_H_

/** \file FL/Fl_Grid.H
  Fl_Grid container widget.
*/

#include <FL/Fl_Group.H>
#include <FL/Fl_Rect.H>

/** Fl_Grid type for child widget alignment control. */
typedef unsigned short Fl_Grid_Align;

/** Align the widget in the middle of the cell (default). */
const Fl_Grid_Align  FL_GRID_CENTER          = 0x0000;

/** Align the widget at the top of the cell. */
const Fl_Grid_Align  FL_GRID_TOP             = 0x0001;

/** Align the widget at the bottom of the cell. */
const Fl_Grid_Align  FL_GRID_BOTTOM          = 0x0002;

/** Align the widget at the left side of the cell. */
const Fl_Grid_Align  FL_GRID_LEFT            = 0x0004;

/** Align the widget at the right side of the cell. */
const Fl_Grid_Align  FL_GRID_RIGHT           = 0x0008;

/** Stretch the widget horizontally to fill the cell. */
const Fl_Grid_Align  FL_GRID_HORIZONTAL      = 0x0010;

/** Stretch the widget vertically to fill the cell. */
const Fl_Grid_Align  FL_GRID_VERTICAL        = 0x0020;

/** Stretch the widget in both directions to fill the cell. */
const Fl_Grid_Align  FL_GRID_FILL            = 0x0030;

/** Stretch the widget proportionally. */
const Fl_Grid_Align  FL_GRID_PROPORTIONAL    = 0x0040;

const Fl_Grid_Align  FL_GRID_TOP_LEFT        =  FL_GRID_TOP |  FL_GRID_LEFT;
const Fl_Grid_Align  FL_GRID_TOP_RIGHT       =  FL_GRID_TOP |  FL_GRID_RIGHT;
const Fl_Grid_Align  FL_GRID_BOTTOM_LEFT     =  FL_GRID_BOTTOM |  FL_GRID_LEFT;
const Fl_Grid_Align  FL_GRID_BOTTOM_RIGHT    =  FL_GRID_BOTTOM |  FL_GRID_RIGHT;

/**
  Fl_Grid is a container (layout) widget with multiple columns and rows.

  This container widget features very flexible layouts in columns and rows
  w/o the need to position each child widget in x/y coordinates.

  Widgets are assigned to grid cells (column, row) with their minimal sizes
  in \p w() and \p h(). The \p x() and \p y() positions are ignored and can
  be (0, 0). Fl_Grid calculates widget positions and resizes the widgets to
  fit into the grid. It is possible to create a single row or column of
  widgets with Fl_Grid.

  You should design your grid with the smallest possible sizes of all widgets
  in mind. Fl_Grid will automatically assign additional space to cells
  according to some rules (described later) when resizing the Fl_Grid widget.

  \b Hint: You should set a minimum window size to make sure the Fl_Grid is
    never resized below its minimal sizes. Resizing below the given widget
    sizes results in undefined behavior.

  Fl_Grid and other container widgets (e.g. Fl_Group) can be nested. One main
  advantage of this usage is that widget coordinates in embedded Fl_Group
  widgets become relative to the group and will be positioned as expected.
  \todo This (relative group coordinates of nested groups of Fl_Grid)
    needs explanation and maybe an example.

  Fl_Grid child widgets are handled by its base class Fl_Group but Fl_Grid
  stores additional data corresponding to each widget in internal grid cells.

  Fl_Grid children are allowed to span multiple columns and rows like HTML
  \<table\> cells. Individual children can have fixed sizes or be aligned
  inside their cells (left, right, top, bottom, and more) and/or follow
  their cell sizes when the Fl_Grid container is resized.

  Note to resizing: since Fl_Grid uses its own layout algorithm the normal
  Fl_Group::resizable() widget is ignored (if set). Calling init_sizes()
  is not necessary.

  \note Fl_Grid is, as of FLTK 1.4.1, still in experimental state and
    should be used with caution. The API can still be changed although it is
    assumed to be almost stable - as stable as possible for a first release.

  Example: Simple 3x3 Fl_Grid with five buttons:
  \n
  \code
  #include <FL/Fl.H>
  #include <FL/Fl_Double_Window.H>
  #include <FL/Fl_Grid.H>
  #include <FL/Fl_Button.H>

  int main(int argc, char **argv) {
    Fl_Double_Window *win = new Fl_Double_Window(320, 180, "3x3 Fl_Grid with Buttons");
    // create the Fl_Grid container with five buttons
    Fl_Grid *grid = new Fl_Grid(0, 0, win->w(), win->h());
    grid->layout(3, 3, 10, 10);
    grid->color(FL_WHITE);
    Fl_Button *b0 = new Fl_Button(0, 0, 0, 0, "New");
    Fl_Button *b1 = new Fl_Button(0, 0, 0, 0, "Options");
    Fl_Button *b3 = new Fl_Button(0, 0, 0, 0, "About");
    Fl_Button *b4 = new Fl_Button(0, 0, 0, 0, "Help");
    Fl_Button *b6 = new Fl_Button(0, 0, 0, 0, "Quit");
    // assign buttons to grid positions
    grid->widget(b0, 0, 0);
    grid->widget(b1, 0, 2);
    grid->widget(b3, 1, 1);
    grid->widget(b4, 2, 0);
    grid->widget(b6, 2, 2);
    grid->show_grid(0);     // 1 to display grid helper lines
    grid->end();
    win->end();
    win->resizable(grid);
    win->size_range(300, 100);
    win->show(argc, argv);
    return Fl::run();
  }
  \endcode

  \image html  Fl_Grid.png
  \image latex Fl_Grid.png "Simple 3x3 Fl_Grid" width=7cm

  \image html  Fl_Grid_show_grid_1.png
  \image latex Fl_Grid_show_grid_1.png "show_grid() set to 1" width=7cm

*/
class FL_EXPORT Fl_Grid : public Fl_Group {
  friend class Fl_Grid_Type;

public:
  class Cell {
    friend class Fl_Grid;
  private:
    Cell *next_;                // next cell in the same row
    short row_;                 // row number
    short col_;                 // column number
    short rowspan_;             // row span (1 - n)
    short colspan_;             // column span (1 - n)
    Fl_Grid_Align align_;       // widget alignment in its cell
    Fl_Widget *widget_;         // assigned widget
    int w_;                     // minimal widget width
    int h_;                     // minimal widget height

  public:

    void Cell_() {              // common initialization
      next_ = NULL;
      row_ = 0;
      col_ = 0;
      rowspan_ = 1;
      colspan_ = 1;
      widget_ = NULL;
      w_ = 0;
      h_ = 0;
      align_ = 0;
    }

    Cell(int row, int col) {    // constructor
      Cell_();
      row_ = row;
      col_ = col;
    }

    Cell(Fl_Widget *w, int row, int col) {  // widget assignment
      Cell_();
      widget_ = w;
      row_ = row;
      col_ = col;
    }

    /**
      The destructor deletes the cell.

      \todo Fl_Grid's cell destructor should remove the cell from the grid.
            Currently it does nothing!
    */
    ~Cell() {}

    /**
      Returns the next widget cell of the same row of this cell.
    */
    Cell *next() {
      return next_;
    }

    /**
      Sets the \c next pointer of a grid's cell.

      \b Internal use only!

      Do not use this method, it may corrupt the allocated memory.

      \internal
        This method is public due to issue #937 but should be private or
        at least protected. For more info see GitHub issue #937.
    */
    void next(Cell *c) {
      next_ = c;
    }

    Fl_Widget *widget() const { return widget_; }

    short row() const { return row_; }
    short col() const { return col_; }

    void rowspan(short v) { rowspan_ = v; }
    void colspan(short v) { colspan_ = v; }
    short rowspan() const { return rowspan_; }
    short colspan() const { return colspan_; }

    void align(Fl_Grid_Align align) { align_ = align; }
    Fl_Grid_Align align() const { return align_; }

    void minimum_size(int w, int h) { if (w>=0) w_ = w; if (h>=0) h_ = h; }
    void minimum_size(int *w, int *h) const { if (w) *w = w_; if (h) *h = h_; }
  }; // class Cell

private:
  class Row;
  class Col;
  short rows_;
  short cols_;

  short margin_left_;         // left margin
  short margin_top_;          // top margin
  short margin_right_;        // right margin
  short margin_bottom_;       // bottom margin
  short gap_row_;             // gap between rows
  short gap_col_;             // gap between columns
  Fl_Rect old_size;           // only for resize callback (TBD)
  Col  *Cols_;                // array of columns
  Row  *Rows_;                // array of rows
  bool need_layout_;          // true if layout needs to be calculated

protected:
  Fl_Color grid_color;        // color for drawing the grid lines (design helper)
  bool draw_grid_;            // draw the grid for testing / design

protected:
  void init();
  Cell *add_cell(int row, int col);
  void remove_cell(int row, int col);

public:
  Fl_Grid(int X, int Y, int W, int H, const char *L = 0);
  virtual ~Fl_Grid();

  // define and manage the layout and resizing

  virtual void layout(int rows, int cols, int margin = -1, int gap = -1);
  virtual void layout();
  virtual void clear_layout();
  virtual void resize(int X, int Y, int W, int H) FL_OVERRIDE;

  short rows() const { return rows_; }
  short cols() const { return cols_; }

  /**
    Request or reset the request to calculate the layout of children.

    If called with \p true (1) this calls redraw() to schedule a
    full draw(). When draw is eventually called, the layout is
    (re)calculated before actually drawing the widget.

    \param[in]  set   1 to request layout calculation,\n
                      0 to reset the request
  */
  void need_layout(int set) {
    if (set) {
      need_layout_ = true;
      redraw();
    }
    else {
      need_layout_ = false;
    }
  }

  /**
    Return whether layout calculation is required.
  */
  bool need_layout() const {
    return need_layout_;
  }

protected:
  virtual void draw() FL_OVERRIDE;
  void on_remove(int) FL_OVERRIDE;
  virtual void draw_grid();           // draw grid lines for debugging

public:

  // get and set individual margins

  virtual void margin(int left, int top = -1, int right = -1, int bottom = -1);
  int margin(int *left, int *top, int *right, int *bottom) const;

  // get and set default row and column gaps for all rows and columns, respectively

  virtual void gap(int row_gap, int col_gap = -1);   // set default row and column gap(s)
  void gap(int *row_gap, int *col_gap) const;

  // find cells, get cell pointers

  Fl_Grid::Cell* cell(int row, int col) const;
  Fl_Grid::Cell* cell(Fl_Widget *widget) const;

  // assign a widget to a cell

  Fl_Grid::Cell* widget(Fl_Widget *wi, int row, int col, Fl_Grid_Align align = FL_GRID_FILL);
  Fl_Grid::Cell* widget(Fl_Widget *wi, int row, int col, int rowspan, int colspan, Fl_Grid_Align align = FL_GRID_FILL);

  // set minimal column and row sizes (widths and heights, respectively),
  // set row and column specific gaps and weights

  void col_width(int col, int value);
  void col_width(const int *value, size_t size);
  int col_width(int col) const;

  void col_weight(int col, int value);
  void col_weight(const int *value, size_t size);
  int col_weight(int col) const;

  void col_gap(int col, int value);
  void col_gap(const int *value, size_t size);
  int col_gap(int col) const;

  void row_height(int row, int value);
  void row_height(const int *value, size_t size);
  int row_height(int row) const;

  void row_weight(int row, int value);
  void row_weight(const int *value, size_t size);
  int row_weight(int row) const;

  void row_gap(int row, int value);
  void row_gap(const int *value, size_t size);
  int row_gap(int row) const;

  int computed_col_width(int col) const;
  int computed_row_height(int row) const;

  /**
    Enable or disable drawing of the grid helper lines for visualization.

    Use this method during the design stage of your Fl_Grid widget or
    for debugging if widgets are not positioned as intended.

    The default is a light green color but you can change it for better
    contrast if needed, see show_grid(int set, Fl_Color col).

    \note You can define the environment variable \c FLTK_GRID_DEBUG=1
      to set show_grid(1) for all Fl_Grid widgets at construction time.
      This enables you to debug the grid layout w/o changing code.

    \param[in]  set   1 (true) = draw, 0 = don't draw the grid

    \see show_grid(int set, Fl_Color col)
  */
  void show_grid(int set) {
    draw_grid_ = set ? true : false;
  }

  /**
    Enable or disable drawing of the grid helper lines for visualization.

    This method also sets the color used for the helper lines.

    The default is a light green color but you can change it to any color
    for better contrast if needed.

    \param[in]  set   1 (true) = draw, 0 = don't draw the grid
    \param[in]  col   color to use for the grid helper lines

    \see show_grid(int set)
  */
  void show_grid(int set, Fl_Color col) {
    draw_grid_ = set ? true : false;
    grid_color = col;
  }

  void debug(int level = 127);

}; // class Fl_Grid

#endif // _FL_FL_GRID_H_
