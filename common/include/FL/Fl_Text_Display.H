//
// Header file for Fl_Text_Display class.
//
// Copyright 2001-2023 by Bill Spitzak and others.
// Original code Copyright Mark Edel.  Permission to distribute under
// the LGPL for the FLTK library granted by Mark Edel.
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
 Fl_Text_Display widget . */

#ifndef FL_TEXT_DISPLAY_H
#define FL_TEXT_DISPLAY_H

#include <FL/Fl.H>              // Fl::scrollbar_size()
#include "fl_draw.H"
#include "Fl_Group.H"
#include "Fl_Widget.H"
#include "Fl_Scrollbar.H"
#include "Fl_Text_Buffer.H"

/**
 \brief Rich text display widget.

 This is the FLTK text display widget. It allows the user to view
 multiple lines of text and supports highlighting, word wrap, mixes
 of font faces and colors, line numbers and scrolling.  The buffer
 that is displayed in the widget is managed by the Fl_Text_Buffer
 class. A single Text Buffer can be displayed by multiple Text
 Displays.

 \image html editor.png "Fl_Text_Display widget"
 \image latex editor.png "Fl_Text_Display widget" width=10cm
 <BR><BR>
 \image html editor-linenumbers.png "Fl_Text_Display widget with line numbers enabled"
 \image latex editor-linenumbers.png "Fl_Text_Display widget with line numbers enabled" width=10cm

 \b Example \b Use
 \code
     #include <FL/Fl_Text_Display.H>
     ..
     int main() {
         ..
         Fl_Text_Buffer *buff = new Fl_Text_Buffer();
         Fl_Text_Display *disp = new Fl_Text_Display(10, 10, 640, 480);
         disp->buffer(buff);                 // attach text buffer to display widget
         buff->text("line one\nline two");   // add some text to buffer
         ..
     }
 \endcode

 \b Features

 - Word wrap: wrap_mode(), wrapped_column(), wrapped_row()
 - Font control: textfont(), textsize(), textcolor()
 - Font styling: highlight_data()
 - Cursor: cursor_style(), show_cursor(), hide_cursor(), cursor_color()
 - Line numbers: linenumber_width(), linenumber_font(),
   linenumber_size(), linenumber_fgcolor(), linenumber_bgcolor(),
   linenumber_align(), linenumber_format()

 Note that other features may be available via Fl_Text_Editor
 and Fl_Text_Buffer classes.

 \note Line numbers were added in FLTK 1.3.3.
 \see Fl_Widget::shortcut_label(int)
 */
class FL_EXPORT Fl_Text_Display: public Fl_Group {

public:

  /**
   text display cursor shapes enumeration
   */
  enum {
    NORMAL_CURSOR,    /**< I-beam */
    CARET_CURSOR,     /**< caret under the text */
    DIM_CURSOR,       /**< dim I-beam */
    BLOCK_CURSOR,     /**< unfille box under the current character */
    HEAVY_CURSOR,     /**< thick I-beam */
    SIMPLE_CURSOR    /**< as cursor as Fl_Input cursor */
  };

  /**
   the character position is the left edge of a character, whereas
   the cursor is thought to be between the centers of two consecutive
   characters.
   */
  enum {
    CURSOR_POS,
    CHARACTER_POS
  };

  /**
   drag types - they match Fl::event_clicks() so that single clicking to
   start a collection selects by character, double clicking selects by
   word and triple clicking selects by line.
   */
  enum {
    DRAG_NONE = -2,
    DRAG_START_DND = -1,
    DRAG_CHAR = 0,
    DRAG_WORD = 1,
    DRAG_LINE = 2
  };

  /**
   wrap types - used in wrap_mode()
   */
  enum {
    WRAP_NONE,      /**< don't wrap text at all */
    WRAP_AT_COLUMN, /**< wrap text at the given text column */
    WRAP_AT_PIXEL,  /**< wrap text at a pixel position */
    WRAP_AT_BOUNDS  /**< wrap text so that it fits into the widget width */
  };

  friend int fl_text_drag_prepare(int pos, int key, Fl_Text_Display* d);
  friend void fl_text_drag_me(int pos, Fl_Text_Display* d);

  typedef void (*Unfinished_Style_Cb)(int, void *);

