//
// Fl_Table -- A table widget for the Fast Light Tool Kit (FLTK).
//
// Copyright 2002 by Greg Ercolano.
// Copyright (c) 2004 O'ksi'D
// Copyright 2023 by Bill Spitzak and others.
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

#ifndef _FL_TABLE_H
#define _FL_TABLE_H

#include <FL/Fl_Group.H>
#include <FL/Fl_Scroll.H>

// EXPERIMENTAL
// We use either std::vector or the private class Fl_Int_Vector
// depending on the build option FLTK_OPTION_STD or --enable-use_std.
// This option allows to use std::string and std::vector in FLTK 1.4.x

#if (FLTK_USE_STD)
#include <vector>
typedef std::vector<int> Fl_Int_Vector;
#else
class Fl_Int_Vector; // private class declared in src/Fl_Int_Vector.H
#endif

/**
  A table of widgets or other content.

  This is the base class for table widgets.

  To be useful it must be subclassed and several virtual functions defined.
  Normally applications use widgets derived from this widget, and do not use this
  widget directly; this widget is usually too low level to be used directly by
  applications.

  This widget does \em not handle the data in the table. The draw_cell()
  method must be overridden by a subclass to manage drawing the contents of
  the cells.

  This widget can be used in several ways:

  - As a custom widget; see examples/table-simple.cxx and test/table.cxx.
    Very optimal for even extremely large tables.
  - As a table made up of a single FLTK widget instanced all over the table,
    simulating a numeric spreadsheet. See examples/table-spreadsheet.cxx and
    examples/table-spreadsheet-with-keyboard-nav.cxx. Optimal for large tables.
  - As a regular container of FLTK widgets, one widget per cell.
    See examples/table-as-container.cxx. \em Not recommended for large tables.

  \image html table-simple.png
  \image latex table-simple.png "table-simple example" width=6cm

  \image html table-as-container.png
  \image latex table-as-container.png "table-as-container example" width=6cm

  When acting as part of a custom widget, events on the cells and/or headings
  generate callbacks when they are clicked by the user. You control when events
  are generated based on the setting for Fl_Table::when().

  When acting as a container for FLTK widgets, the FLTK widgets maintain
  themselves. Although the draw_cell() method must be overridden, its contents
  can be very simple. See the draw_cell() code in examples/table-simple.cxx.

  The following variables are available to classes deriving from Fl_Table:

  \anchor table_dimensions_diagram
  \image html table-dimensions.png
  \image latex table-dimensions.png "Fl_Table Dimensions" width=6cm

  <table border=0>
  <tr><td>x()/y()/w()/h()</td>
  <td>Fl_Table widget's outer dimension. The outer edge of the border of the
  Fl_Table. (Red in the diagram above)</td></tr>

  <tr><td>wix/wiy/wiw/wih</td>
  <td>Fl_Table widget's inner dimension. The inner edge of the border of the
  Fl_Table. eg. if the Fl_Table's box() is FL_NO_BOX, these values are the same
  as x()/y()/w()/h(). (Yellow in the diagram above)</td></tr>

  <tr><td>tox/toy/tow/toh</td>
  <td>The table's outer dimension. The outer edge of the border around the cells,
  but inside the row/col headings and scrollbars. (Green in the diagram above)
  </td></tr>

  <tr><td>tix/tiy/tiw/tih</td>
  <td>The table's inner dimension. The inner edge of the border around the cells,
  but inside the row/col headings and scrollbars. AKA the table's clip region.
  eg. if the table_box() is FL_NO_BOX, these values are the same as
  tox/toy/tow/toh. (Blue in the diagram above)
  </td></tr></table>

  CORE DEVELOPERS

  - Greg Ercolano : 12/16/2002 - initial implementation 12/16/02. Fl_Table, Fl_Table_Row, docs.
  - Jean-Marc Lienher : 02/22/2004 - added keyboard nav + mouse selection, and ported Fl_Table into fltk-utf8-1.1.4

  OTHER CONTRIBUTORS

  - Inspired by the Feb 2000 version of FLVW's Flvw_Table widget. Mucho thanks to those folks.
  - Mister Satan : 04/07/2003 - MinGW porting mods, and singleinput.cxx; a cool Fl_Input oriented spreadsheet example
  - Marek Paliwoda : 01/08/2003 - Porting mods for Borland
  - Ori Berger : 03/16/2006 - Optimizations for >500k rows/cols

  LICENSE

  Greg kindly gave his permission to integrate Fl_Table and Fl_Table_Row
  into FLTK, allowing FLTK license to apply while his widgets are part
  of the library. [updated by Greg, 04/26/17]
*/
class FL_EXPORT Fl_Table : public Fl_Group {
public:
  /**
    The context bit flags for Fl_Table related callbacks.

    Should be used in draw_cell() to determine what's being drawn,
    or in a callback() to determine where a recent event occurred.
  */
  enum TableContext {
    CONTEXT_NONE       = 0,     ///< no known context
    CONTEXT_STARTPAGE  = 0x01,  ///< before the table is redrawn
    CONTEXT_ENDPAGE    = 0x02,  ///< after the table is redrawn
    CONTEXT_ROW_HEADER = 0x04,  ///< drawing or event occurred in the row header
    CONTEXT_COL_HEADER = 0x08,  ///< drawing or event occurred in the col header
    CONTEXT_CELL       = 0x10,  ///< drawing or event occurred in a cell
    CONTEXT_TABLE      = 0x20,  ///< drawing or event occurred in a dead zone of table
    CONTEXT_RC_RESIZE  = 0x40   ///< column or row is being resized
  };

private:
  int _rows, _cols;     // total rows/cols
  int _row_header_w;    // width of row header
  int _col_header_h;    // height of column header
  int _row_position;    // last row_position set (not necessarily == toprow!)
  int _col_position;    // last col_position set (not necessarily == leftcol!)

