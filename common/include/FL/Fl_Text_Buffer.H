//
// Header file for Fl_Text_Buffer class.
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
 Fl_Text_Buffer, Fl_Text_Selection widget . */

#ifndef FL_TEXT_BUFFER_H
#define FL_TEXT_BUFFER_H

#include <stdarg.h>     /* va_list */
#include "fl_attr.h"    /* Doxygen can't find <FL/fl_attr.h> */

#undef ASSERT_UTF8

#ifdef ASSERT_UTF8
# include <assert.h>
# define IS_UTF8_ALIGNED(a) if (a && *a) assert(fl_utf8len(*(a))>0);
# define IS_UTF8_ALIGNED2(a, b) if (b>=0 && b<a->length()) assert(fl_utf8len(a->byte_at(b))>0);
#else
# define IS_UTF8_ALIGNED(a)
# define IS_UTF8_ALIGNED2(a, b)
#endif


/*
 "character size" is the size of a UTF-8 character in bytes
 "character width" is the width of a Unicode character in pixels
 "column" was originally defined as a character offset from the left margin.
 It was identical to the byte offset. In UTF-8, we have neither a byte offset
 nor truly fixed width fonts (*). Column could be a pixel value multiplied with
 an average character width (which is a bearable approximation).

 * in Unicode, there are no fixed width fonts! Even if the ASCII characters may
   happen to be all the same width in pixels, Chinese characters surely are not.
   There are plenty of exceptions, like ligatures, that make special handling of
   "fixed" character widths a nightmare. I decided to remove all references to
   fixed fonts and see "columns" as a multiple of the average width of a
   character in the main font.
     - Matthias
 */


/* Maximum length in characters of a tab or control character expansion
   of a single buffer character */
#define FL_TEXT_MAX_EXP_CHAR_LEN 20

#include "Fl_Export.H"

class Fl_Text_Undo_Action_List;
class Fl_Text_Undo_Action;

/**
  \class Fl_Text_Selection
  \brief This is an internal class for Fl_Text_Buffer to manage text selections.

  All methods use byte (not UTF-8 character) offsets and start at 0. This
  class works correctly with UTF-8 strings assuming that the parameters
  for all calls are on character boundaries.

  If the selection is inactive (not currently used), then selected()
  returns \p false and start() and end() return 0 (zero).

  The stored offsets are in ascending order, hence the following conditions
  are true (pseudo code):
  \code
    if ( !selected() ) :  (start() == 0) && (end() == 0) && (start() == end())
    if (  selected() ) :  start() < end()
    always             :  0 <= start() <= end()
    always             :  length() == end() - start()
  \endcode

  The selection size in bytes can always (unconditionally) be computed by
  \code
    int size = sel->end() - sel->start();
  \endcode

  \see length()

  \note
    The \b protected member variables \p mStart and \p mEnd are not
    necessarily 0 (zero) if mSelected == \p false because they are
    not cleared when \p selected(false) is called (as of Jul 2017).
    This may be changed in the future.
*/
class FL_EXPORT Fl_Text_Selection {
  friend class Fl_Text_Buffer;

public:

  // Sets the selection range and selected().
  void set(int startpos, int endpos);

  // Updates a selection after text was modified.
  void update(int pos, int nDeleted, int nInserted);

  /**
    \brief Returns the byte offset to the first selected character.

    The returned offset is only valid if selected() returns true.
    If the selection is not valid the returned offset is 0 since FLTK 1.4.0.

    \note In FLTK 1.3.x the returned offset could be non-zero even if
      selected() would have returned 0.

    \return byte offset or 0 if not selected.
  */
  int start() const { return mSelected ? mStart : 0; }

  /**
    \brief Returns the byte offset to the character after the last selected character.

    The returned offset is only valid if selected() returns true (non-zero).
    The offset is 0 if no text is selected (since FLTK 1.4.0).

    \note In FLTK 1.3.x the returned offset could be non-zero even if
      selected() would have returned 0.

    \return byte offset or 0 if not selected.
  */
  int end() const { return mSelected ? mEnd : 0; }

