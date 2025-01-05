//
// Header file for Fl_Text_Editor class.
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
   Fl_Text_Editor widget . */


#ifndef FL_TEXT_EDITOR_H
#define FL_TEXT_EDITOR_H

#include "Fl_Text_Display.H"

// key will match in any state
#define FL_TEXT_EDITOR_ANY_STATE  (-1L)

/**
  This is the FLTK text editor widget.

  It allows the user to edit multiple lines of text and supports highlighting
  and scrolling. The buffer that is displayed in the widget is managed
  by the Fl_Text_Buffer class.
*/
class FL_EXPORT Fl_Text_Editor : public Fl_Text_Display {
  public:
    /** Key function binding callback type. */
    typedef int (*Key_Func)(int key, Fl_Text_Editor* editor);

    /** Simple linked list item associating a key/state to a function. */
    struct Key_Binding {
      int          key;         ///< the key pressed
      int          state;       ///< the state of key modifiers
      Key_Func     function;    ///< associated function
      Key_Binding* next;        ///< next key binding in the list
    };

    Fl_Text_Editor(int X, int Y, int W, int H, const char* l = 0);
    ~Fl_Text_Editor() { remove_all_key_bindings(); }
    int handle(int e) FL_OVERRIDE;
    /**
        Sets the current insert mode; if non-zero, new text
        is inserted before the current cursor position. Otherwise, new
        text replaces text at the current cursor position.
    */
    void insert_mode(int b) { insert_mode_ = b; }
    /**
        Gets the current insert mode; if non-zero, new text
        is inserted before the current cursor position. Otherwise, new
        text replaces text at the current cursor position.
    */
    int insert_mode() { return insert_mode_; }
    void tab_nav(int val);
    int tab_nav() const;
    void add_key_binding(int key, int state, Key_Func f, Key_Binding** list);
    /** Adds a \p key of state \p state with the function \p f. */
    void add_key_binding(int key, int state, Key_Func f)
      { add_key_binding(key, state, f, &key_bindings); }
    void remove_key_binding(int key, int state, Key_Binding** list);
    /** Removes the key binding associated with the key "key" of state "state". */
    void remove_key_binding(int key, int state)
      { remove_key_binding(key, state, &key_bindings); }
    void remove_all_key_bindings(Key_Binding** list);
    /** Removes all of the key bindings associated with the text editor or list. */
    void remove_all_key_bindings() { remove_all_key_bindings(&key_bindings); }
    void add_default_key_bindings(Key_Binding** list);
    Key_Func bound_key_function(int key, int state, Key_Binding* list) const;
    /**  Returns the function associated with a key binding. */
    Key_Func bound_key_function(int key, int state) const
      { return bound_key_function(key, state, key_bindings); }
    /**  Sets the default key function for unassigned keys. */
    void default_key_function(Key_Func f) { default_key_function_ = f; }

    // functions for the built in default bindings
    static int kf_default(int c, Fl_Text_Editor* e);
    static int kf_ignore(int c, Fl_Text_Editor* e);
    static int kf_backspace(int c, Fl_Text_Editor* e);
    static int kf_enter(int c, Fl_Text_Editor* e);
    static int kf_move(int c, Fl_Text_Editor* e);
    static int kf_shift_move(int c, Fl_Text_Editor* e);
    static int kf_ctrl_move(int c, Fl_Text_Editor* e);
    static int kf_c_s_move(int c, Fl_Text_Editor* e);
    static int kf_meta_move(int c, Fl_Text_Editor* e);
    static int kf_m_s_move(int c, Fl_Text_Editor* e);
    static int kf_home(int, Fl_Text_Editor* e);
    static int kf_end(int c, Fl_Text_Editor* e);
    static int kf_left(int c, Fl_Text_Editor* e);
    static int kf_up(int c, Fl_Text_Editor* e);
    static int kf_right(int c, Fl_Text_Editor* e);
    static int kf_down(int c, Fl_Text_Editor* e);
    static int kf_page_up(int c, Fl_Text_Editor* e);
    static int kf_page_down(int c, Fl_Text_Editor* e);
    static int kf_insert(int c, Fl_Text_Editor* e);
    static int kf_delete(int c, Fl_Text_Editor* e);
    static int kf_copy(int c, Fl_Text_Editor* e);
    static int kf_cut(int c, Fl_Text_Editor* e);
    static int kf_paste(int c, Fl_Text_Editor* e);
    static int kf_select_all(int c, Fl_Text_Editor* e);
    static int kf_undo(int c, Fl_Text_Editor* e);
    static int kf_redo(int c, Fl_Text_Editor* e);

  protected:
    int handle_key();
    void maybe_do_callback(Fl_Callback_Reason reason = FL_REASON_CHANGED);

#ifndef FL_DOXYGEN
    int insert_mode_;
    Key_Binding* key_bindings;
#endif

    /** Global key binding list.

      Derived classes can add key bindings for all Fl_Text_Editor widgets
      by adding a Key_Binding to this list.

      \see add_key_binding(int key, int state, Key_Func f, Key_Binding** list);
    */
    static Key_Binding* global_key_bindings;

#ifndef FL_DOXYGEN
    Key_Func default_key_function_;
#endif
};

#endif