  char _row_header;     // row header enabled?
  char _col_header;     // col header enabled?
  char _row_resize;     // row resizing enabled?
  char _col_resize;     // col resizing enabled?
  int _row_resize_min;  // row minimum resizing height (default=1)
  int _col_resize_min;  // col minimum resizing width (default=1)

  // OPTIMIZATION: partial row/column redraw variables
  int _redraw_toprow;
  int _redraw_botrow;
  int _redraw_leftcol;
  int _redraw_rightcol;
  Fl_Color _row_header_color;
  Fl_Color _col_header_color;

  int _auto_drag;
  int _selecting;
  int _scrollbar_size;
  enum {
    TABCELLNAV = 1<<0                   ///> tab cell navigation flag
  };
  unsigned int flags_;

  Fl_Int_Vector *_colwidths;            // column widths in pixels
  Fl_Int_Vector *_rowheights;           // row heights in pixels

  // number of columns and rows == size of corresponding vectors
  int col_size();                       // size of the column widths vector
  int row_size();                       // size of the row heights vector

  Fl_Cursor _last_cursor;               // last mouse cursor before changed to 'resize' cursor

  // EVENT CALLBACK DATA
  TableContext _callback_context;       // event context
  int _callback_row, _callback_col;     // event row/col

  // handle() state variables.
  //    Put here instead of local statics in handle(), so more
  //    than one Fl_Table can exist without crosstalk between them.
  //
  int _resizing_col;                    // column being dragged
  int _resizing_row;                    // row being dragged
  int _dragging_x;                      // starting x position for horiz drag
  int _dragging_y;                      // starting y position for vert drag
  int _last_row;                        // last row we FL_PUSH'ed

  // Redraw single cell
  void _redraw_cell(TableContext context, int R, int C);

  void _start_auto_drag();
  void _stop_auto_drag();
  void _auto_drag_cb();
  static void _auto_drag_cb2(void *d);

protected:
  enum ResizeFlag {
    RESIZE_NONE      = 0,
    RESIZE_COL_LEFT  = 1,
    RESIZE_COL_RIGHT = 2,
    RESIZE_ROW_ABOVE = 3,
    RESIZE_ROW_BELOW = 4
  };

  int table_w;                          ///< table's virtual width (in pixels)
  int table_h;                          ///< table's virtual height (in pixels)
  int toprow;                           ///< top row# of currently visible table on screen
  int botrow;                           ///< bottom row# of currently visible table on screen
  int leftcol;                          ///< left column# of currently visible table on screen
  int rightcol;                         ///< right column# of currently visible table on screen

  // selection
  int current_row;                      ///< selection cursor's current row (-1 if none)
  int current_col;                      ///< selection cursor's current column (-1 if none)
  int select_row;                       ///< extended selection row (-1 if none)
  int select_col;                       ///< extended selection column (-1 if none)

  // OPTIMIZATION: Precomputed scroll positions for the toprow/leftcol
  int toprow_scrollpos;                 ///< precomputed scroll position for top row
  int leftcol_scrollpos;                ///< precomputed scroll position for left column