  /**
   \brief Returns true if any text is selected.
   \return  \p true if any text has been selected, or \p false
            if no text is selected.
  */
  bool selected() const { return mSelected; }

  /**
    \brief Modifies the 'selected' flag.
    \param b new flag
  */
  void selected(bool b) { mSelected = b; }

  /**
    \brief Returns the size in bytes of the selection.

    This is a convenience method. It always returns the same as
    \code
      end() - start()
    \endcode
    and it returns 0 if selected() == false.

    \return size in bytes or 0 if not selected.

    \since FLTK 1.4.0
  */
  int length() const { return mSelected ? mEnd - mStart : 0; }

  // Returns true if position \p pos is in this Fl_Text_Selection.
  int includes(int pos) const;

  // Returns true if selected() and the positions of this selection.
  int selected(int *startpos, int *endpos) const;
  FL_DEPRECATED("since 1.4.0 - use selected(startpos, endpos) instead",
  int position(int *startpos, int *endpos) const) { return selected(startpos, endpos); }

protected:

  int mStart;         ///< byte offset to the first selected character
  int mEnd;           ///< byte offset to the character after the last selected character
  bool mSelected;     ///< this flag is set if any text is selected
};


typedef void (*Fl_Text_Modify_Cb)(int pos, int nInserted, int nDeleted,
                                  int nRestyled, const char* deletedText,
                                  void* cbArg);


typedef void (*Fl_Text_Predelete_Cb)(int pos, int nDeleted, void* cbArg);


/**
 This class manages Unicode text displayed in one or more Fl_Text_Display widgets.

 All text in Fl_Text_Buffer must be encoded in UTF-8. All indices used in the
 function calls must be aligned to the start of a UTF-8 sequence. All indices
 and pointers returned will be aligned. All functions that return a single
 character will return that in an unsigned int in UCS-4 encoding.

 The Fl_Text_Buffer class is used by the Fl_Text_Display and Fl_Text_Editor
 to manage complex text data and is based upon the excellent NEdit text
 editor engine - see https://sourceforge.net/projects/nedit/.
 */
class FL_EXPORT Fl_Text_Buffer {
public:

  /**
   Create an empty text buffer of a pre-determined size.
   \param requestedSize use this to avoid unnecessary re-allocation
    if you know exactly how much the buffer will need to hold
   \param preferredGapSize Initial size for the buffer gap (empty space
    in the buffer where text might be inserted
    if the user is typing sequential characters)
   */
  Fl_Text_Buffer(int requestedSize = 0, int preferredGapSize = 1024);

  /**
   Frees a text buffer
   */
  ~Fl_Text_Buffer();

  /**
   \brief Returns the number of bytes in the buffer.
   \return size of text in bytes
   */
  int length() const { return mLength; }

  /**
   \brief Get a copy of the entire contents of the text buffer.
   Memory is allocated to contain the returned string, which the caller
   must free.
   \return newly allocated text buffer - must be free'd, text is UTF-8
   */
  char* text() const;

  /**
   Replaces the entire contents of the text buffer.
   \param text Text must be valid UTF-8. If null, an empty string is substituted.
   */
  void text(const char* text);

  /**
   \brief Get a copy of a part of the text buffer.
   Return a copy of the text between \p start and \p end character positions
   from text buffer \p buf. Positions start at 0, and the range does not
   include the character pointed to by \p end.
   When you are done with the text, free it using the free() function.
   \param start byte offset to first character
   \param end byte offset after last character in range
   \return newly allocated text buffer - must be free'd, text is UTF-8
   */
  char* text_range(int start, int end) const;

  /**
   Returns the character at the specified position \p pos in the buffer.
   Positions start at 0.
   \param pos byte offset into buffer, \p pos must be at a UTF-8 character boundary
   \return Unicode UCS-4 encoded character
   */
  unsigned int char_at(int pos) const;