  /**
   This structure associates the color, font, and font size of a string to draw
   with an attribute mask matching attr.

   There must be one entry for each style that can be used in an
   Fl_Text_Display for displaying text. The style table is an array of
   struct Style_Table_Entry.

   The style table is associated with an Fl_Text_Display by using
   Fl_Text_Display::highlight_data().

   \see Fl_Text_Display::highlight_data()
   */
  struct Style_Table_Entry {
    Fl_Color    color;    ///< text color
    Fl_Font     font;     ///< text font
    Fl_Fontsize size;     ///< text font size
    unsigned    attr;     ///< further attributes for the text style (see `ATTR_BGCOLOR`, etc.)
    Fl_Color    bgcolor;  ///< text background color if `ATTR_BGCOLOR` or `ATTR_BGCOLOR_EXT` is set
  };

  /**
   attribute flags in `Style_Table_Entry.attr`
   */
  enum {
    ATTR_BGCOLOR        = 0x0001, ///< use the background color in the `bgcolor` field
    ATTR_BGCOLOR_EXT_   = 0x0002, ///< (internal use)
    ATTR_BGCOLOR_EXT    = 0x0003, ///< extend background color to the end of the line
    ATTR_UNDERLINE      = 0x0004, ///< a single underline, underline types are mutually exclusive
    ATTR_GRAMMAR        = 0x0008, ///< grammar suggestion (blue dotted underline)
    ATTR_SPELLING       = 0x000C, ///< spelling suggestion (red dotted underline)
    ATTR_STRIKE_THROUGH = 0x0010, ///< line through the middle of the text
    ATTR_LINES_MASK     = 0x001C  ///< the mask for all underline and strike through types
  };

  Fl_Text_Display(int X, int Y, int W, int H, const char *l = 0);
  ~Fl_Text_Display();

  int handle(int e) FL_OVERRIDE;

  void buffer(Fl_Text_Buffer* buf);

  /**
   Sets the current text buffer associated with the text widget.
   Multiple text widgets can be associated with the same text buffer.
   \param buf new text buffer
   \see Fl_Text_Display::buffer(Fl_Text_Buffer* buf)
   */
  void buffer(Fl_Text_Buffer& buf) { buffer(&buf); }

  /**
   Gets the current text buffer associated with the text widget.
   Multiple text widgets can be associated with the same text buffer.
   \return current text buffer
   \see Fl_Text_Display::buffer(Fl_Text_Buffer* buf)
   \see Fl_Text_Display::buffer(Fl_Text_Buffer& buf)
   */
  Fl_Text_Buffer* buffer() const { return mBuffer; }
  /**
   Gets the current style buffer associated with the text widget.
   Multiple text widgets can be associated with the same style buffer.
   \return current style buffer
   \see Fl_Text_Display::highlight_data()
   */
  Fl_Text_Buffer* style_buffer() const { return mStyleBuffer; }

  void redisplay_range(int start, int end);
  void scroll(int topLineNum, int horizOffset);
  void insert(const char* text);
  void overstrike(const char* text);
  void insert_position(int newPos);

  /**
    Gets the position of the text insertion cursor for text display.

    The insert position is the byte count (offset) from the beginning
    of the text buffer (starting with 0). Returns 0 (zero) if no buffer
    is associated to the text display. Returns buffer()->length() if the
    insert position is at the end of the buffer.

    \returns insert position index into text buffer
    \see insert_position(int)
  */
  int insert_position() const { return mCursorPos; }
  int position_to_xy(int pos, int* x, int* y) const;

  int in_selection(int x, int y) const;
  void show_insert_position();

  int move_right();
  int move_left();
  int move_up();
  int move_down();
  int count_lines(int start, int end, bool start_pos_is_line_start) const;
  int line_start(int pos) const;
  int line_end(int startPos, bool startPosIsLineStart) const;
  int skip_lines(int startPos, int nLines, bool startPosIsLineStart);
  int rewind_lines(int startPos, int nLines);
  void next_word(void);
  void previous_word(void);

  void show_cursor(int b = 1);

  /**
   Hides the text cursor.
   */
  void hide_cursor() { show_cursor(0); }

  void cursor_style(int style);
  int cursor_style() const { return mCursorStyle; }

  /**
   Gets the text cursor color.
   \return cursor color
   */
  Fl_Color cursor_color() const {return mCursor_color;}

  /**
   Sets the text cursor color.
   \param n new cursor color
   */
  void cursor_color(Fl_Color n) {mCursor_color = n;}

  /**
    Returns the global value Fl::scrollbar_size() unless a specific scrollbar_width_ has been set.
   \deprecated Use scrollbar_size() instead.
    \todo This method should eventually be removed.
  */
  int scrollbar_width() const {
      return scrollbar_width_ ? scrollbar_width_ : Fl::scrollbar_size();
  }

  /**
    Sets the global Fl::scrollbar_size(), and forces this instance of the widget to use it.
   \deprecated Use scrollbar_size() instead.
    \todo This method should eventually be removed
  */
  void scrollbar_width(int width) {
      Fl::scrollbar_size(width);
      scrollbar_width_ = 0;
  }