  // Data table's inner dimension
  int tix;      ///< Data table's inner x dimension, inside bounding box. See \ref table_dimensions_diagram "Table Dimension Diagram"
  int tiy;      ///< Data table's inner y dimension, inside bounding box. See \ref table_dimensions_diagram "Table Dimension Diagram"
  int tiw;      ///< Data table's inner w dimension, inside bounding box. See \ref table_dimensions_diagram "Table Dimension Diagram"
  int tih;      ///< Data table's inner h dimension, inside bounding box. See \ref table_dimensions_diagram "Table Dimension Diagram"

  // Data table's outer dimension
  int tox;      ///< Data table's outer x dimension, outside bounding box. See \ref table_dimensions_diagram "Table Dimension Diagram"
  int toy;      ///< Data table's outer y dimension, outside bounding box. See \ref table_dimensions_diagram "Table Dimension Diagram"
  int tow;      ///< Data table's outer w dimension, outside bounding box. See \ref table_dimensions_diagram "Table Dimension Diagram"
  int toh;      ///< Data table's outer h dimension, outside bounding box. See \ref table_dimensions_diagram "Table Dimension Diagram"

  // Table widget's inner dimension
  int wix;      ///< Table widget's inner x dimension, inside bounding box. See \ref table_dimensions_diagram "Table Dimension Diagram"
  int wiy;      ///< Table widget's inner y dimension, inside bounding box. See \ref table_dimensions_diagram "Table Dimension Diagram"
  int wiw;      ///< Table widget's inner w dimension, inside bounding box. See \ref table_dimensions_diagram "Table Dimension Diagram"
  int wih;      ///< Table widget's inner h dimension, inside bounding box. See \ref table_dimensions_diagram "Table Dimension Diagram"

  Fl_Scroll *table;                             ///< child Fl_Scroll widget container for child fltk widgets (if any)
  Fl_Scrollbar *vscrollbar;                     ///< child vertical scrollbar widget
  Fl_Scrollbar *hscrollbar;                     ///< child horizontal scrollbar widget

  // Fltk
  int handle(int e) FL_OVERRIDE;        // fltk handle() FL_OVERRIDE

  // Class maintenance
  void recalc_dimensions();
  void table_resized();                         // table resized; recalc
  void table_scrolled();                        // table scrolled; recalc
  void get_bounds(TableContext context,         // return x/y/w/h bounds for context
                  int &X, int &Y, int &W, int &H);
  void change_cursor(Fl_Cursor newcursor);      // change mouse cursor to some other shape
  TableContext cursor2rowcol(int &R, int &C, ResizeFlag &resizeflag);
  int find_cell(TableContext context,           // find cell's x/y/w/h given r/c
                int R, int C, int &X, int &Y, int &W, int &H);
  int row_col_clamp(TableContext context, int &R, int &C);
  // clamp r/c to known universe