  /**
   Returns the raw byte at the specified position pos in the buffer.
   Positions start at 0.
   \param pos byte offset into buffer
   \return unencoded raw byte
   */
  char byte_at(int pos) const;

  /**
   Convert a byte offset in buffer into a memory address.
   \param pos byte offset into buffer
   \return byte offset converted to a memory address
   */
  const char *address(int pos) const
  { return (pos < mGapStart) ? mBuf+pos : mBuf+pos+mGapEnd-mGapStart; }

  /**
   Convert a byte offset in buffer into a memory address.
   \param pos byte offset into buffer
   \return byte offset converted to a memory address
   */
  char *address(int pos)
  { return (pos < mGapStart) ? mBuf+pos : mBuf+pos+mGapEnd-mGapStart; }

  /**
   Inserts null-terminated string \p text at position \p pos.
   \param pos insertion position as byte offset (must be UTF-8 character aligned)
   \param text UTF-8 encoded text
   \param insertedLength number of bytes to insert, or -1 to indicate \p text is null-terminated
   */
  void insert(int pos, const char* text, int insertedLength = -1);

  /**
   Appends the text string to the end of the buffer.
   \param t UTF-8 encoded text
   \param addedLength number of bytes to append, or -1 to indicate \p t is null-terminated
   */
  void append(const char* t, int addedLength = -1) { insert(length(), t, addedLength); }

  void vprintf(const char *fmt, va_list ap);
  void printf(const char* fmt, ...);

  /**
   Deletes a range of characters in the buffer.
   \param start byte offset to first character to be removed
   \param end byte offset to character after last character to be removed
   */
  void remove(int start, int end);

  /**
   Deletes the characters between \p start and \p end, and inserts the
   null-terminated string \p text in their place in the buffer.
   \param start byte offset to first character to be removed and new insert position
   \param end byte offset to character after last character to be removed
   \param text UTF-8 encoded text
   \param insertedLength number of bytes to insert, or -1 to indicate \p text is null-terminated
   */
  void replace(int start, int end, const char *text, int insertedLength = -1);

  /**
   Copies text from another Fl_Text_Buffer to this one.
   \param fromBuf source text buffer, may be the same as this
   \param fromStart byte offset into buffer
   \param fromEnd byte offset into buffer
   \param toPos destination byte offset into buffer
   */
  void copy(Fl_Text_Buffer* fromBuf, int fromStart, int fromEnd, int toPos);

  /**
   Undo text modification according to the undo variables or insert text
   from the undo buffer
   */
  int undo(int *cp=0);

  /**
   Check if undo is enabled and if the last action can be undone.
   \see canUndo()
   */
  bool can_undo() const;

  /**
   Redo previous undo action.
   */
  int redo(int *cp=0);

  /**
   Check if undo is enabled and if the last undo action can be redone.
   \see canUndo()
   */
  bool can_redo() const;

  /**
   Enable or disable undo actions for this text buffer.
   Undo actions are enable for text buffer by default. If used as a style buffer
   in Fl_Text_Display, undo actions are disabled as they are handled by the
   text buffer.
   \see can_undo()
   */
  void canUndo(char flag=1);

  /**
   Inserts a file at the specified position.
   Returns
    - 0 on success
    - non-zero on error (strerror() contains reason)
    - 1 indicates open for read failed (no data loaded)
    - 2 indicates error occurred while reading data (data was partially loaded)

   File can be UTF-8 or CP1252 encoded.
   If the input file is not UTF-8 encoded, the Fl_Text_Buffer widget will
   contain data transcoded to UTF-8. By default, the message
   Fl_Text_Buffer::file_encoding_warning_message
   will warn the user about this.
   \see input_file_was_transcoded and transcoding_warning_action.
   */
  int insertfile(const char *file, int pos, int buflen = 128*1024);