  /**
    Gets the current size of the scrollbars' troughs, in pixels.

    If this value is zero (default), this widget will use the
    Fl::scrollbar_size() value as the scrollbar's width.

    \returns Scrollbar size in pixels, or 0 if the global Fl::scrollbar_size() is being used.
    \see Fl::scrollbar_size(int)
  */
  int scrollbar_size() const {
      return(scrollbar_width_);
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
      scrollbar_width_ = newSize;
  }

  /**
   Gets the scrollbar alignment type.
   \return scrollbar alignment
   */
  Fl_Align scrollbar_align() const { return scrollbar_align_; }

  /**
   Sets the scrollbar alignment type.
   \param a new scrollbar alignment
   */
  void scrollbar_align(Fl_Align a) { scrollbar_align_ = a; }

  /**
   Moves the insert position to the beginning of the current word.
   \param pos start calculation at this index
   \return beginning of the words
   */
  int word_start(int pos) const { return buffer()->word_start(pos); }

  /**
   Moves the insert position to the end of the current word.
   \param pos start calculation at this index
   \return index of first character after the end of the word
   */
  int word_end(int pos) const { return buffer()->word_end(pos); }


  void highlight_data(Fl_Text_Buffer *styleBuffer,
                      const Style_Table_Entry *styleTable,
                      int nStyles, char unfinishedStyle,
                      Unfinished_Style_Cb unfinishedHighlightCB,
                      void *cbArg);

  int position_style(int lineStartPos, int lineLen, int lineIndex) const;

  /**
   \todo FIXME : get set methods pointing on shortcut_
   have no effects as shortcut_ is unused in this class and derived!
   \return the current shortcut key
   */
  int shortcut() const {return shortcut_;}

  /**
   \todo FIXME : get set methods pointing on shortcut_
   have no effects as shortcut_ is unused in this class and derived!
   \param s the new shortcut key
   */
  void shortcut(int s) {shortcut_ = s;}

  /**
   Gets the default font used when drawing text in the widget.
   \return current text font face unless overridden by a style
   */
  Fl_Font textfont() const {return textfont_;}

  /**
   Sets the default font used when drawing text in the widget.
   \param s default text font face
   */
  void textfont(Fl_Font s) {textfont_ = s; mColumnScale = 0; }

  /**
   Gets the default size of text in the widget.
   \return current text height unless overridden by a style
   */
  Fl_Fontsize textsize() const {return textsize_;}

  /**
   Sets the default size of text in the widget.
   \param s new text size
   */
  void textsize(Fl_Fontsize s) {textsize_ = s; mColumnScale = 0; }

  /**
   Gets the default color of text in the widget.
   \return text color unless overridden by a style
   */
  Fl_Color textcolor() const {return textcolor_;}

  /**
   Sets the default color of text in the widget.
   \param n new text color
   */
  void textcolor(Fl_Color n) {textcolor_ = n;}

  /**
   Sets the underline color for style attribute ATTR_GRAMMAR.
   \param color underline color
   */
  void grammar_underline_color(Fl_Color color) { grammar_underline_color_ = color; }

  /**
   Gets the underline color for style attribute ATTR_GRAMMAR.
   \return underline color
   */
  Fl_Color grammar_underline_color() const { return grammar_underline_color_;}

  /**
   Sets the underline color for style attribute ATTR_SPELLING.
   \param color underline color
   */
  void spelling_underline_color(Fl_Color color) { spelling_underline_color_ = color; }

  /**
   Gets the underline color for style attribute ATTR_SPELLING.
   \return underline color
   */
  Fl_Color spelling_underline_color() const { return spelling_underline_color_;}

  /**
   Sets the background color for the secondary selection block.
   \param color background color
   */
  void secondary_selection_color(Fl_Color color) { secondary_selection_color_ = color; }

  /**
   Gets the background color for the secondary selection block.
   \return background color color
   */
  Fl_Color secondary_selection_color() const { return secondary_selection_color_;}

  int wrapped_column(int row, int column) const;
  int wrapped_row(int row) const;
  void wrap_mode(int wrap, int wrap_margin);

  virtual void recalc_display();
  virtual void display_needs_recalc();
  void resize(int X, int Y, int W, int H) FL_OVERRIDE;

  /**
   Convert an x pixel position into a column number.
   \param x number of pixels from the left margin
   \return an approximate column number based on the main font
   */
  double x_to_col(double x) const;