  /**
    Subclass should override this method to handle drawing the cells.

    This method will be called whenever the table is redrawn, once per cell.

    Only cells that are completely (or partially) visible will be told to draw.

    \p context will be one of the following:

    <table border=1>
    <tr>
    <td>\p Fl_Table::CONTEXT_STARTPAGE</td>
    <td>When table, or parts of the table, are about to be redrawn.<br>
        Use to initialize static data, such as font selections.<p>
        R/C will be zero,<br>
        X/Y/W/H will be the dimensions of the table's entire data area.<br>
        (Useful for locking a database before accessing; see
        also visible_cells())</td>
    </tr><tr>
    <td>\p Fl_Table::CONTEXT_ENDPAGE</td>
    <td>When table has completed being redrawn.<br>
        R/C will be zero, X/Y/W/H dimensions of table's data area.<br>
        (Useful for unlocking a database after accessing)</td>
    </tr><tr>
    <td>\p Fl_Table::CONTEXT_ROW_HEADER</td>
    <td>Whenever a row header cell needs to be drawn.<br>
        R will be the row number of the header being redrawn,<br>
        C will be zero,<br>
        X/Y/W/H will be the fltk drawing area of the row header in the window </td>
    </tr><tr>
    <td>\p Fl_Table::CONTEXT_COL_HEADER</td>
    <td>Whenever a column header cell needs to be drawn.<br>
        R will be zero, <br>
        C will be the column number of the header being redrawn,<br>
        X/Y/W/H will be the fltk drawing area of the column header in the window </td>
    </tr><tr>
    <td>\p Fl_Table::CONTEXT_CELL</td>
    <td>Whenever a data cell in the table needs to be drawn.<br>
        R/C will be the row/column of the cell to be drawn,<br>
        X/Y/W/H will be the fltk drawing area of the cell in the window </td>
    </tr><tr>
    <td>\p Fl_Table::CONTEXT_RC_RESIZE</td>
    <td>Whenever table or row/column is resized or scrolled,
        either interactively or via col_width() or row_height().<br>
        R/C/X/Y/W/H will all be zero.
        <p>
        Useful for fltk containers that need to resize or move
        the child fltk widgets.</td>
    </tr>
    </table>

    \p R and \p C will be set to the row and column number
    of the cell being drawn. In the case of row headers, \p C will be \a 0.
    In the case of column headers, \p R will be \a 0.

    <tt>X/Y/W/H</tt> will be the position and dimensions of where the cell
    should be drawn.

    In the case of custom widgets, a minimal draw_cell() override might
    look like the following. With custom widgets it is up to the caller to handle
    drawing everything within the dimensions of the cell, including handling the
    selection color.  Note all clipping must be handled as well; this allows drawing
    outside the dimensions of the cell if so desired for 'custom effects'.

    \code
    // This is called whenever Fl_Table wants you to draw a cell
    void MyTable::draw_cell(TableContext context, int R=0, int C=0, int X=0, int Y=0, int W=0, int H=0) {
        static char s[40];
        sprintf(s, "%d/%d", R, C);             // text for each cell
        switch ( context ) {
           case CONTEXT_STARTPAGE:             // Fl_Table telling us it's starting to draw page
               fl_font(FL_HELVETICA, 16);
               return;

           case CONTEXT_ROW_HEADER:            // Fl_Table telling us to draw row/col headers
           case CONTEXT_COL_HEADER:
               fl_push_clip(X, Y, W, H);
               {
                   fl_draw_box(FL_THIN_UP_BOX, X, Y, W, H, color());
                   fl_color(FL_BLACK);
                   fl_draw(s, X, Y, W, H, FL_ALIGN_CENTER);
               }
               fl_pop_clip();
               return;

           case CONTEXT_CELL:                  // Fl_Table telling us to draw cells
               fl_push_clip(X, Y, W, H);
               {
                   // BG COLOR
                   fl_color( row_selected(R) ? selection_color() : FL_WHITE);
                   fl_rectf(X, Y, W, H);

                   // TEXT
                   fl_color(FL_BLACK);
                   fl_draw(s, X, Y, W, H, FL_ALIGN_CENTER);

                   // BORDER
                   fl_color(FL_LIGHT2);
                   fl_rect(X, Y, W, H);
               }
               fl_pop_clip();
               return;

           default:
               return;
       }
       //NOTREACHED
   }
   \endcode
   */
  virtual void draw_cell(TableContext context, int R=0, int C=0,
                         int X=0, int Y=0, int W=0, int H=0)
  { (void)context; (void)R; (void)C; (void)X; (void)Y; (void)W; (void)H;}                                           // overridden by deriving class

  long row_scroll_position(int row);            // find scroll position of row (in pixels)
  long col_scroll_position(int col);            // find scroll position of col (in pixels)

  /**
   Does the table contain any child fltk widgets?
  */
  int is_fltk_container() {                     // does table contain fltk widgets?
    return( Fl_Group::children() > 3 );         // (ie. more than box and 2 scrollbars?)
  }

  static void scroll_cb(Fl_Widget*,void*);      // h/v scrollbar callback

  void damage_zone(int r1, int c1, int r2, int c2, int r3 = 0, int c3 = 0);

  /**
   Define region of cells to be redrawn by specified range of rows/cols,
   and then sets damage(DAMAGE_CHILD).  Extends any previously defined range to redraw.
  */
  void redraw_range(int topRow, int botRow, int leftCol, int rightCol) {
    if ( _redraw_toprow == -1 ) {
      // Initialize redraw range
      _redraw_toprow = topRow;
      _redraw_botrow = botRow;
      _redraw_leftcol = leftCol;
      _redraw_rightcol = rightCol;
    } else {
      // Extend redraw range
      if ( topRow < _redraw_toprow ) _redraw_toprow = topRow;
      if ( botRow > _redraw_botrow ) _redraw_botrow = botRow;
      if ( leftCol < _redraw_leftcol ) _redraw_leftcol = leftCol;
      if ( rightCol > _redraw_rightcol ) _redraw_rightcol = rightCol;
    }
    // Indicate partial redraw needed of some cells
    damage(FL_DAMAGE_CHILD);
  }

