//
// Input base class header file for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2015 by Bill Spitzak and others.
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
   Fl_Input_ widget . */

#ifndef Fl_Input__H
#define Fl_Input__H

#ifndef Fl_Widget_H
#include "Fl_Widget.H"
#endif

#define FL_NORMAL_INPUT         0
#define FL_FLOAT_INPUT          1
#define FL_INT_INPUT            2
#define FL_HIDDEN_INPUT         3
#define FL_MULTILINE_INPUT      4
#define FL_SECRET_INPUT         5
#define FL_INPUT_TYPE           7
#define FL_INPUT_READONLY       8
#define FL_NORMAL_OUTPUT        (FL_NORMAL_INPUT | FL_INPUT_READONLY)
#define FL_MULTILINE_OUTPUT     (FL_MULTILINE_INPUT | FL_INPUT_READONLY)
#define FL_INPUT_WRAP           16
#define FL_MULTILINE_INPUT_WRAP (FL_MULTILINE_INPUT | FL_INPUT_WRAP)
#define FL_MULTILINE_OUTPUT_WRAP (FL_MULTILINE_INPUT | FL_INPUT_READONLY | FL_INPUT_WRAP)

class Fl_Input_Undo_Action;
class Fl_Input_Undo_Action_List;

/**
  This class provides a low-overhead text input field.

  This is a virtual base class below Fl_Input. It has all
  the same interfaces, but lacks the handle() and
  draw() method. You may want to subclass it if you are
  one of those people who likes to change how the editing keys
  work. It may also be useful for adding scrollbars
  to the input field.

  This can act like any of the subclasses of Fl_Input, by
  setting type() to one of the following values:

  \code
  #define FL_NORMAL_INPUT          0
  #define FL_FLOAT_INPUT           1
  #define FL_INT_INPUT             2
  #define FL_MULTILINE_INPUT       4
  #define FL_SECRET_INPUT          5
  #define FL_INPUT_TYPE            7
  #define FL_INPUT_READONLY        8
  #define FL_NORMAL_OUTPUT         (FL_NORMAL_INPUT | FL_INPUT_READONLY)
  #define FL_MULTILINE_OUTPUT      (FL_MULTILINE_INPUT | FL_INPUT_READONLY)
  #define FL_INPUT_WRAP            16
  #define FL_MULTILINE_INPUT_WRAP  (FL_MULTILINE_INPUT | FL_INPUT_WRAP)
  #define FL_MULTILINE_OUTPUT_WRAP (FL_MULTILINE_INPUT | FL_INPUT_READONLY | FL_INPUT_WRAP)
  \endcode

  All variables that represent an index into a text buffer are byte-oriented,
  not character oriented, counting from 0 (at or before the first character)
  to size() (at the end of the buffer, after the last byte). Since UTF-8
  characters can be up to six bytes long, simply incrementing such an index
  will not reliably advance to the next character in the text buffer.

  Indices and pointers into the text buffer should always point at a 7 bit ASCII
  character or the beginning of a UTF-8 character sequence. Behavior for false
  UTF-8 sequences and pointers into the middle of a sequence are undefined.

  \see Fl_Text_Display, Fl_Text_Editor for more powerful text handling widgets
  \see Fl_Widget::shortcut_label(int)

  \internal
  When porting this widget from ASCII to UTF-8, previously legal pointers into
  the text of this widget can become illegal by pointing into the middle of
  a UTF-8 sequence. This is not a big problem for Fl_Input_ because all code
  in this module is quite tolerant. It could be problematic though when deriving
  from this class because no feedback for illegal pointers is given. Additionally,
  a careless "copy" call can put partial UTF-8 sequences into the clipboard.

  None of these issues should be disastrous. Nevertheless, we should
  discuss how FLTK should handle false UTF-8 sequences and pointers.
*/
class FL_EXPORT Fl_Input_ : public Fl_Widget {

  /** \internal Storage for the text field. */
  const char* value_;

  /** \internal Buffer memory for expanded text. \see expand() */
  char* buffer;

  /** \internal Size of text in bytes in the \p value_ field. */
  int size_;

  /** \internal Current size of internal value() buffer in bytes. */
  int bufsize;

  /** \internal Position of the cursor in the document. */
  int position_;

  /** \internal Position of the other end of the selected text.
      If \p position_ equals \p mark_, no text is selected */
  int mark_;