  /**
   Appends the named file to the end of the buffer. See also insertfile().
   */
  int appendfile(const char *file, int buflen = 128*1024)
  { return insertfile(file, length(), buflen); }

  /**
   Loads a text file into the buffer. See also insertfile().
   */
  int loadfile(const char *file, int buflen = 128*1024)
  { select(0, length()); remove_selection(); return appendfile(file, buflen); }

  /**
   Writes the specified portions of the text buffer to a file.
   Returns
    - 0 on success
    - non-zero on error (strerror() contains reason)
    - 1 indicates open for write failed (no data saved)
    - 2 indicates error occurred while writing data (data was partially saved)

   \see savefile(const char *file, int buflen)
   */
  int outputfile(const char *file, int start, int end, int buflen = 128*1024);

  /**
   Saves a text file from the current buffer.
   Returns
    - 0 on success
    - non-zero on error (strerror() contains reason)
    - 1 indicates open for write failed (no data saved)
    - 2 indicates error occurred while writing data (data was partially saved)

   \see outputfile(const char *file, int start, int end, int buflen)
   */
  int savefile(const char *file, int buflen = 128*1024)
  { return outputfile(file, 0, length(), buflen); }

  /**
   Gets the tab width.

   The tab width is measured in characters. The pixel position is
   calculated using an average character width.
   */
  int tab_distance() const { return mTabDist; }

  /**
   Set the hardware tab distance (width) used by all displays for this buffer,
   and used in computing offsets for rectangular selection operations.
   */
  void tab_distance(int tabDist);

  /**
   Selects a range of characters in the buffer.
   */
  void select(int start, int end);

  /**
   Returns a non-zero value if text has been selected, 0 otherwise.
   */
  int selected() const { return mPrimary.selected(); }

  /**
   Cancels any previous selection on the primary text selection object.
   */
  void unselect();

  /**
   Gets the selection position.
   */
  int selection_position(int* start, int* end);

  /**
   Returns the currently selected text.

   When you are done with the text, free it using the free() function.
   */
  char* selection_text();

  /**
   Removes the text in the primary selection.
   */
  void remove_selection();

  /**
   Replaces the text in the primary selection.
   */
  void replace_selection(const char* text);

  /**
   Selects a range of characters in the secondary selection.
   */
  void secondary_select(int start, int end);

  /**
   Returns a non-zero value if text has been selected in the secondary
   text selection, 0 otherwise.
   */
  int secondary_selected() { return mSecondary.selected(); }

  /**
   Clears any selection in the secondary text selection object.
   */
  void secondary_unselect();

  /**
   Returns the current selection in the secondary text selection object.
   */
  int secondary_selection_position(int* start, int* end);

  /**
   Returns the text in the secondary selection.

   When you are done with the text, free it using the free() function.
   */
  char* secondary_selection_text();

  /**
   Removes the text from the buffer corresponding to the secondary text
   selection object.
   */
  void remove_secondary_selection();

  /**
   Replaces the text from the buffer corresponding to the secondary
   text selection object with the new string \p text.
   */
  void replace_secondary_selection(const char* text);

  /**
   Highlights the specified text within the buffer.
   */
  void highlight(int start, int end);

  /**
   Returns a non-zero value if text has been highlighted, 0 otherwise.
   */
  int highlight() { return mHighlight.selected(); }

  /**
   Unhighlights text in the buffer.
   */
  void unhighlight();

  /**
   Highlights the specified text between \p start and \p end within the buffer.
   */
  int highlight_position(int* start, int* end);

  /**
   Returns the highlighted text.

   When you are done with the text, free it using the free() function.
   */
  char* highlight_text();

  /**
   Adds a callback function that is called whenever the text buffer is modified.

   The callback function is declared as follows:

   \code
   typedef void (*Fl_Text_Modify_Cb)(int pos, int nInserted, int nDeleted,
      int nRestyled, const char* deletedText,
      void* cbArg);
   \endcode
   */
  void add_modify_callback(Fl_Text_Modify_Cb bufModifiedCB, void* cbArg);