  // draw() has to be protected per FLTK convention (was public in 1.3.x)
  void draw() FL_OVERRIDE;

public:
  Fl_Table(int X, int Y, int W, int H, const char *l=0);
  ~Fl_Table();

  /**
   Clears the table to zero rows (rows(0)), zero columns (cols(0)), and clears
   any widgets (table->clear()) that were added with begin()/end() or add()/insert()/etc.
   \see rows(int), cols(int)
  */
  virtual void clear() {
    rows(0);
    cols(0);
    table->clear();
  }

  // \todo: add topline(), middleline(), bottomline()

  /**
   Sets the kind of box drawn around the data table,
   the default being FL_NO_BOX. Changing this value will cause the table
   to redraw.
  */
  inline void table_box(Fl_Boxtype val) {
    table->box(val);
    table_resized();
  }

  /**
   Returns the current box type used for the data table.
  */
  inline Fl_Boxtype table_box( void ) {
    return(table->box());
  }

  virtual void rows(int val);                   // set number of rows

  /**
   Returns the number of rows in the table.
  */
  inline int rows() {
    return(_rows);
  }

  virtual void cols(int val);                   // set number of columns

  /**
   Get the number of columns in the table.
  */
  inline int cols() {
    return(_cols);
  }

  /**
    Returns the range of row and column numbers for all visible
    and partially visible cells in the table.

    These values can be used e.g. by your draw_cell() routine during
    CONTEXT_STARTPAGE to figure out what cells are about to be redrawn
    for the purposes of locking the data from a database before it's drawn.

    \code
           leftcol             rightcol
              :                   :
    toprow .. .-------------------.
              |                   |
              |  V I S I B L E    |
              |                   |
              |    T A B L E      |
              |                   |
    botrow .. '-------------------`
    \endcode

    e.g. in a table where the visible rows are 5-20, and the
    visible columns are 100-120, then those variables would be:

    - toprow = 5
    - botrow = 20
    - leftcol = 100
    - rightcol = 120
  */
  inline void visible_cells(int& r1, int& r2, int& c1, int& c2) {
    r1 = toprow;
    r2 = botrow;
    c1 = leftcol;
    c2 = rightcol;
  }

  /**
    Returns 1 if someone is interactively resizing a row or column.
    You can currently call this only from within your callback().
  */
  int is_interactive_resize() {
    return(_resizing_row != -1 || _resizing_col != -1);
  }

  /**
    Returns if row resizing by the user is allowed.
  */
  inline int row_resize() {
    return(_row_resize);
  }

  /**
    Allows/disallows row resizing by the user.
    1=allow interactive resizing, 0=disallow interactive resizing.
    Since interactive resizing is done via the row headers,
    row_header() must also be enabled to allow resizing.
  */
  void row_resize(int flag) {                   // enable row resizing
    _row_resize = flag;
  }

  /**
    Returns if column resizing by the user is allowed.
  */
  inline int col_resize() {
    return(_col_resize);
  }

  /**
    Allows/disallows column resizing by the user.
    1=allow interactive resizing, 0=disallow interactive resizing.
    Since interactive resizing is done via the column headers,
    \p col_header() must also be enabled to allow resizing.
  */
  void col_resize(int flag) {                   // enable col resizing
    _col_resize = flag;
  }

  /**
    Returns the current column minimum resize value.
  */
  inline int col_resize_min() {                 // column minimum resizing width
    return(_col_resize_min);
  }

  /**
    Sets the current column minimum resize value.
    This is used to prevent the user from interactively resizing
    any column to be smaller than 'pixels'. Must be a value >=1.
  */
  void col_resize_min(int val) {
    _col_resize_min = ( val < 1 ) ? 1 : val;
  }

  /**
    Returns the current row minimum resize value.
  */
  inline int row_resize_min() {                 // column minimum resizing width
    return(_row_resize_min);
  }

  /**
    Sets the current row minimum resize value.
    This is used to prevent the user from interactively resizing
    any row to be smaller than 'pixels'. Must be a value >=1.
  */
  void row_resize_min(int val) {
    _row_resize_min = ( val < 1 ) ? 1 : val;
  }

  /**
    Returns if row headers are enabled or not.
  */
  inline int row_header() {                     // set/get row header enable flag
    return(_row_header);
  }

  /**
    Enables/disables showing the row headers. 1=enabled, 0=disabled.
    If changed, the table is redrawn.
  */
  void row_header(int flag) {
    _row_header = flag;
    table_resized();
    redraw();
  }

  /**
    Returns if column headers are enabled or not.
  */
  inline int col_header() {                     // set/get col header enable flag
    return(_col_header);
  }