  /** \internal Behavior of Tab key in multiline input widget.
      If enabled (default) Tab causes focus nav, otherwise Tab is inserted
      as a character. */
  int tab_nav_;

  /** \internal Offset to text origin within widget bounds */
  int xscroll_, yscroll_;

  /** \internal Minimal update pointer. Display requires redraw from here to the end
      of the buffer. */
  int mu_p;

  /** \internal Maximum number of (UTF-8) characters a user can input. */
  int maximum_size_;

  /** \internal Shortcut key that will fetch focus for this widget. */
  int shortcut_;

  /** \internal This is set if no text but only the cursor needs updating. */
  uchar erase_cursor_only;

  /** \internal The font used for the entire text. */
  Fl_Font textfont_;

  /** \internal Height of the font used for the entire text. */
  Fl_Fontsize textsize_;

  /** \internal color of the entire text */
  Fl_Color textcolor_;

  /** \internal color of the text cursor */
  Fl_Color cursor_color_;

  /** \internal local undo event */
  Fl_Input_Undo_Action* undo_;
  Fl_Input_Undo_Action_List* undo_list_;
  Fl_Input_Undo_Action_List* redo_list_;

  /** \internal Horizontal cursor position in pixels while moving up or down. */
  static double up_down_pos;

  /** \internal Flag to remember last cursor move. */
  static int was_up_down;

  /* Convert a given text segment into the text that will be rendered on screen. */
  const char* expand(const char*, char*) const;

  /* Calculates the width in pixels of part of a text buffer. */
  double expandpos(const char*, const char*, const char*, int*) const;

  /* Mark a range of characters for update. */
  void minimal_update(int, int);

  /* Mark a range of characters for update. */
  void minimal_update(int p);

  /* Copy the value from a possibly static entry into the internal buffer. */
  void put_in_buffer(int newsize);

  /* Set the current font and font size. */
  void setfont() const;

protected:

  /* Find the start of a word. */
  int word_start(int i) const;

  /* Find the end of a word. */
  int word_end(int i) const;

  /* Find the start of a line. */
  int line_start(int i) const;

  /* Find the end of a line. */
  int line_end(int i) const;

  /* Draw the text in the passed bounding box. */
  void drawtext(int, int, int, int);

  /* Draw the text in the passed bounding box. */
  void drawtext(int, int, int, int, bool draw_active);

  /* Move the cursor to the column given by up_down_pos. */
  int up_down_position(int, int keepmark=0);

  /* Handle mouse clicks and mouse moves. */
  void handle_mouse(int, int, int, int, int keepmark=0);

  /* Handle all kinds of text field related events. */
  int handletext(int e, int, int, int, int);

  /* Check the when() field and do a callback if indicated. */
  void maybe_do_callback(Fl_Callback_Reason reason = FL_REASON_UNKNOWN);

  /** \internal Horizontal offset of text to left edge of widget. */
  int xscroll() const {return xscroll_;}

  /** \internal Vertical offset of text to top edge of widget. */
  int yscroll() const {return yscroll_;}
  void yscroll(int yOffset) { yscroll_ = yOffset; damage(FL_DAMAGE_EXPOSE);}

  /* Return the number of lines displayed on a single page.  */
  int linesPerPage();

  /* Apply the current undo/redo operation, called from undo() or redo() */
  int apply_undo();

public:

  /* Change the size of the widget. */
  void resize(int, int, int, int) FL_OVERRIDE;

  /* Constructor */
  Fl_Input_(int, int, int, int, const char* = 0);

  /* Destructor */
  ~Fl_Input_();

  /* Changes the widget text. */
  int value(const char*);

  /* Changes the widget text. */
  int value(const char*, int);

  /* Changes the widget text. */
  int value(int value);

  /* Changes the widget text. */
  int value(double value);

  /* Changes the widget text. */
  int static_value(const char*);

  /* Changes the widget text. */
  int static_value(const char*, int);

  /**
      Returns the text displayed in the widget.

      This function returns the current value, which is a pointer
      to the internal buffer and is valid only until the next event is
      handled.

      \return pointer to an internal buffer - do not free() this
      \see Fl_Input_::value(const char*)
  */
  const char* value() const {return value_;}

  int ivalue() const;

  double dvalue() const;

  /* Returns the Unicode character at index \p i. */
  unsigned int index(int i) const;

  /**
    Returns the number of bytes in value().

    This may be greater than <tt>strlen(value())</tt> if there are
    \c nul characters in the text.

    \return number of bytes in the text
  */
  int size() const {return size_;}