  /**
   Removes a modify callback.
   */
  void remove_modify_callback(Fl_Text_Modify_Cb bufModifiedCB, void* cbArg);

  /**
   Calls all modify callbacks that have been registered using
   the add_modify_callback() method.
   */
  void call_modify_callbacks() { call_modify_callbacks(0, 0, 0, 0, 0); }

  /**
   Adds a callback routine to be called before text is deleted from the buffer.
   */
  void add_predelete_callback(Fl_Text_Predelete_Cb bufPredelCB, void* cbArg);

  /**
   Removes a callback routine \p bufPreDeleteCB associated with argument \p cbArg
   to be called before text is deleted from the buffer.
   */
  void remove_predelete_callback(Fl_Text_Predelete_Cb predelCB, void* cbArg);

  /**
   Calls the stored pre-delete callback procedure(s) for this buffer to update
   the changed area(s) on the screen and any other listeners.
   */
  void call_predelete_callbacks() { call_predelete_callbacks(0, 0); }

  /**
   Returns the text from the entire line containing the specified
   character position.

   When you are done with the text, free it using the free() function.
   \param pos byte index into buffer
   \return copy of UTF-8 text, must be free'd
   */
  char* line_text(int pos) const;

  /**
   Returns the position of the start of the line containing position \p pos.
   \param pos byte index into buffer
   \return byte offset to line start
   */
  int line_start(int pos) const;

  /**
   Finds and returns the position of the end of the line containing position
   \p pos (which is either a pointer to the newline character ending the line
   or a pointer to one character beyond the end of the buffer).
   \param pos byte index into buffer
   \return byte offset to line end
   */
  int line_end(int pos) const;

  /**
   Returns the position corresponding to the start of the word.
   \param pos byte index into buffer
   \return byte offset to word start
   */
  int word_start(int pos) const;

  /**
   Returns the position corresponding to the end of the word.
   \param pos byte index into buffer
   \return byte offset to word end
   */
  int word_end(int pos) const;

  /**
   Count the number of displayed characters between buffer position
   \p lineStartPos and \p targetPos.

   Displayed characters are the characters shown on the screen to represent
   characters in the buffer, where tabs and control characters are expanded.
   */
  int count_displayed_characters(int lineStartPos, int targetPos) const;

  /**
   Count forward from buffer position \p startPos in displayed characters.

   Displayed characters are the characters shown on the screen to represent
   characters in the buffer, where tabs and control characters are expanded.
   \param lineStartPos byte offset into buffer
   \param nChars number of bytes that are sent to the display
   \return byte offset in input after all output bytes are sent
   */
  int skip_displayed_characters(int lineStartPos, int nChars);

  /**
   Counts the number of newlines between \p startPos and \p endPos in buffer.
   The character at position \p endPos is not counted.
   */
  int count_lines(int startPos, int endPos) const;

  /**
   Estimate the number of newlines between \p startPos and \p endPos in buffer.
   This call takes line wrapping into account. It assumes a line break at every
   `lineLen` characters after the beginning of a line.
   */
  int estimate_lines(int startPos, int endPos, int lineLen) const;

  /**
   Finds the first character of the line \p nLines forward from \p startPos
   in the buffer and returns its position.
   */
  int skip_lines(int startPos, int nLines);

  /**
   Finds and returns the position of the first character of the line \p nLines
   backwards from \p startPos (not counting the character pointed to by
   \p startpos if that is a newline) in the buffer.
   \p nLines == 0 means find the beginning of the line.
   */
  int rewind_lines(int startPos, int nLines);