  /**
    Enable or disable column headers.
    If changed, the table is redrawn.
  */
  void col_header(int flag) {
    _col_header = flag;
    table_resized();
    redraw();
  }

  /**
    Sets the height in pixels for column headers and redraws the table.
  */
  inline void col_header_height(int height) {   // set/get col header height
    _col_header_h = height;
    table_resized();
    redraw();
  }

  /**
    Gets the column header height.
  */
  inline int col_header_height() {
    return(_col_header_h);
  }

  /**
    Sets the row header width to n and causes the screen to redraw.
  */
  inline void row_header_width(int width) {     // set/get row header width
    _row_header_w = width;
    table_resized();
    redraw();
  }

  /**
    Returns the current row header width (in pixels).
  */
  inline int row_header_width() {
    return(_row_header_w);
  }

  /**
    Sets the row header color and causes the screen to redraw.
  */
  inline void row_header_color(Fl_Color val) {  // set/get row header color
    _row_header_color = val;
    redraw();
  }

  /**
    Returns the current row header color.
  */
  inline Fl_Color row_header_color() {
    return(_row_header_color);
  }

  /**
    Sets the color for column headers and redraws the table.
  */
  inline void col_header_color(Fl_Color val) {  // set/get col header color
    _col_header_color = val;
    redraw();
  }

  /**
    Gets the color for column headers.
  */
  inline Fl_Color col_header_color() {
    return(_col_header_color);
  }

  void row_height(int row, int height);         // set row height in pixels

  // Returns the current height of the specified row as a value in pixels.
  int row_height(int row);

  void col_width(int col, int width);           // set a column's width in pixels

  // Returns the current width of the specified column in pixels.
  int col_width(int col);

  /**
    Convenience method to set the height of all rows to the
    same value, in pixels. The screen is redrawn.
  */
  void row_height_all(int height) {             // set all row/col heights
    for ( int r=0; r<rows(); r++ ) {
      row_height(r, height);
    }
  }

  /**
    Convenience method to set the width of all columns to the
    same value, in pixels. The screen is redrawn.
  */
  void col_width_all(int width) {
    for ( int c=0; c<cols(); c++ ) {
      col_width(c, width);
    }
  }

  void row_position(int row);                   // set/get table's current scroll position
  void col_position(int col);

  /**
    Returns the current row scroll position as a row number.
  */
  int row_position() {
    return(_row_position);
  }

  /**
    Returns the current column scroll position as a column number.
  */
  int col_position() {
    return(_col_position);
  }

  /**
    Sets which row should be at the top of the table,
    scrolling as necessary, and the table is redrawn. If the table
    cannot be scrolled that far, it is scrolled as far as possible.
  */
  inline void top_row(int row) {                // set/get top row (deprecated)
    row_position(row);
  }

  /**
    Returns the current top row shown in the table.
    This row may be partially obscured.
  */
  inline int top_row() {
    return(row_position());
  }
  int is_selected(int r, int c);                // selected cell
  void get_selection(int &row_top, int &col_left, int &row_bot, int &col_right);
  void set_selection(int row_top, int col_left, int row_bot, int col_right);
  int move_cursor(int R, int C, int shiftselect);
  int move_cursor(int R, int C);
  void resize(int X, int Y, int W, int H) FL_OVERRIDE; // fltk resize() FL_OVERRIDE

  // This crashes sortapp() during init.
  //  void box(Fl_Boxtype val) {
  //    Fl_Group::box(val);
  //    if ( table ) {
  //      resize(x(), y(), w(), h());
  //    }
  //  }
  //  Fl_Boxtype box(void) const {
  //    return(Fl_Group::box());
  //  }

  // Child group management

  /**
    Resets the internal array of widget sizes and positions.
    \sa Fl_Group::init_sizes()
  */
  void init_sizes() {
    table->init_sizes();
    table->redraw();
  }

  /**
    The specified widget is removed from its current group (if any)
    and added to the end of Fl_Table's group.
  */
  void add(Fl_Widget& wgt) {
    table->add(wgt);
    if ( table->children() > 2 ) {
      table->show();
    } else {
      table->hide();
    }
  }

  /**
    The specified widget is removed from its current group (if any)
    and added to the end of Fl_Table's group.
  */
  void add(Fl_Widget* wgt) {
    add(*wgt);
  }

  /**
    The specified widget is removed from its current group (if any)
    and inserted into the Fl_Table's group at position 'n'.
  */
  void insert(Fl_Widget& wgt, int n) {
    table->insert(wgt,n);
  }