  /** Sets the width and height of this widget.
    \param [in] W, H new width and height
    \see Fl_Widget::size(int, int) */
  void size(int W, int H) { Fl_Widget::size(W, H); }

  /** Gets the maximum length of the input field in characters.
    \see maximum_size(int). */
  int maximum_size() const {return maximum_size_;}

  /** Sets the maximum length of the input field in characters.

    This limits the number of <b>characters</b> that can be inserted
    in the widget.

    Since FLTK 1.3 this is different than the buffer size, since one
    character can be more than one byte in UTF-8 encoding. In FLTK 1.1
    this was the same (one byte = one character).
  */
  void maximum_size(int m) {maximum_size_ = m;}

  /** Gets the position of the text cursor.
    \return the cursor position as an index in the range 0..size()
    \see insert_position(int, int)
  */
  int insert_position() const { return position_; }
  FL_DEPRECATED("since 1.4.0 - use insert_position() instead",
  int position() const ) { return insert_position(); }

  /** Gets the current selection mark.
    \return index into the text */
  int mark() const {return mark_;}

  /* Sets the index for the cursor and mark. */
  int insert_position(int p, int m);
  FL_DEPRECATED("since 1.4.0 - use insert_position(p, m) or Fl_Widget::position(x, y) instead",
  int position(int p, int m)) { return insert_position(p, m); }

  /** Sets the cursor position and mark.
    position(n) is the same as <tt>position(n, n)</tt>.
    \param p new index for cursor and mark
    \return 0 if no positions changed
    \see insert_position(int, int), insert_position(), mark(int)
  */
  int insert_position(int p) { return insert_position(p, p); }
  FL_DEPRECATED("since 1.4.0 - use insert_position(p) instead",
  int position(int p)) { return insert_position(p); }

  /** Sets the current selection mark.
    mark(n) is the same as <tt>insert_position(insert_position(),n)</tt>.
    \param m new index of the mark
    \return 0 if the mark did not change
    \see insert_position(), insert_position(int, int) */
  int mark(int m) {return insert_position(insert_position(), m);}

  /* Deletes text from \p b to \p e and inserts the new string \p text. */
  int replace(int b, int e, const char *text, int ilen=0);

  /**
    Deletes the current selection.

    This function deletes the currently selected text
    \e without storing it in the clipboard. To use the clipboard,
    you may call copy() first or copy_cuts() after
    this call.

    \return 0 if no data was copied
  */
  int cut() {return replace(insert_position(), mark(), 0);}

  /**
    Deletes the next \p n bytes rounded to characters before or after the cursor.

    This function deletes the currently selected text
    \e without storing it in the clipboard. To use the clipboard,
    you may call copy() first or copy_cuts() after
    this call.

    \param n number of bytes rounded to full characters and clamped to the buffer.
           A negative number will cut characters to the left of the cursor.
    \return 0 if no data was copied
  */
  int cut(int n) {return replace(insert_position(), insert_position()+n, 0);}

  /**
    Deletes all characters between index \p a and \p b.

    This function deletes the currently selected text
    \e without storing it in the clipboard. To use the clipboard,
    you may call copy() first or copy_cuts() after
    this call.

    \param a, b range of bytes rounded to full characters and clamped to the buffer
    \return 0 if no data was copied
  */
  int cut(int a, int b) {return replace(a, b, 0);}

  /**
    Inserts text at the cursor position.

    This function inserts the string in \p t at the cursor
    position() and moves the new position and mark to
    the end of the inserted text.

    \param [in] t text that will be inserted
    \param [in] l length of text, or 0 if the string is terminated by \c nul.
    \return 0 if no text was inserted
  */
  int insert(const char* t, int l=0){return replace(position_, mark_, t, l);}

  /* Append text at the end. */
  int append(const char* t, int l=0, char keep_selection=0);

  /* Put the current selection into the clipboard. */
  int copy(int clipboard);

  /* Undo previous changes to the text buffer. */
  int undo();

  /* Return true if the last operation can be undone. */
  bool can_undo() const;

  /* Redo previous undo operations. */
  int redo();

  /* Return true if there is a redo action in the list. */
  bool can_redo() const;

  /* Copy the yank buffer to the clipboard. */
  int copy_cuts();

  /** Return the shortcut key associated with this widget.
    \return shortcut keystroke
    \see Fl_Button::shortcut() */
  int shortcut() const {return shortcut_;}