  /**
   Convert a column number into an x pixel position.
   \param col an approximate column number based on the main font
   \return number of pixels from the left margin to the left of an
           average sized character
   */
  double col_to_x(double col) const;

  void linenumber_width(int width);
  int linenumber_width() const;
  void linenumber_font(Fl_Font val);
  Fl_Font linenumber_font() const;
  void linenumber_size(Fl_Fontsize val);
  Fl_Fontsize linenumber_size() const;
  void linenumber_fgcolor(Fl_Color val);
  Fl_Color linenumber_fgcolor() const;
  void linenumber_bgcolor(Fl_Color val);
  Fl_Color linenumber_bgcolor() const;
  void linenumber_align(Fl_Align val);
  Fl_Align linenumber_align() const;
  void linenumber_format(const char* val);
  const char* linenumber_format() const;

protected:
  // Most (all?) of this stuff should only be called from resize() or
  // draw().
  // Anything with "vline" indicates thats it deals with currently
  // visible lines.

  void draw() FL_OVERRIDE;
  void draw_text(int X, int Y, int W, int H);
  void draw_range(int start, int end);
  void draw_cursor(int, int);

  void draw_string(int style, int x, int y, int toX, const char *string,
                   int nChars) const;

  void draw_vline(int visLineNum, int leftClip, int rightClip,
                  int leftCharIndex, int rightCharIndex);

  int find_x(const char *s, int len, int style, int x) const;

  enum {
    DRAW_LINE,
    FIND_INDEX,
    FIND_INDEX_FROM_ZERO,
    GET_WIDTH,
    FIND_CURSOR_INDEX   // STR #2788
  };

  int handle_vline(int mode,
                   int lineStart, int lineLen, int leftChar, int rightChar,
                   int topClip, int bottomClip,
                   int leftClip, int rightClip) const;

  int handle_rmb(int readonly);

  void draw_line_numbers(bool clearAll);

  void clear_rect(int style, int x, int y, int width, int height) const;
  void display_insert();

  void offset_line_starts(int newTopLineNum);

  void calc_line_starts(int startLine, int endLine);

  void update_line_starts(int pos, int charsInserted, int charsDeleted,
                          int linesInserted, int linesDeleted, int *scrolled);

  void calc_last_char();

  int position_to_line( int pos, int* lineNum ) const;
  double string_width(const char* string, int length, int style) const;

  static void scroll_timer_cb(void*);

  static void buffer_predelete_cb(int pos, int nDeleted, void* cbArg);
  static void buffer_modified_cb(int pos, int nInserted, int nDeleted,
                                 int nRestyled, const char* deletedText,
                                 void* cbArg);

  static void h_scrollbar_cb(Fl_Scrollbar* w, Fl_Text_Display* d);
  static void v_scrollbar_cb( Fl_Scrollbar* w, Fl_Text_Display* d);
  void update_v_scrollbar();
  void update_h_scrollbar();
  int measure_vline(int visLineNum) const;
  int longest_vline() const;
  int empty_vlines() const;
  int vline_length(int visLineNum) const;
  int xy_to_position(int x, int y, int PosType = CHARACTER_POS) const;

  void xy_to_rowcol(int x, int y, int* row, int* column,
                    int PosType = CHARACTER_POS) const;
  void maintain_absolute_top_line_number(int state);
public:
  int get_absolute_top_line_number() const;
  int scroll_row() { return mTopLineNum; }
  int scroll_col() { return mHorizOffset; }
protected:
  void absolute_top_line_number(int oldFirstChar);
  int maintaining_absolute_top_line_number() const;
  void reset_absolute_top_line_number();
  int position_to_linecol(int pos, int* lineNum, int* column) const;
  int scroll_(int topLineNum, int horizOffset);

  void extend_range_for_styles(int* start, int* end);

  void find_wrap_range(const char *deletedText, int pos, int nInserted,
                       int nDeleted, int *modRangeStart, int *modRangeEnd,
                       int *linesInserted, int *linesDeleted);
  void measure_deleted_lines(int pos, int nDeleted);
  void wrapped_line_counter(Fl_Text_Buffer *buf, int startPos, int maxPos,
                            int maxLines, bool startPosIsLineStart,
                            int styleBufOffset, int *retPos, int *retLines,
                            int *retLineStart, int *retLineEnd,
                            bool countLastLineMissingNewLine = true) const;
  void find_line_end(int pos, bool start_pos_is_line_start, int *lineEnd,
                     int *nextLineStart) const;
  double measure_proportional_character(const char *s, int colNum, int pos) const;
  int wrap_uses_character(int lineEndPos) const;