  /**
    The specified widget is removed from its current group (if any)
    and inserted into Fl_Table's group before widget 'w2'.
    This will append if 'w2' is not in Fl_Table's group.
  */
  void insert(Fl_Widget& wgt, Fl_Widget* w2) {
    table->insert(wgt,w2);
  }

  /**
    The specified widget is removed from Fl_Table's group.
  */
  void remove(Fl_Widget& wgt) {
    table->remove(wgt);
  }

  // (doxygen will substitute Fl_Group's docs here)
  void begin() {
    table->begin();
  }

  // (doxygen will substitute Fl_Group's docs here)
  void end() {
    table->end();
    // HACK: Avoid showing Fl_Scroll; seems to erase screen
    //       causing unnecessary flicker, even if its box() is FL_NO_BOX.
    //
    if ( table->children() > 2 ) {
      table->show();
    } else {
      table->hide();
    }
    Fl_Group::current(Fl_Group::parent());
  }

  /**
    Returns a pointer to the array of children. <I>This pointer is only
    valid until the next time a child is added or removed.</I>
  */
  Fl_Widget*const* array() {
    return(table->array());
  }

  /**
    Returns the child widget by an index.

    When using the Fl_Table as a container for FLTK widgets, this method returns
    the widget pointer from the internal array of widgets in the container.

    Typically used in loops, eg:
    \code
    for ( int i=0; i<children(); i++ ) {
      Fl_Widget *w = child(i);
      [..]
    }
    \endcode
  */
  Fl_Widget *child(int n) const {
    return(table->child(n));
  }

  /**
    Returns the number of children in the table.

    When using the Fl_Table as a container for FLTK widgets, this method returns
    how many child widgets the table has.

    \see child(int)
  */
  int children() const {
    return(table->children()-2);    // -2: skip Fl_Scroll's h/v scrollbar widgets
  }

  // (doxygen will substitute Fl_Group's docs here)
  int find(const Fl_Widget *wgt) const {
    return(table->find(wgt));
  }

  // (doxygen will substitute Fl_Group's docs here)
  int find(const Fl_Widget &wgt) const {
    return(table->find(wgt));
  }

  // CALLBACKS

  /**
    Returns the current row the event occurred on.

    This function should only be used from within the user's callback function.
  */
  int callback_row() {
    return(_callback_row);
  }

  /**
    Returns the current column the event occurred on.

    This function should only be used from within the user's callback function.
  */
  int callback_col() {
    return(_callback_col);
  }

  /**
    Returns the current 'table context'.

    This function should only be used from within the user's callback function.
  */
  TableContext callback_context() {
    return(_callback_context);
  }