  /**
    Sets the shortcut key associated with this widget.
    Pressing the shortcut key gives text editing focus to this widget.
    \param [in] s new shortcut keystroke
    \see Fl_Button::shortcut()
  */
  void shortcut(int s) {shortcut_ = s;}

  /** Gets the font of the text in the input field.
    \return the current Fl_Font index */
  Fl_Font textfont() const {return textfont_;}

  /** Sets the font of the text in the input field.
    The text font defaults to \c FL_HELVETICA.
    \param [in] s the new text font */
  void textfont(Fl_Font s) {textfont_ = s;}

  /** Gets the size of the text in the input field.
    \return the text height in pixels */
  Fl_Fontsize textsize() const {return textsize_;}

  /** Sets the size of the text in the input field.
    The text height defaults to \c FL_NORMAL_SIZE.
    \param [in] s the new font height in pixel units */
  void textsize(Fl_Fontsize s) {textsize_ = s;}

  /** Gets the color of the text in the input field.
    \return the text color
    \see textcolor(Fl_Color) */
  Fl_Color textcolor() const {return textcolor_;}

  /** Sets the color of the text in the input field.
    The text color defaults to \c FL_FOREGROUND_COLOR.
    \param [in] n new text color
    \see textcolor() */
  void textcolor(Fl_Color n) {textcolor_ = n;}

  /** Gets the color of the cursor.
    \return the current cursor color */
  Fl_Color cursor_color() const {return cursor_color_;}

  /** Sets the color of the cursor.
    The default color for the cursor is \c FL_BLACK.
    \param [in] n the new cursor color */
  void cursor_color(Fl_Color n) {cursor_color_ = n;}

  /** Gets the input field type.
    \return the current input type */
  int input_type() const {return type() & FL_INPUT_TYPE; }

  /** Sets the input field type.
    A redraw() is required to reformat the input field.
    \param [in] t new input type */
  void input_type(int t) { type((uchar)(t | readonly())); }

  /** Gets the read-only state of the input field.
    \return non-zero if this widget is read-only */
  int readonly() const { return type() & FL_INPUT_READONLY; }

  /** Sets the read-only state of the input field.
    \param [in] b if \p b is 0, the text in this widget can be edited by the user */
  void readonly(int b) { if (b) type((uchar)(type() | FL_INPUT_READONLY));
                         else type((uchar)(type() & ~FL_INPUT_READONLY)); }

  /**
    Gets  the word wrapping state of the input field.
    Word wrap is only functional with multi-line input fields.
  */
  int wrap() const { return type() & FL_INPUT_WRAP; }

  /**
    Sets the word wrapping state of the input field.
    Word wrap is only functional with multi-line input fields.
  */
  void wrap(int b) { if (b) type((uchar)(type() | FL_INPUT_WRAP));
                         else type((uchar)(type() & ~FL_INPUT_WRAP)); }

  /**
    Sets whether the Tab key does focus navigation,
    or inserts tab characters into Fl_Multiline_Input.

    By default this flag is enabled to provide the 'normal' behavior
    most users expect; Tab navigates focus to the next widget.
    To inserting an actual Tab character, users can use Ctrl-I
    or copy/paste.

    Disabling this flag gives the old FLTK behavior where Tab
    inserts a tab character into the text field, in which case
    only the mouse can be used to navigate to the next field.

    History: This flag was provided for backwards support of FLTK's old 1.1.x
    behavior where Tab inserts a tab character instead of navigating
    focus to the next widget. This behavior was unique to Fl_Multiline_Input.
    With the advent of Fl_Text_Editor, this old behavior has been deprecated.

    \param [in] val If \p val is 1, Tab advances focus (default).<BR>
                    If \p val is 0, Tab inserts a tab character (old FLTK behavior).

    \see tab_nav(), Fl::OPTION_ARROW_FOCUS.
  */
  void tab_nav(int val) {
    tab_nav_ = val;
  }

  /**
    Gets whether the Tab key causes focus navigation in multiline input fields or not.

    If enabled (default), hitting Tab causes focus navigation to the next widget.

    If disabled, hitting Tab inserts a tab character into the text field.
    \returns 1 if Tab advances focus (default), 0 if Tab inserts tab characters.

    \see tab_nav(int), Fl::OPTION_ARROW_FOCUS.
  */
  int tab_nav() const {
    return tab_nav_;
  }
};

#endif