  int damage_range1_start, damage_range1_end;
  int damage_range2_start, damage_range2_end;
  int mCursorPos;
  int mCursorOn;
  int mCursorOldY;              /* Y pos. of cursor for blanking */
  int mCursorToHint;            /* Tells the buffer modified callback
                                 where to move the cursor, to reduce
                                 the number of redraw calls */
  int mCursorStyle;             /* One of enum cursorStyles above */
  int mCursorPreferredXPos;     /* Pixel position for vert. cursor movement */
  int mNVisibleLines;           /* # of visible (displayed) lines. This is
                                   also the size of the mLineStarts[] array. */
  int mNBufferLines;            /* # of newlines in the buffer, or number of
                                   wraps if line wrapping is enabled. Note that
                                   partial lines at the end of the buffer are
                                   not counted, so you may want to add 1. */
  Fl_Text_Buffer* mBuffer;      /* Contains text to be displayed */
  Fl_Text_Buffer* mStyleBuffer; /* Optional parallel buffer containing
                                 color and font information */
  int mFirstChar, mLastChar;    /* Buffer positions of first and last
                                 displayed character (lastChar points
                                 either to a newline or one character
                                 beyond the end of the buffer) */
  int mContinuousWrap;          /* Wrap long lines when displaying */
  int mWrapMarginPix;           /* Margin in # of pixels for
                                 wrapping in continuousWrap mode */
  int* mLineStarts;             /* Array of the size mNVisibleLines.
                                   This array only keeps track of lines
                                   within the display area. Each entry
                                   contains the starting character offset
                                   (from the beginning of the text buffer)
                                   for each /visible/ line.
                                   If wrap enabled, points to the beginning
                                   of each wrap. So a long line wrapping
                                   into 3 separate lines in the display
                                   will take up 3 separate array entries. */
  int mTopLineNum;              /* Line number of top displayed line
                                 of file (first line of file is 1) */
  int mAbsTopLineNum;           /* In continuous wrap mode, the line
                                  number of the top line if the text
                                  were not wrapped (note that this is
                                  only maintained as needed). */
  int mNeedAbsTopLineNum;       /* Externally settable flag to continue
                                 maintaining absTopLineNum even if
                                 it isn't needed for line # display */
  int mHorizOffset;             /* Horizontal scroll pos. in pixels */
  int mTopLineNumHint;          /* Line number of top displayed line
                                 of file (first line of file is 1) */
  int mHorizOffsetHint;         /* Horizontal scroll pos. in pixels */
  int mNStyles;                 /* Number of entries in styleTable */
  const Style_Table_Entry *mStyleTable; /* Table of fonts and colors for
                                         coloring/syntax-highlighting */
  char mUnfinishedStyle;        /* Style buffer entry which triggers
                                 on-the-fly reparsing of region */
  Unfinished_Style_Cb mUnfinishedHighlightCB; /* Callback to parse "unfinished" */
  /* regions */
  void* mHighlightCBArg;        /* Arg to unfinishedHighlightCB */

  int mMaxsize;

  int mSuppressResync;          /* Suppress resynchronization of line
                                 starts during buffer updates */
  int mNLinesDeleted;           /* Number of lines deleted during
                                 buffer modification (only used
                                 when resynchronization is suppressed) */
  int mModifyingTabDistance;    /* Whether tab distance is being modified XXX: UNUSED */

  mutable double mColumnScale; /* Width in pixels of an average character. This
                                 value is calculated as needed (lazy eval); it
                                 needs to be mutable so that it can be calculated
                                 within a method marked as "const" */

  bool display_needs_recalc_;  /* Set to true when the display needs
                                 to be recalculated. */

  Fl_Color mCursor_color;

  Fl_Scrollbar* mHScrollBar;
  Fl_Scrollbar* mVScrollBar;
  int scrollbar_width_;         // size of scrollbar trough (behavior changed in 1.4)
  Fl_Align scrollbar_align_;
  int dragPos, dragType, dragging;
  int display_insert_position_hint;
  struct { int x, y, w, h; } text_area;

  int shortcut_;

  Fl_Font textfont_;
  Fl_Fontsize textsize_;
  Fl_Color textcolor_;
  Fl_Color grammar_underline_color_;
  Fl_Color spelling_underline_color_;
  Fl_Color secondary_selection_color_;

  // Line number margin and width
  int mLineNumLeft, mLineNumWidth;

  // Line number font/colors
  Fl_Font     linenumber_font_;
  Fl_Fontsize linenumber_size_;
  Fl_Color    linenumber_fgcolor_;
  Fl_Color    linenumber_bgcolor_;
  Fl_Align    linenumber_align_;
  const char* linenumber_format_;
};

#endif