  /**
    Calls the widget callback.

    Saves the specified 'context', 'row', and 'col' values, so that the user's
    callback can then access them with the member functions
    callback_context(), callback_row() and callback_col().
  */
  void do_callback(TableContext context, int row, int col) {
    _callback_context = context;
    _callback_row = row;
    _callback_col = col;
    Fl_Widget::do_callback();
  }

#ifdef FL_DOXYGEN
  /**
    The Fl_Widget::when() function is used to set a group of flags, determining
    when the widget callback is called:

    <table border=1>
    <tr>
    <td>\p FL_WHEN_CHANGED</td>
    <td>
    callback() will be called when rows or columns are resized (interactively or
    via col_width() or row_height()), passing CONTEXT_RC_RESIZE via
    callback_context().
    </td>
    </tr><tr>
    <td>\p FL_WHEN_RELEASE</td>
    <td>
    callback() will be called during FL_RELEASE events, such as when someone
    releases a mouse button somewhere on the table.
    </td>
    </tr>
    </table>

    The callback() routine is sent a TableContext that indicates the context the
    event occurred in, such as in a cell, in a header, or elsewhere on the table.
    When an event occurs in a cell or header, callback_row() and
    callback_col() can be used to determine the row and column. The callback
    can also look at the regular fltk event values (ie. Fl::event() and
    Fl::event_button()) to determine what kind of event is occurring.
  */
  void when(Fl_When flags);
#endif

#ifdef FL_DOXYGEN
  /**
    Callbacks will be called depending on the setting of Fl_Widget::when().

    Callback functions should use the following functions to determine the
    context/row/column:

    - Fl_Table::callback_row() returns current row
    - Fl_Table::callback_col() returns current column
    - Fl_Table::callback_context() returns current table context

    callback_row() and callback_col() will be set to the row and column number the
    event occurred on. If someone clicked on a row header, \p col will be \a 0.
    If someone clicked on a column header, \p row will be \a 0.

    callback_context() will return one of the following:

    <table border=1>
    <tr><td><tt>Fl_Table::CONTEXT_ROW_HEADER</tt></td>
    <td>Someone clicked on a row header. Excludes resizing.</td>
    </tr><tr>
    <td><tt>Fl_Table::CONTEXT_COL_HEADER</tt></td>
    <td>Someone clicked on a column header. Excludes resizing.</td>
    </tr><tr>
    <td><tt>Fl_Table::CONTEXT_CELL</tt></td>
    <td>
    Someone clicked on a cell.

    To receive callbacks for FL_RELEASE events, you must set
    when(FL_WHEN_RELEASE).
    </td>
    </tr><tr>
    <td><tt>Fl_Table::CONTEXT_RC_RESIZE</tt></td>
    <td>
    Someone is resizing rows/columns either interactively,
    or via the col_width() or row_height() API.

    Use is_interactive_resize()
    to determine interactive resizing.

    If resizing a column, R=0 and C=column being resized.

    If resizing a row, C=0 and R=row being resized.

    NOTE: To receive resize events, you must set when(FL_WHEN_CHANGED).
    </td>
    </tr>
    </table>

    \code
    class MyTable : public Fl_Table {
      [..]
    private:
      // Handle events that happen on the table
      void event_callback2() {
        int R = callback_row(),                         // row where event occurred
        C = callback_col();                             // column where event occurred
        TableContext context = callback_context();      // which part of table
        fprintf(stderr, "callback: Row=%d Col=%d Context=%d Event=%d\n",
                R, C, (int)context, (int)Fl::event());
      }

      // Actual static callback
      static void event_callback(Fl_Widget*, void* data) {
        MyTable *o = (MyTable*)data;
        o->event_callback2();
      }

    public:
      // Constructor
      MyTable() {
        [..]
        table.callback(&event_callback, (void*)this);   // setup callback
        table.when(FL_WHEN_CHANGED|FL_WHEN_RELEASE);    // when to call it
      }
    };
    \endcode
  */
  void callback(Fl_Widget*, void*);
#endif

  /**
    Gets the current size of the scrollbars' troughs, in pixels.

    If this value is zero (default), this widget will use the
    Fl::scrollbar_size() value as the scrollbar's width.

    \returns Scrollbar size in pixels, or 0 if the global Fl::scrollbar_size() is being used.
    \see Fl::scrollbar_size(int)
  */
  int scrollbar_size() const {
      return(_scrollbar_size);
  }

  /**
    Sets the pixel size of the scrollbars' troughs to \p newSize, in pixels.

    Normally you should not need this method, and should use
    Fl::scrollbar_size(int) instead to manage the size of ALL your
    widgets' scrollbars. This ensures your application has a consistent
    UI, is the default behavior, and is normally what you want.

    Only use THIS method if you really need to override the global
    scrollbar size. The need for this should be rare.

    Setting \p newSize to the special value of 0 causes the widget to
    track the global Fl::scrollbar_size(), which is the default.

    \param[in] newSize Sets the scrollbar size in pixels.\n
                    If 0 (default), scrollbar size tracks the global Fl::scrollbar_size()
    \see Fl::scrollbar_size()
  */
  void scrollbar_size(int newSize) {
      if ( newSize != _scrollbar_size ) redraw();
      _scrollbar_size = newSize;
  }

  /**
    Flag to control if Tab navigates table cells or not.

    If on, Tab key navigates table cells.
    If off, Tab key navigates fltk widget focus. (default)

    As of fltk 1.3, the default behavior of the Tab key is to navigate focus off
    of the current widget, and on to the next one.  But in some applications,
    it's useful for Tab to be used to navigate cells in the Fl_Table.

    \param [in] val If \p val is 1, Tab key navigates cells in table, not fltk widgets.<BR>
                    If \p val is 0, Tab key will advance focus to the next fltk widget (default), and does not navigate cells in table.
  */
  void tab_cell_nav(int val) {
    if ( val ) flags_ |=  TABCELLNAV;
    else       flags_ &= ~TABCELLNAV;
  }

  /**
    Get state of table's 'Tab' key cell navigation flag.

    \returns 1 if Tab configured to navigate cells in table<br>0 to navigate widget focus (default)

    \see tab_cell_nav(int)
  */
  int tab_cell_nav() const {
    return(flags_ & TABCELLNAV ? 1 : 0);
  }
};

#endif /*_FL_TABLE_H*/