  /**
   Finds the next occurrence of the specified character.
   Search forwards in buffer for character \p searchChar, starting
   with the character \p startPos, and returning the result in \p foundPos.
   Returns 1 if found, 0 if not.
   The difference between this and search_forward() is that it's optimized
   for single characters. The overall performance of the text widget is
   dependent on its ability to count lines quickly, hence searching for a
   single character: newline.
   \param startPos byte offset to start position
   \param searchChar UCS-4 character that we want to find
   \param foundPos byte offset where the character was found
   \return 1 if found, 0 if not
   */
  int findchar_forward(int startPos, unsigned searchChar, int* foundPos) const;

  /**
   Search backwards in buffer \p buf for character \p searchChar, starting
   with the character \e before \p startPos, returning the result in \p foundPos.

   Returns 1 if found, 0 if not.  The difference between this and
   search_backward() is that it's optimized for single characters.  The
   overall performance of the text widget is dependent on its ability to
   count lines quickly, hence searching for a single character: newline.
   \param startPos byte offset to start position
   \param searchChar UCS-4 character that we want to find
   \param foundPos byte offset where the character was found
   \return 1 if found, 0 if not
   */
  int findchar_backward(int startPos, unsigned int searchChar, int* foundPos) const;

  /**
   Search forwards in buffer for string \p searchString, starting with the
   character \p startPos, and returning the result in \p foundPos.

   Returns 1 if found, 0 if not.
   \param startPos byte offset to start position
   \param searchString UTF-8 string that we want to find
   \param foundPos byte offset where the string was found
   \param matchCase if set, match character case
   \return 1 if found, 0 if not
   */
  int search_forward(int startPos, const char* searchString, int* foundPos,
                     int matchCase = 0) const;

  /**
   Search backwards in buffer for string \p searchString, starting with
   the character \e at \p startPos, returning the result in \p foundPos.

   Returns 1 if found, 0 if not.
   \param startPos byte offset to start position
   \param searchString UTF-8 string that we want to find
   \param foundPos byte offset where the string was found
   \param matchCase if set, match character case
   \return 1 if found, 0 if not
   */
  int search_backward(int startPos, const char* searchString, int* foundPos,
                      int matchCase = 0) const;

  /**
   Returns the primary selection.
   */
  const Fl_Text_Selection* primary_selection() const { return &mPrimary; }

  /**
   Returns the primary selection.
   */
  Fl_Text_Selection* primary_selection() { return &mPrimary; }

  /**
   Returns the secondary selection.
   */
  const Fl_Text_Selection* secondary_selection() const { return &mSecondary; }

  /**
   Returns the current highlight selection.
   */
  const Fl_Text_Selection* highlight_selection() const { return &mHighlight; }

  /**
   Returns the index of the previous character.
   \param ix index to the current character
   */
  int prev_char(int ix) const;
  int prev_char_clipped(int ix) const;

  /**
   Returns the index of the next character.
   \param ix index to the current character
   */
  int next_char(int ix) const;
  int next_char_clipped(int ix) const;

  /**
   Align an index into the buffer to the current or previous UTF-8 boundary.
   */
  int utf8_align(int) const;

  /**
   \brief true if the loaded file has been transcoded to UTF-8.
   */
  int input_file_was_transcoded;

  /** This message may be displayed using the fl_alert() function when a file
   which was not UTF-8 encoded is input.
   */
  static const char* file_encoding_warning_message;

  /**
   \brief Pointer to a function called after reading a non UTF-8 encoded file.

   This function is called after reading a file if the file content
   was transcoded to UTF-8. Its default implementation calls fl_alert()
   with the text of \ref file_encoding_warning_message. No warning message is
   displayed if this pointer is set to NULL. Use \ref input_file_was_transcoded
   to be informed if file input required transcoding to UTF-8.
   */
  void (*transcoding_warning_action)(Fl_Text_Buffer*);
  bool is_word_separator(int pos) const;

protected:

  /**
   Calls the stored modify callback procedure(s) for this buffer to update the
   changed area(s) on the screen and any other listeners.
   */
  void call_modify_callbacks(int pos, int nDeleted, int nInserted,
                             int nRestyled, const char* deletedText) const;

  /**
   Calls the stored pre-delete callback procedure(s) for this buffer to update
   the changed area(s) on the screen and any other listeners.
   */
  void call_predelete_callbacks(int pos, int nDeleted) const;

  /**
   Internal (non-redisplaying) version of insert().

   Returns the length of text inserted (this is just strlen(\p text) if
   \p insertedLength == -1, however
   this calculation can be expensive and the length will be required by any
   caller who will continue on to call redisplay). \p pos must be contiguous
   with the existing text in the buffer (i.e. not past the end).
   \return the number of bytes inserted
   */
  int insert_(int pos, const char* text, int insertedLength = -1);

  /**
   Internal (non-redisplaying) version of remove().

   Removes the contents of the buffer between \p start and \p end (and moves
   the gap to the site of the delete).
   */
  void remove_(int start, int end);

  /**
   Calls the stored redisplay procedure(s) for this buffer to update the
   screen for a change in a selection.
   */
  void redisplay_selection(Fl_Text_Selection* oldSelection,
                           Fl_Text_Selection* newSelection) const;

  /**
   Move the gap to start at a new position.
   */
  void move_gap(int pos);

  /**
   Reallocates the text storage in the buffer to have a gap starting at \p newGapStart
   and a gap size of \p newGapLen, preserving the buffer's current contents.
   */
  void reallocate_with_gap(int newGapStart, int newGapLen);

  char* selection_text_(Fl_Text_Selection* sel) const;

  /**
   Removes the text from the buffer corresponding to \p sel.
   */
  void remove_selection_(Fl_Text_Selection* sel);

  /**
   Replaces the \p text in selection \p sel.
   */
  void replace_selection_(Fl_Text_Selection* sel, const char* text);

  /**
   Updates all of the selections in the buffer for changes in the buffer's text
   */
  void update_selections(int pos, int nDeleted, int nInserted);

  /**
   Apply the current undo/redo operation, called from undo() or redo().
   */
  int apply_undo(Fl_Text_Undo_Action* action, int* cursorPos);

  Fl_Text_Selection mPrimary;     /**< highlighted areas */
  Fl_Text_Selection mSecondary;   /**< highlighted areas */
  Fl_Text_Selection mHighlight;   /**< highlighted areas */
  int mLength;                    /**< length of the text in the buffer (the length
                                       of the buffer itself must be calculated:
                                       gapEnd - gapStart + length) */
  char* mBuf;                     /**< allocated memory where the text is stored */
  int mGapStart;                  /**< points to the first character of the gap */
  int mGapEnd;                    /**< points to the first character after the gap */
  // The hardware tab distance used by all displays for this buffer,
  // and used in computing offsets for rectangular selection operations.
  int mTabDist;                   /**< equiv. number of characters in a tab */
  int mNModifyProcs;              /**< number of modify-redisplay procs attached */
  Fl_Text_Modify_Cb *mModifyProcs;/**< procedures to call when buffer is
                                       modified to redisplay contents */
  void** mCbArgs;                 /**< caller arguments for modifyProcs above */
  int mNPredeleteProcs;           /**< number of pre-delete procs attached */
  Fl_Text_Predelete_Cb *mPredeleteProcs; /**< procedure to call before text is deleted
                                       from the buffer; at most one is supported. */
  void **mPredeleteCbArgs;        /**< caller argument for pre-delete proc above */
  int mCursorPosHint;             /**< hint for reasonable cursor position after
                                       a buffer modification operation */
  char mCanUndo;                  /**< if this buffer is used for attributes, it must
                                       not do any undo calls */
  int mPreferredGapSize;          /**< the default allocation for the text gap is 1024
                                       bytes and should only be increased if frequent
                                       and large changes in buffer size are expected */
  Fl_Text_Undo_Action* mUndo;     /**< local undo event */
  Fl_Text_Undo_Action_List* mUndoList; /**< List of undo event */
  Fl_Text_Undo_Action_List* mRedoList; /**< List of redo event */
};

#endif
