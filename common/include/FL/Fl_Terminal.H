//
// Fl_Terminal - A terminal widget for Fast Light Tool Kit (FLTK).
//
// Copyright 2022 by Greg Ercolano.
// Copyright 2024 by Bill Spitzak and others.
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

/** \file
   Fl_Terminal widget.
*/

#ifndef Fl_Terminal_H
#define Fl_Terminal_H

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Scrollbar.H>
#include <FL/Fl_Rect.H>

#include <stdarg.h>             // va_list (MinGW)

/** \class Fl_Terminal

  \brief Terminal widget supporting Unicode/utf-8, ANSI/xterm escape codes with full RGB color control.

  \section Fl_Terminal

  \image html Fl_Terminal-demo.png "Fl_Terminal widget showing a linux manual page"
  \image latex Fl_Terminal-demo.png "Fl_Terminal widget showing a linux manual page" width=6cm

  Fl_Terminal is an output-only text widget supporting ASCII and UTF-8/Unicode.
  It supports most terminal text features, such as most VT100/xterm style escape sequences
  (see \ref Fl_Terminal_escape_codes), text colors/attributes, scrollback history, mouse selection, etc.

  It is recommended that accessing features such as setting text colors and cursor positioning
  is best done with ANSI/XTERM escape sequences.  But if one sets ansi(false) then this is
  not possible, so the public API can be used for common operations, e.g.
  \par
  Public API              | ESC code equivalent      | Description                 |
  ------------------------|--------------------------|-----------------------------|
  clear_screen_home()     | ESC [ H ESC [ 2 J        | Clear screen, home cursor   |
  cursor_home()           | ESC [ H                  | Home the cursor             |
  clear_history()         | ESC [ 3 J                | Clear scrollback history    |
  reset_terminal()        | ESC [ c                  | Reset terminal              |

  To access more advanced API calls, one can derive a class from Fl_Terminal to
  access protected methods manipulate the terminal more directly, e.g.
  \par
  Protected API                | ESC code equiv.  | Description                             |
  -----------------------------|------------------|-----------------------------------------|
  current_style()              | ESC [ # m        | Set text attributes                     |
  clear_eod()                  | ESC [ 0 J        | Clear from cursor to end of display     |
  clear_sod()                  | ESC [ 1 J        | Clear from cursor to start of display   |
  clear_eol()                  | ESC [ 0 K        | Clear from cursor to end of line        |
  clear_sol()                  | ESC [ 1 K        | Clear from cursor to start of line      |
  clear_line()                 | ESC [ 2 K        | Clear line cursor is on                 |
  scroll(int) // >0 for up     | ESC [ 1 S        | Scroll up one line                      |
  scroll(int) // <0 for down   | ESC [ 1 T        | Scroll down one line                    |
  cursor_left()                | ESC [ 1 D        | Move cursor left (no wrap)              |
  cursor_right()               | ESC [ 1 C        | Move cursor right (no wrap)             |
  cursor_up()                  | ESC [ 1 B        | Move cursor up (no scroll or wrap)      |
  cursor_down()                | ESC [ 1 A        | Move cursor down (no scroll or wrap)    |
  cursor_row() cursor_col()    | ESC [ # ; # H    | Move cursor to row# / column#           |
  insert_char()                | ESC [ # @        | Insert a char at cursor position        |
  delete_chars()               | ESC [ # P        | Delete chars at cursor position         |
  insert_rows()                | ESC [ # L        | Insert rows at cursor position          |
  delete_rows()                | ESC [ # M        | Delete rows at cursor position          |
  etc...                       | etc...           | etc...                                  |

  Many commonly used API functions are public, such as textfgcolor() for setting
  text colors. Others, such as cursor_up() are protected to prevent common misuse, and are
  available only to subclasses.

  Some common operations:
  - Set the terminal's background color, see color(Fl_Color)
  - Set the terminal's default text color, see textfgcolor_default(Fl_Color)
  - Printing text to the terminal, see Fl_Terminal::printf() and Fl_Terminal::append()
  - Clearing the screen, see clear()
  - Getting the terminal's buffer contents, see text()
  - Getting single utf8 characters by row/col from the terminal display, see utf8_char_at_disp()
  - Getting the text from a text selection, see get_selection()

  For applications that need input support, the widget can be subclassed to provide
  keyboard input, and advanced features like pseudo ttys, termio, serial port I/O, etc.,
  as such features are beyond the scope of FLTK.

  \subsection Fl_Terminal_Examples Examples

  \par
  \code
    //
    // Fl_Terminal: Simple Use
    //
    Fl_Terminal *tty = new Fl_Terminal(...);
    :
    tty->append("Hello world.\n");                      // simple strings
    tty->append("\033[31mThis text is red.\033[0m\n");  // colored text
    tty->append("\033[32mThis text is green.\033[0m\n");
    tty->printf("The value of x is %.02f\n", x);        // printf() formatting
  \endcode
  \par
  There are also public methods for doing what most "\033[" escape codes do,
  so that if ansi(bool) is set to "false", one can still change text colors
  or clear the screen via application control, e.g.
  \par
  \code
     tty->home();                  // home the cursor
     tty->clear_screen();          // clear the screen
     tty->textfgcolor(0xff000000); // change the text color to RED
     tty->textbgcolor(0x0000ff00); // change the background color to BLUE
     //                 ├┘├┘├┘
     //                 R G B
  \endcode
  \par
  When creating the widget, the width/height determine the default column
  and row count for the terminal's display based on the current font size.
  The column width determines where text will wrap.
  \par
  You can specify wider column sizes than the screen using
  display_columns(colwidth). When this value is larger than
  the widget's width, text will wrap off-screen, and can be revealed by
  resizing the widget wider.

  \subsection Fl_Terminal_Writing Writing To Terminal From Applications

  \par
  An application needing terminal output as part of its user interface
  can instance Fl_Terminal, and write text strings with:
  \par
     - append() to append strings
     - printf() to append formatted strings
  \par
  Single character output can be done with:
  \par
     - print_char() to print a single ASCII/UTF-8 char at the cursor
     - plot_char() to put single ASCII/UTF-8 char at an x,y position
  \par

  \subsection Fl_Terminal_Attributes Text Attributes
  \par
  The terminal's text supports these attributes:
  \par
  - Italic - italicized text: <TT>\\033[3m</TT>
  - Bold - brighter/thicker text: <TT>\\033[1m</TT>
  - Dim - lower brightness text: <TT>\\033[2m</TT>
  - Underline - text that is underlined: <TT>\\033[4m</TT>
  - Strikeout - text that has a line through the text: <TT>\\033[9m</TT>
  - Inverse - text whose background and foreground colors are swapped: <TT>\\033[7m</TT>
  - Normal - normal text: <TT>\\033[0m</TT>
  \par
  \image html Fl_Terminal-utf8-demo.png "Fl_Terminal screen"
  \image latex Fl_Terminal-utf8-demo.png "Fl_Terminal screen" width=6cm

  \subsection Fl_Terminal_Colors Text and Background Colors

  \par
  There's at least two ways to specify colors for text and background colors:
  \par
  - 3 bit / 8 Color Values
  - Full 24 bit R/G/B colors
  \par
  Example of 3 bit colors:
  \image html  Fl_Terminal-3bit-colors.png "Fl_Terminal 3 bit colors"
  \image latex Fl_Terminal-3bit-colors.png "Fl_Terminal 3 bit colors" width=6cm
  \par
  Example application source code using 3 bit colors:
  \code
    //
    // Text colors
    //
    tty->append("\033[31m Red text.\033[0m\n");   // Print red text..
    tty->append("\033[32m Green text.\033[0m\n");
    :
    tty->append("\033[36m Cyan text.\033[0m\n");
    tty->append("\033[37m White text.\033[0m\n");
    //
    // Background colors
    //
    tty->append("\033[41m Red Background.\033[0m\n");  // background will be red
    tty->append("\033[42m Green Background.\033[0m\n");
    :
    tty->append("\033[46m Cyan Background.\033[0m\n");
    tty->append("\033[47m White Background.\033[0m\n");
  \endcode
  \par
  Example of 24 bit colors:
  \image html  Fl_Terminal-24bit-colors.png "Fl_Terminal 24 bit colors"
  \image latex Fl_Terminal-24bit-colors.png "Fl_Terminal 24 bit colors" width=6cm
  \par
  Example application source code using 24 bit colors:
  \code
    //
    // 24 bit Text Color
    //
    tty->append("\033[38;2;0;0;255m Text is BLUE.\033[0m\n");                // RGB: R=0,   G=0,  B=255
    tty->append("\033[38;2;255;0;0m Text is RED.\033[0m\n");                 // RGB: R=255, G=0,  B=0
    tty->append("\033[38;2;127;64;0m Text is DARK ORANGE.\033[0m\n");        // RGB: R=127, G=64, B=0
    //
    // 24 bit Background Color
    //
    tty->append("\033[48;2;0;0;255m Background is BLUE.\033[0m\n");          // RGB: R=0,   G=0,  B=255
    tty->append("\033[48;2;255;0;0m Background is RED.\033[0m\n");           // RGB: R=255, G=0,  B=0
    tty->append("\033[48;2;127;64;0m Background is DARK ORANGE.\033[0m\n");  // RGB: R=127, G=64, B=0
  \endcode
  \par
  For more on the ANSI escape codes, see \ref Fl_Terminal_escape_codes.

  \subsection Fl_Terminal_Features Features

  \par
  Most standard terminal behaviors are supported, e.g.
  \par
     - ASCII + UTF-8/Unicode
     - scrollback history management
     - mouse selection + copy/paste (^C, ^A)
     - autoscroll during selection
  \par
  Most popular ANSI/DEC VT100/Xterm escape sequences are supported (see \ref Fl_Terminal_escape_codes), including:
     - per-character colors for text and background
     - per-character text attributes: bold/dim, underline, strikeout
     - scrolling up/down
     - character insert/delete for characters/rows/screen
     - clearing characters/rows/screen
  \par
  Does not (yet) support:
     - programmable regions (scroll regions and attribute blocks)
     - dynamic line wrap (where resizing display dynamically re-wraps long lines)
  \par
  Will likely never implement as part of this widget:
     - pty/termio management (such features should be _subclassed_)
     - Different per-character font family + sizes (font family/size is global only)
     - variable width fonts
  \par
  Regarding the font family+size; the way the terminal is currently designed,
  the font family and size must not vary within text; rows have to be consistent
  height. Varying widths are tricky too, esp. when it comes to moving the cursor
  up/down within a column; varying *widths* are supported (due to Unicode characters
  sometimes being "wide", but not heights.

  \subsection Fl_Terminal_Margins Margins

  \par
  The margins define the amount of space (in pixels) around the outside of the
  text display area, the space between the widget's inner edge (inside the box())
  and the text display area's outer edge. The margins can be inspected and changed
  with the margin_left(), margin_right(), margin_top() and margin_bottom() methods.
  \par
  \code
·
     TERMINAL WIDGET (Fl_Terminal)
     ┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┳━━━┓
     ┃                          ↑ margin_top                                ┃   ┃
     ┃     TEXT DISPLAY AREA    ↓                                           ┃   ┃
     ┃     ┌┬┬┬┬┬┬┬┬┬┬┬┬┬┬┬┬┬┬┬┬┬┬┬┬┬┬┬┬─▴─┬┬┬┬┬┬┬┬┬┬┬┬┬┬┬┬┬┬┬┬┬┬┬┬┬┬┬┐     ┃   ┃
     ┃     ├┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼ │ ┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┤     ┃ S ┃
     ┃     ├┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼ │ ┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┤     ┃ c ┃
     ┃     ├┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼ display_rows ┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┤     ┃ r ┃
     ┃     ├┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼ │ ┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┤     ┃ o ┃
     ┃     ├┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼ │ ┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┤     ┃ l ┃
     ┃     ├┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼ │ ┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┤     ┃ l ┃
     ┃     │◄── display_columns ─────────┼───────────────────────────►│     ┃   ┃
     ┃     ├┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼ │ ┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┤     ┃ B ┃
     ┃     ├┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼ │ ┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┤     ┃ a ┃
     ┃     ├┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼ │ ┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┤     ┃ r ┃
     ┃     ├┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼ │ ┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┤     ┃   ┃
     ┃     ├┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼ │ ┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┤     ┃   ┃
     ┃◄─┬─►├┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼ │ ┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┤◄─┬─►┃   ┃
     ┃  │  └┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴─▾─┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┘  │  ┃   ┃
     ┃ margin_left              ↑                              margin_right ┃   ┃
     ┃                          ↓ margin_bottom                             ┃   ┃
     ┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┻━━━┛
                                Fl_Terminal Margins


                             ░░░░░░░░░░░░░░░░░░░░░░░░░
                             ░░░░░░░░░░░░░░░░░░░░░░░░░
                             ░░░░░░:::::::::::::░░░░░◉
                             ░░░░░░::##:::::##:◉░░░░░░╲
                             ░░░░░░::###:::###::╲░░░░░ ╲
                             ░░░░░░::####:####::░╲░░░░  Fl_Terminal::color()
                             ░░░░░░::##:###:##::░░╲░░░
                             ░░░░░░::##::#::##::░░░╲░░
                             ░░░░░░::##:.:::##::░░░░╲░
                             ░░░░░░::##:::::#◉::░░░░░╲
                             ░░░░░░:::::::::::╲:░░░░░░╲
                             ░░░░░░░░░░░░░░░░░░╲░░░░░░ Fl_Terminal::textbgcolor()
                             ░░░░░░░░░░░░░░░░░░░╲░░░░░
                                                 ╲
                                                  Fl_Terminal::textfgcolor()
·
  \endcode

  \subsection Fl_Terminal_Caveats Caveats

  \par
  - This widget is not a full terminal emulator; it does not do stdio redirection,
    pseudo ttys/termios/character cooking, keyboard input processing, full
    curses(3) support. However, such features CAN be implemented with subclassing.
    \par
  - The printf() and vprintf() functions are limited to strings no longer than
    1024 characters (including NULL). For printing longer strings, use append()
    which has no string limits.
    \par
  - For background colors textbgcolor() and textbgcolor_default(), the special
    color value 0xffffffff lets the widget's box() color() show through behind
    the text.
**/

class FL_EXPORT Fl_Terminal : public Fl_Group {
  //////////////////////////////////////
  ////// Fl_Terminal Public Enums //////
  //////////////////////////////////////
public:
  /**
    \enum RedrawStyle
    Determines when Fl_Terminal calls redraw() if new text is added.
    RATE_LIMITED is the recommended setting, using redraw_rate(float) to determine
    the maximum rate of redraws.
    \see redraw_style(), redraw_rate()
  */
  enum RedrawStyle {
    NO_REDRAW=0,        ///< app must call redraw() as needed to update text to screen
    RATE_LIMITED,       ///< timer controlled redraws. (DEFAULT)
    PER_WRITE           ///< redraw triggered after *every* append() / printf() / etc. operation
  };

  /**
    \enum Attrib
    Bits for the per-character attributes, which control text features
    such as italic, bold, underlined text, etc.

    NOTE: enum names with a leading underbar must not be used, and are
    reserved for future use.
  */
  enum Attrib {
    NORMAL      = 0x00,   ///< all attributes off
    BOLD        = 0x01,   ///< bold text: uses bold font, color brighter than normal
    DIM         = 0x02,   ///< dim text; color slightly darker than normal
    ITALIC      = 0x04,   ///< italic font text
    UNDERLINE   = 0x08,   ///< underlined text
    _RESERVED_1 = 0x10,   ///< <EM>(reserved for internal future use)</EM>
    INVERSE     = 0x20,   ///< inverse text; fg/bg color are swapped
    _RESERVED_2 = 0x40,   ///< <EM>(reserved for internal future use)</EM>
    STRIKEOUT   = 0x80    ///< strikeout text
  };

  /**
    \enum CharFlags
    Per-character 8 bit flags (uchar) used to manage special states for characters.
  */
  enum CharFlags {
    FG_XTERM   = 0x01,    ///< this char's fg color is an XTERM color; can be affected by Dim+Bold
    BG_XTERM   = 0x02,    ///< this char's bg color is an XTERM color; can be affected by Dim+Bold
    EOL        = 0x04,    ///< TODO: char at EOL, used for line re-wrap during screen resizing
    RESV_A     = 0x08,
    RESV_B     = 0x10,
    RESV_C     = 0x20,
    RESV_D     = 0x40,
    RESV_E     = 0x80,
    COLORMASK  = (FG_XTERM | BG_XTERM)
  };

  /**
    \enum OutFlags
    Output translation flags for special control character translations.
  */
  enum OutFlags {
    OFF        = 0x00,     ///< no output translation
    CR_TO_LF   = 0x01,     ///< carriage return generates a vertical line-feed (\\r -> \\n)
    LF_TO_CR   = 0x02,     ///< line-feed generates a carriage return (\\n -> \\r)
    LF_TO_CRLF = 0x04      ///< line-feed generates a carriage return line-feed (\\n -> \\r\\n)
  };

  /**
    \enum ScrollbarStyle
    Behavior of scrollbars
  */
  enum ScrollbarStyle {
    SCROLLBAR_OFF  = 0x00, ///< scrollbar always invisible
    SCROLLBAR_AUTO = 0x01, ///< scrollbar visible if widget resized in a way that hides columns (default)
    SCROLLBAR_ON   = 0x02  ///< scrollbar always visible
  };

  ///////////////////////////////////////////////////////////////
  //////
  ////// Fl_Terminal Protected Classes
  //////
  ///////////////////////////////////////////////////////////////
protected:
  // Margin Class ////////////////////////////////////////////
  //
  // Class to manage the terminal's margins
  //
  class FL_EXPORT Margin {
    int left_, right_, top_, bottom_;
  public:
    Margin(void) { left_ = right_ = top_ = bottom_ = 3; }
    int left(void)   const { return left_;   }
    int right(void)  const { return right_;  }
    int top(void)    const { return top_;    }
    int bottom(void) const { return bottom_; }
    void left(int val)     { left_   = val;  }
    void right(int val)    { right_  = val;  }
    void top(int val)      { top_    = val;  }
    void bottom(int val)   { bottom_ = val;  }
  };

  // CharStyle Class ////////////////////////////////////////////
  //
  // Class to manage the terminal's character style
  // This includes the font, color, and some cached internal
  // info for optimized drawing speed.
  //
  class FL_EXPORT CharStyle {
    uchar       attrib_;           // bold, underline..
    uchar       charflags_;        // CharFlags (xterm color management)
    Fl_Color    fgcolor_;          // foreground color for text
    Fl_Color    bgcolor_;          // background color for text
    Fl_Color    defaultfgcolor_;   // default fg color used by ESC[0m
    Fl_Color    defaultbgcolor_;   // default bg color used by ESC[0m
    Fl_Font     fontface_;         // font face
    Fl_Fontsize fontsize_;         // font size
    int         fontheight_;       // font height (in pixels)
    int         fontdescent_;      // font descent (pixels below font baseline)
    int         charwidth_;        // width of a fixed width ASCII character
  public:
    CharStyle(bool fontsize_defer);
    uchar attrib(void) const            { return attrib_; }
    uchar charflags(void) const         { return charflags_; }
    // Colors - All access to colors are by Fl_Color only.
    //          There are three ways to SET colors: Fl_Color, rgb, xterm(uchar)
    //
    Fl_Color fltk_fg_color(uchar ci);
    Fl_Color fltk_bg_color(uchar ci);
    Fl_Color fgcolor(void) const;
    Fl_Color bgcolor(void) const;
    Fl_Color defaultfgcolor(void) const { return defaultfgcolor_; }
    Fl_Color defaultbgcolor(void) const { return defaultbgcolor_; }
    Fl_Font fontface(void) const        { return fontface_; }
    Fl_Fontsize fontsize(void) const    { return fontsize_; }
    int  fontheight(void) const         { return fontheight_; }
    int  fontdescent(void) const        { return fontdescent_; }
    int  charwidth(void) const          { return charwidth_; }
    uchar colorbits_only(uchar inflags) const;
    void attrib(uchar val)              { attrib_    = val; }
    void charflags(uchar val)           { charflags_ = val; }
    void set_charflag(uchar val)        { charflags_ |= val; }
    void clr_charflag(uchar val)        { charflags_ &= ~val; }
    // Non-xterm colors
    void fgcolor(int r,int g,int b)     { fgcolor_ = (r<<24) | (g<<16) | (b<<8); clr_charflag(FG_XTERM); }
    void bgcolor(int r,int g,int b)     { bgcolor_ = (r<<24) | (g<<16) | (b<<8); clr_charflag(BG_XTERM); }
    void fgcolor(Fl_Color val)          { fgcolor_ = val;                        clr_charflag(FG_XTERM); }
    void bgcolor(Fl_Color val)          { bgcolor_ = val;                        clr_charflag(BG_XTERM); }
    // Xterm colors
    void fgcolor_xterm(Fl_Color val)    { fgcolor_ = val;                        set_charflag(FG_XTERM); }
    void bgcolor_xterm(Fl_Color val)    { bgcolor_ = val;                        set_charflag(BG_XTERM); }
    void fgcolor_xterm(uchar val);
    void bgcolor_xterm(uchar val);
    //
    void defaultfgcolor(Fl_Color val)   { defaultfgcolor_ = val; }
    void defaultbgcolor(Fl_Color val)   { defaultbgcolor_ = val; }
    void fontface(Fl_Font val)          { fontface_ = val; update(); }
    void fontsize(Fl_Fontsize val)      { fontsize_ = val; update(); }
    void update(void);
    void update_fake(void);
    // SGR MODES: Set Graphics Rendition
    void sgr_reset(void) {                         // e.g. ESC[0m
      attrib(Fl_Terminal::NORMAL);
      if (charflags() & FG_XTERM) fgcolor_xterm(defaultfgcolor_);
      else                        fgcolor(defaultfgcolor_);
      if (charflags() & BG_XTERM) bgcolor_xterm(defaultbgcolor_);
      else                        bgcolor(defaultbgcolor_);
    }
    int  onoff(bool flag, Attrib a) { return (flag ? (attrib_ | a) : (attrib_ & ~a)); }
    void sgr_bold(bool val)      { attrib_ = onoff(val, Fl_Terminal::BOLD);      } // e.g. ESC[1m
    void sgr_dim(bool val)       { attrib_ = onoff(val, Fl_Terminal::DIM);       } // e.g. ESC[2m
    void sgr_italic(bool val)    { attrib_ = onoff(val, Fl_Terminal::ITALIC);    } // e.g. ESC[3m
    void sgr_underline(bool val) { attrib_ = onoff(val, Fl_Terminal::UNDERLINE); } // e.g. ESC[3m
    void sgr_dbl_under(bool val) { attrib_ = onoff(val, Fl_Terminal::UNDERLINE); } // e.g. ESC[21m (TODO!)
    void sgr_blink(bool val)     { (void)val; /* NOT IMPLEMENTED */              } // e.g. ESC[5m
    void sgr_inverse(bool val)   { attrib_ = onoff(val, Fl_Terminal::INVERSE);   } // e.g. ESC[7m
    void sgr_strike(bool val)    { attrib_ = onoff(val, Fl_Terminal::STRIKEOUT); } // e.g. ESC[9m
  };

protected:
  // Cursor Class ///////////////////////////////////////////////////////////
  //
  // Class to manage the terminal's cursor position, color, etc.
  //
  class FL_EXPORT Cursor {
    int      col_;      // cursor's current col (x) position on display
    int      row_;      // cursor's current row (y) position on display
    int      h_;        // cursor's height (affected by font size)
    Fl_Color fgcolor_;  // cursor's fg color (color of text, if any)
    Fl_Color bgcolor_;  // cursor's bg color
  public:
    Cursor(void) {
      col_     = 0;
      row_     = 0;
      h_       = 10;
      fgcolor_ = 0xfffff000;   // wht
      bgcolor_ = 0x00d00000;   // grn
    }
    int col(void) const          { return col_; }
    int row(void) const          { return row_; }
    int h(void) const            { return h_; }
    Fl_Color fgcolor(void) const { return fgcolor_; }
    Fl_Color bgcolor(void) const { return bgcolor_; }
    void col(int val)            { col_ = val >= 0 ? val : 0; }
    void row(int val)            { row_ = val >= 0 ? val : 0; }
    void h(int val)              { h_   = val; }
    void fgcolor(Fl_Color val)   { fgcolor_ = val; }
    void bgcolor(Fl_Color val)   { bgcolor_ = val; }
    int  left(void)              { col_ = (col_>0) ? (col_-1) : 0; return col_; }
    int  right(void)             { return ++col_; }
    int  up(void)                { row_ = (row_>0) ? (row_-1) : 0; return row_; }
    int  down(void)              { return ++row_; }
    bool is_rowcol(int drow,int dcol) const;
    void scroll(int nrows);
    void home(void) { row_ = 0; col_ = 0; }
  };

  // Utf8Char Class ///////////////////////////////////////////////////////////
  //
  //    Class to manage the terminal's individual UTF-8 characters.
  //    Includes fg/bg color, attributes (BOLD, UNDERLINE..)
  //
  class FL_EXPORT Utf8Char {
    static const int max_utf8_ = 4; // RFC 3629 paraphrased: In UTF-8, chars are encoded with 1 to 4 octets
    char     text_[max_utf8_];      // memory for actual ASCII or UTF-8 byte contents
    uchar    len_;                  // length of bytes in text_[] buffer; 1 for ASCII, >1 for UTF-8
    uchar    attrib_;               // attribute bits for this char (bold, underline..)
    uchar    charflags_;            // CharFlags (xterm colors management)
    Fl_Color fgcolor_;              // fltk fg color (supports 8color or 24bit color set w/ESC[37;<r>;<g>;<b>m)
    Fl_Color bgcolor_;              // fltk bg color (supports 8color or 24bit color set w/ESC[47;<r>;<g>;<b>m)
    // Private methods
    void text_utf8_(const char *text, int len);
    Fl_Color attr_color_(Fl_Color col, const Fl_Widget *grp) const;
  public:
    // Public methods
    Utf8Char(void);                             // ctor
    Utf8Char(const Utf8Char& o);                // copy ctor
    ~Utf8Char(void);                            // dtor
    Utf8Char& operator=(const Utf8Char& o);     // assignment
    inline int max_utf8() const { return max_utf8_; }
    void text_utf8(const char *text, int len, const CharStyle& style);
    void text_ascii(char c, const CharStyle& style);
    void fl_font_set(const CharStyle& style) const;

    // Return the UTF-8 text string for this character.
    //   Use length() to get number of bytes in string, which will be 1 for ASCII chars.
    //
    const char* text_utf8(void) const { return text_; }
    // Return the attribute for this char
    uchar attrib(void)    const { return attrib_; }
    uchar charflags(void) const { return charflags_; }
    Fl_Color fgcolor(void) const;
    Fl_Color bgcolor(void) const;
    // Return the length of this character in bytes (UTF-8 can be multibyte..)
    int length(void) const { return int(len_); }
    double pwidth(void) const;
    int pwidth_int(void) const;
    // Clear the character to a 'space'
    void clear(const CharStyle& style) { text_utf8(" ", 1, style); charflags_ = 0; attrib_ = 0; }
    bool is_char(char c) const { return *text_ == c; }
    void show_char(void) const { ::printf("%.*s", len_, text_); }
    void show_char_info(void) const { ::fprintf(stderr, "UTF-8('%.*s', len=%d)\n", len_, text_, len_); }
    Fl_Color attr_fg_color(const Fl_Widget *grp) const;
    Fl_Color attr_bg_color(const Fl_Widget *grp) const;
  };

  // RingBuffer Class ///////////////////////////////////////////////////
  //
  // Manages ring with indexed row/col and "history" vs. "display" concepts.
  //
  class FL_EXPORT RingBuffer {
    Utf8Char *ring_chars_;    // the ring UTF-8 char buffer
    int ring_rows_;           // #rows in ring total
    int ring_cols_;           // #columns in ring/hist/disp
    int nchars_;              // #chars in ring (ring_rows*ring_cols)
    int hist_rows_;           // #rows in history
    int hist_use_;            // #rows in use by history
    int disp_rows_;           // #rows in display
    int offset_;              // index offset (used for 'scrolling')

private:
    void new_copy(int drows, int dcols, int hrows, const CharStyle& style);
    //DEBUG    void write_row(FILE *fp, Utf8Char *u8c, int cols) const {
    //DEBUG      cols = (cols != 0) ? cols : ring_cols();
    //DEBUG      for ( int col=0; col<cols; col++, u8c++ ) {
    //DEBUG        ::fprintf(fp, "%.*s", u8c->length(), u8c->text_utf8());
    //DEBUG      }
    //DEBUG    }
  public:
    void clear(void);
    void clear_hist(void);
    RingBuffer(void);
    RingBuffer(int drows, int dcols, int hrows);
    ~RingBuffer(void);

    // Methods to access ring
    //
    //    The 'offset' concept allows the 'history' and 'display'
    //    to be scrolled indefinitely. The 'offset' is applied
    //    to all the row accesses, and are clamped to within their bounds.
    //
    //    For 'raw' access to the ring (without the offset concept),
    //    use the ring_chars() method, and walk from 0 - ring_rows().
    //
    //          _____________
    //         |             | <- hist_srow()  <- ring_srow()
    //         |   H i s t   |
    //         |             |
    //         |_____________| <- hist_erow()
    //         |             | <- disp_srow()
    //         |   D i s p   |
    //         |             |
    //         |_____________| <- disp_erow()  <- ring_erow()
    //
    //          \___________/
    //           ring_cols()
    //           hist_cols()
    //           disp_cols()
    //
    inline int ring_rows(void) const { return ring_rows_; }
    inline int ring_cols(void) const { return ring_cols_; }
    inline int ring_srow(void) const { return(0); }
    inline int ring_erow(void) const { return(ring_rows_ - 1); }
    inline int hist_rows(void) const { return hist_rows_; }
    inline int hist_cols(void) const { return ring_cols_; }
    inline int hist_srow(void) const { return((offset_ + 0) % ring_rows_); }
    inline int hist_erow(void) const { return((offset_ + hist_rows_ - 1) % ring_rows_); }
    inline int disp_rows(void) const { return disp_rows_; }
    inline int disp_cols(void) const { return ring_cols_; }
    inline int disp_srow(void) const { return((offset_ + hist_rows_) % ring_rows_); }
    inline int disp_erow(void) const { return((offset_ + hist_rows_ + disp_rows_ - 1) % ring_rows_); }
    inline int offset(void)    const { return offset_; }
    void offset_adjust(int rows);
    void hist_rows(int val) { hist_rows_ = val; }
    void disp_rows(int val) { disp_rows_ = val; }

    // History use
    inline int  hist_use(void)  const       { return hist_use_; }
    inline void hist_use(int val)           { hist_use_ = val; }
    inline int  hist_use_srow(void) const   { return((offset_ + hist_rows_ - hist_use_) % ring_rows_); }
    inline Utf8Char *ring_chars(void)       { return ring_chars_; }  // access ring buffer directly
    inline Utf8Char *ring_chars(void) const { return ring_chars_; }  // access ring buffer directly

    bool is_hist_ring_row(int grow) const;
    bool is_disp_ring_row(int grow) const;
    //DEBUG void show_ring_info(void) const;
    void move_disp_row(int src_row, int dst_row);
    void clear_disp_rows(int sdrow, int edrow, const CharStyle& style);
    void scroll(int rows, const CharStyle& style);

    const Utf8Char* u8c_ring_row(int row) const;
    const Utf8Char* u8c_hist_row(int hrow) const;
    const Utf8Char* u8c_hist_use_row(int hurow) const;
    const Utf8Char* u8c_disp_row(int drow) const;
    // Non-const versions of the above methods
    //   Using "Effective C++" ugly-as-hell syntax technique.
    //
    Utf8Char* u8c_ring_row(int row);
    Utf8Char* u8c_hist_row(int hrow);
    Utf8Char* u8c_hist_use_row(int hurow);
    Utf8Char* u8c_disp_row(int drow);

    void create(int drows, int dcols, int hrows);
    void resize(int drows, int dcols, int hrows, const CharStyle& style);

    void change_disp_rows(int drows, const CharStyle& style);
    void change_disp_cols(int dcols, const CharStyle& style);
  };

  // Selection Class ///////////////////////////////////////////////////
  //
  // Class to manage mouse selection
  //
  class FL_EXPORT Selection {
    Fl_Terminal *terminal_;
    int srow_, scol_, erow_, ecol_;     // selection start/end. NOTE: start *might* be > end
    int push_row_, push_col_;           // global row/col for last FL_PUSH
    bool push_char_right_;
    Fl_Color selectionbgcolor_;
    Fl_Color selectionfgcolor_;
    int state_ ;                        // 0=none, 1=started, 2=extended, 3=done
    bool is_selection_;                 // false: no selection
  public:
    Selection(Fl_Terminal *terminal);
    int srow(void) const { return srow_; }
    int scol(void) const { return scol_; }
    int erow(void) const { return erow_; }
    int ecol(void) const { return ecol_; }
    void push_clear() { push_row_ = push_col_ = -1; push_char_right_ = false; }
    void push_rowcol(int row,int col,bool char_right) {
      push_row_ = row; push_col_ = col; push_char_right_ = char_right; }
    void start_push() { start(push_row_, push_col_, push_char_right_); }
    bool dragged_off(int row,int col,bool char_right) {
      return (push_row_ != row) || (push_col_+push_char_right_ != col+char_right); }
    void selectionfgcolor(Fl_Color val) { selectionfgcolor_ = val; }
    void selectionbgcolor(Fl_Color val) { selectionbgcolor_ = val; }
    Fl_Color selectionfgcolor(void) const { return selectionfgcolor_; }
    Fl_Color selectionbgcolor(void) const { return selectionbgcolor_; }
    bool is_selection(void) const { return is_selection_; }
    bool get_selection(int &srow,int &scol,int &erow,int &ecol) const; // guarantees return (start < end)
    bool start(int row, int col, bool char_right);
    bool extend(int row, int col, bool char_right);
    void end(void);
    void select(int srow, int scol, int erow, int ecol);
    bool clear(void);
    int  state(void) const { return state_; }
    void scroll(int nrows);
  };

  // EscapeSeq Class ///////////////////////////////////////////////////
  //
  // Class to handle parsing ESC sequences
  //
  // Holds all state information for parsing esc sequences,
  // so sequences can span multiple block read(2) operations, etc.
  // Handling of parsed sequences is NOT handled in this class,
  // just the parsing of the sequences and managing generic integers.
  //
  class FL_EXPORT EscapeSeq {
  public:
    // EscapeSeq Constants
    // Maximums
    static const int maxbuff   = 80;  // character buffer
    static const int maxvals   = 20;  // integer value buffer
    // Return codes
    static const int success   = 0;   // operation succeeded
    static const int fail      = -1;  // operation failed
    static const int completed = 1;   // multi-step operation completed successfully
  private:
    char  esc_mode_;                  // escape parsing mode state
    char  csi_;                       // This is an ESC[.. sequence (Ctrl Seq Introducer)
    char  buff_[maxbuff];             // escape sequence being parsed
    char *buffp_;                     // parsing ptr into buff[]
    char *buffendp_;                  // end of buff[] (ptr to last valid buff char)
    char *valbuffp_;                  // pointer to first char in buff of integer being parsed
    int   vals_[maxvals];             // value array for parsing #'s in ESC[#;#;#..
    int   vali_;                      // parsing index into vals_[], 0 if none
    int   save_row_, save_col_;       // used by ESC[s/u for save/restore

    int append_buff(char c);
    int append_val(void);

  public:
    EscapeSeq(void);
    void reset(void);
    char esc_mode(void) const;
    void esc_mode(char val);
    int  total_vals(void) const;
    int  val(int i) const;
    int  defvalmax(int dval, int max) const;
    bool parse_in_progress(void) const;
    bool is_csi(void) const;
    int  parse(char c);
    void save_cursor(int row, int col);
    void restore_cursor(int &row, int &col);
  };

  // Partial UTF-8 Buffer Class ////////////////////////////////////////////
  //
  // Class to manage buffering partial UTF-8 characters between write calls.
  //
  class FL_EXPORT PartialUtf8Buf {
    char buf_[10];      // buffer partial UTF-8 encoded char
    int  buflen_;       // length of buffered UTF-8 encoded char
    int  clen_;         // final byte length of a UTF-8 char
  public:
    void clear(void) { buflen_ = clen_ = 0; }    // clear the buffer
    PartialUtf8Buf(void) { clear(); }            // Ctor
    // Is byte 'c' in the middle of a UTF-8 encoded byte sequence?
    bool is_continuation(char c) {
      //           Byte 1    Byte 2    Byte 3   ..etc..
      //   ASCII:  0xxxxxxx
      // UTF8(2):  110xxxxx  10xxxxxx
      // UTF8(3):  1110xxxx  10xxxxxx  10xxxxxx
      // UTF8(4):  11110xxx  10xxxxxx  10xxxxxx  10xxxxxx
      // UTF8(5):  111110xx  10xxxxxx  10xxxxxx  10xxxxxx  10xxxxxx
      // UTF8(6):  1111110x  10xxxxxx  10xxxxxx  10xxxxxx  10xxxxxx  10xxxxxx
      //           \______/  \______________________________________________/
      //          Start byte           Continuation bytes
      //                               (c & 0xc0) == 0x80
      return ((c & 0xc0) == 0x80);
    }
    // Access buffer
    const char* buf(void) const { return buf_; }
    // Access buffer length
    int buflen(void)      const { return buflen_; }
    // Append bytes of a partial UTF-8 string to the buffer.
    //
    // Returns:
    //   - true if done OK. Use is_complete() to see if a complete char received.
    //   - false if buffer overrun occurred, class is clear()ed.
    //
    // An appropriate response to 'false' would be to print the
    // "unknown character" and skip all subsequent UTF-8 continuation chars.
    //
    bool append(const char* p, int len) {
      if (len <= 0) return true;                       // ignore silly requests: say we did but dont
      if (buflen_ + len >= (int)sizeof(buf_))          // overrun check
        { clear(); return false; }                     // clear self, return false
      if (!buflen_) clen_ = fl_utf8len(*p);            // first byte? save char len for later
      while (len>0) { buf_[buflen_++] = *p++; len--; } // append byte to buffer
      return true;
    }
    bool is_complete(void) const { return (buflen_ && (buflen_ == clen_)); }
  };

  ///////////////////////////////////////////////////////////////
  //////
  ////// Fl_Terminal members + methods
  //////
  ///////////////////////////////////////////////////////////////
public:
  /**
    Vertical scrollbar. This is public so it can be accessed directly, e.g.

    - \ref Fl_Scrollbar::value(void) "scrollbar->value(void)" returns the row offset
      from the bottom of the display, 0 being the bottom (default).
    - \ref Fl_Scrollbar::value(int) "scrollbar->value(int)" similarly sets the row offset,
      which should be in the range [0 .. Fl_Scrollbar::maximum()].
    - \ref Fl_Scrollbar::step(double) "scrollbar->step(double)" sets the smoothness
      of scrolling, default is 0.25 for 4 steps of motion per column.

    \todo Support scrollbar_left/right() - See Fl_Browser_::scrollbar docs
    \todo Support new ScrollbarStyle
   */
  Fl_Scrollbar  *scrollbar;         // vertical scrollbar (value: rows above disp_chars[])
  /**
    Horizontal scrollbar. This is public so it can be accessed directly, e.g.

    - \ref Fl_Scrollbar::value(void) "hscrollbar->value(void)" returns the column offset
      position from the left edge of the display; 0 being the left edge (default).
    - \ref Fl_Scrollbar::value(int) "hscrollbar->value(int)" similarly sets the column offset,
      which should be in the range [0 .. Fl_Scrollbar::maximum()].
    - \ref Fl_Scrollbar::step(double) "hscrollbar->step(double)" sets the smoothness
      of scrolling, default is 0.25 for 4 steps of motion per column.
   */
  Fl_Scrollbar  *hscrollbar;        // horizontal scrollbar
private:
  // Special utf8 symbols
  const char    *error_char_;       // utf8 string shown for invalid utf8, bad ANSI, etc
  bool           fontsize_defer_;   // flag defers font calcs until first draw() (issue 837)
  int            scrollbar_size_;   // local preference for scrollbar size
  ScrollbarStyle hscrollbar_style_;
  CharStyle     *current_style_;    // current font, attrib, color..
  OutFlags       oflags_;           // output translation flags (CR_TO_LF, LF_TO_CR, LF_TO_CRLF)

  // A ring buffer is used for the terminal's history (hist) and display (disp) buffer.
  //    See README-Fl_Terminal.txt, section "RING BUFFER DESCRIPTION" for diagrams/info.
  //
  // Ring buffer
  RingBuffer     ring_;             // terminal history/display ring buffer
  Cursor         cursor_;           // terminal cursor (position, color, etc)
  Margin         margin_;           // terminal margins (top,left,bottom,right)
  Selection      select_;           // mouse selection
  EscapeSeq      escseq;            // Escape sequence parsing (ESC[ xterm/vt100)
  bool           show_unknown_;     // if true, show unknown chars as '¿' (default off)
  bool           ansi_;             // if true, parse ansi codes (default on)
  char          *tabstops_;         // array of tab stops (0|1)   \__ TODO: This should probably
  int            tabstops_size_;    // size of tabstops[] array   /         be a class "TabStops".
  Fl_Rect        scrn_;             // terminal screen xywh inside box(), margins, and scrollbar
  int            autoscroll_dir_;   // 0=autoscroll timer off, 3=scrolling up, 4=scrolling down
  int            autoscroll_amt_;   // #pixels above or below edge, used for autoscroll speed
  RedrawStyle    redraw_style_;     // NO_REDRAW, RATE_LIMITED, PER_WRITE
  float          redraw_rate_;      // maximum redraw rate in seconds, default=0.10
  bool           redraw_modified_;  // display modified; used by update_cb() to rate limit redraws
  bool           redraw_timer_;     // if true, redraw timer is running
  PartialUtf8Buf pub_;              // handles Partial Utf8 Buffer (pub)

protected:
  // Ring buffer management
  const Utf8Char* u8c_ring_row(int grow) const;
  const Utf8Char* u8c_hist_row(int hrow) const;
  const Utf8Char* u8c_hist_use_row(int hrow) const;
  const Utf8Char* u8c_disp_row(int drow) const;
  // Non-const versions of the above.
  //     "Effective C++" says: implement non-const method to cast away const
  //
  Utf8Char* u8c_ring_row(int grow);
  Utf8Char* u8c_hist_row(int hrow);
  Utf8Char* u8c_hist_use_row(int hurow);
  Utf8Char* u8c_disp_row(int drow);
  Utf8Char* u8c_cursor(void);
private:
  void create_ring(int drows, int dcols, int hrows);
  void init_(int X,int Y,int W,int H,const char*L,int rows,int cols,int hist,bool fontsize_defer);
  // Tabstops
  void init_tabstops(int newsize);
  void default_tabstops(void);
  void clear_all_tabstops(void);
  void set_tabstop(void);
  void clear_tabstop(void);
  // Updates
  void update_screen_xywh(void);
  void update_screen(bool font_changed);
  void set_scrollbar_params(Fl_Scrollbar* scroll, int min, int max);
  void update_scrollbar(void);
  // Resize
  void resize_display_rows(int drows);
  void resize_display_columns(int dcols);
  void refit_disp_to_screen(void);
  // Callbacks
  static void scrollbar_cb(Fl_Widget*, void*);    // scrollbar manipulation
  static void autoscroll_timer_cb(void*);         // mouse drag autoscroll
  void        autoscroll_timer_cb2(void);
  static void redraw_timer_cb(void*);             // redraw rate limiting timer
  void        redraw_timer_cb2(void);

  // Screen management
protected:
  CharStyle& current_style(void) const;
  void current_style(const CharStyle& sty);
private:
  int  x_to_glob_col(int X, int grow, int &gcol, bool &gcr) const;
  int  xy_to_glob_rowcol(int X, int Y, int &grow, int &gcol, bool &gcr) const;
protected:
  int  w_to_col(int W) const;
  int  h_to_row(int H) const;
  // API: Display clear operations
  void clear_sod(void);
  void clear_eod(void);
  void clear_eol(void);
  void clear_sol(void);
  void clear_line(int row);
  void clear_line(void);
  const Utf8Char* walk_selection(const Utf8Char *u8c, int &row, int &col) const;
  bool get_selection(int &srow,int &scol,int &erow,int &ecol) const;
  bool is_selection(void) const;
  bool is_inside_selection(int row,int col) const;
private:
  bool is_hist_ring_row(int grow) const;
  bool is_disp_ring_row(int grow) const;
public:
  int  selection_text_len(void) const;
  const char* selection_text(void) const;
protected:
  void clear_mouse_selection(void);
  bool selection_extend(int X,int Y);
  void select_word(int grow, int gcol);
  void select_line(int grow);
  void scroll(int rows);
  void insert_rows(int count);
  void delete_rows(int count);
  void insert_char_eol(char c, int drow, int dcol, int rep);
  void insert_char(char c, int rep);
  void delete_chars(int drow, int dcol, int rep);
  void delete_chars(int rep);
public:
 // API: Terminal operations
  void clear(void);
  void clear(Fl_Color val);
  void clear_screen(bool scroll_to_hist=true);      // ESC [ 2 J
  void clear_screen_home(bool scroll_to_hist=true); // ESC [ H ESC [ 2 J
  void clear_history(void);                         // ESC [ 3 J
  void reset_terminal(void);                        // ESC c
  void cursor_home(void);                           // ESC [ 0 H
 // API: Cursor
  void cursorfgcolor(Fl_Color val);
  void cursorbgcolor(Fl_Color val);
  Fl_Color cursorfgcolor(void) const;
  Fl_Color cursorbgcolor(void) const;
protected:
  void cursor_row(int row);
  void cursor_col(int col);
public:
  int  cursor_row(void) const;
  int  cursor_col(void) const;
protected:
  void cursor_up(int count=1, bool do_scroll=false);
  void cursor_down(int count=1, bool do_scroll=false);
  void cursor_left(int count=1);
  void cursor_right(int count=1, bool do_scroll=false);
  void cursor_eol(void);
  void cursor_sol(void);
  void cursor_cr(void);
  void cursor_crlf(int count=1);
  void cursor_tab_right(int count=1);
  void cursor_tab_left(int count=1);
  void save_cursor(void);
  void restore_cursor(void);
  // Output translation
public:
  void output_translate(Fl_Terminal::OutFlags val);
  Fl_Terminal::OutFlags output_translate(void) const;
private:
  void handle_lf(void);
  void handle_cr(void);
  void handle_esc(void);
  // Printing
  void handle_ctrl(char c);
  bool is_printable(char c);
  bool is_ctrl(char c);
  void handle_SGR(void);
  void handle_DECRARA(void);
  void handle_escseq(char c);
  // --
  void display_modified(void);
  void display_modified_clear(void);
  void clear_char_at_disp(int drow, int dcol);
  const Utf8Char* utf8_char_at_disp(int drow, int dcol) const;
  const Utf8Char* utf8_char_at_glob(int grow, int gcol) const;
  void repeat_char(char c, int rep);
  void utf8_cache_clear(void);
  void utf8_cache_flush(void);
  // API: Character display output
public:
  void plot_char(const char *text, int len, int drow, int dcol);
  void plot_char(char c, int drow, int dcol);
  void print_char(const char *text, int len=-1);
  void print_char(char c);
  // API: String display output
  void append_utf8(const char *buf, int len=-1);
  void append_ascii(const char *s);
  void append(const char *s, int len=-1);
protected:
  int handle_unknown_char(void);
  int handle_unknown_char(int drow, int dcol);
  // Drawing
  void draw_row_bg(int grow, int X, int Y) const;
  void draw_row(int grow, int Y) const;
  void draw_buff(int Y) const;
private:
  void handle_selection_autoscroll(void);
  int  handle_selection(int e);
public:
  // FLTK: draw(), resize(), handle()
  void draw(void) FL_OVERRIDE;
  void resize(int X,int Y,int W,int H) FL_OVERRIDE;
  int  handle(int e) FL_OVERRIDE;
  const char* text(bool lines_below_cursor=false) const;

protected:
  // Internal short names
  //   Don't make these public, but allow internals and
  //   derived classes to maintain brevity.
  //
  /// Return the number of rows in the ring buffer.
  inline int ring_rows(void) const { return ring_.ring_rows(); }
  /// Return the number of columns in the ring buffer.
  inline int ring_cols(void) const { return ring_.ring_cols(); }
  /// Return the starting row# in the ring buffer. (Always 0)
  inline int ring_srow(void) const { return ring_.ring_srow(); }
  /// Return the ending row# in the ring buffer (Always ring_rows()-1)
  inline int ring_erow(void) const { return ring_.ring_erow(); }
  /// Return the number of rows in the scrollback history.
  inline int hist_rows(void) const { return ring_.hist_rows(); }
  /// Return the number of columns in the scrollback history.
  inline int hist_cols(void) const { return ring_.hist_cols(); }
  /// Return the starting row# of the scrollback history.
  inline int hist_srow(void) const { return ring_.hist_srow(); }
  /// Return the ending row# of the scrollback history.
  inline int hist_erow(void) const { return ring_.hist_erow(); }
  /// Return number of rows in use by the scrollback history.
  inline int hist_use(void)  const { return ring_.hist_use();  }
  /// Return the starting row of the "in use" scrollback history.
  inline int hist_use_srow(void) const { return ring_.hist_use_srow(); }
  /// Return the number of rows in the display area.
  inline int disp_rows(void) const { return ring_.disp_rows(); }
  /// Return the number of columns in the display area.
  inline int disp_cols(void) const { return ring_.disp_cols(); }
  /// Return the starting row# in the display area.
  inline int disp_srow(void) const { return ring_.disp_srow(); }
  /// Return the ending row# in the display area.
  inline int disp_erow(void) const { return ring_.disp_erow(); }
  /// Returns the current offset into the ring buffer.
  inline int offset(void) const    { return ring_.offset(); }

  // TODO: CLEAN UP WHAT'S PUBLIC, AND WHAT SHOULD BE 'PROTECTED' AND 'PRIVATE'
  //       Some of the public stuff should, quite simply, "not be".

  // API: Terminal features
public:
  // API: Scrollbar
  int   scrollbar_size(void) const;
  void  scrollbar_size(int val);
  int   scrollbar_actual_size(void) const;
  void  hscrollbar_style(ScrollbarStyle val);
  ScrollbarStyle hscrollbar_style(void) const;
  // API: History
  int   history_rows(void) const;
  void  history_rows(int val);
  int   history_use(void) const;
  // API: Display
  int   display_rows(void) const;
  void  display_rows(int val);
  int   display_columns(void) const;
  void  display_columns(int val);
  // API: Box
  /// Sets the box type, updates terminal margins et al. Default is FL_DOWN_FRAME.
  ///
  /// FL_XXX_FRAME types are handled in a special way by this widget, and guarantee
  /// the background is a flat field.
  ///
  /// FL_XXX_BOX may draw gradients as inherited by Fl::scheme().
  ///
  void  box(Fl_Boxtype val) { Fl_Group::box(val); update_screen(false); }
  /// Returns the current box type.
  Fl_Boxtype box(void) const { return Fl_Group::box(); }
  // API: Margins
  /// Return the left margin; see \ref Fl_Terminal_Margins.
  int   margin_left(void)   const  { return margin_.left();   }
  /// Return the right margin; see \ref Fl_Terminal_Margins.
  int   margin_right(void)  const  { return margin_.right();  }
  /// Return the top margin; see \ref Fl_Terminal_Margins.
  int   margin_top(void)    const  { return margin_.top();    }
  /// Return the bottom margin; see \ref Fl_Terminal_Margins.
  int   margin_bottom(void) const  { return margin_.bottom(); }
  void  margin_left(int val);
  void  margin_right(int val);
  void  margin_top(int val);
  void  margin_bottom(int val);
  // API: Text font/size/color
  void  textfont(Fl_Font val);
  void  textsize(Fl_Fontsize val);
  void  textcolor(Fl_Color val);
  void  color(Fl_Color val);
  void  textfgcolor(Fl_Color val);
  void  textbgcolor(Fl_Color val);
  void  textfgcolor_default(Fl_Color val);
  void  textbgcolor_default(Fl_Color val);
  /// Return text font used to draw all text in the terminal.
  Fl_Font     textfont(void)      const { return current_style_->fontface(); }
  /// Return text font size used to draw all text in the terminal.
  Fl_Fontsize textsize(void)      const { return current_style_->fontsize(); }
  /// Return base widget Fl_Group's box() color()
  Fl_Color    color(void)         const { return Fl_Group::color(); }
  /// Return textcolor(). This is a convenience method that returns textfgcolor_default()
  Fl_Color    textcolor(void)     const { return textfgcolor_default(); }
  /// Return text's current foreground color.
  Fl_Color    textfgcolor(void)   const { return current_style_->fgcolor(); }
  /// Return text's current background color.
  Fl_Color    textbgcolor(void)   const { return current_style_->bgcolor(); }
  /// Return text's default foreground color. \see textfgcolor()
  Fl_Color    textfgcolor_default(void)   const { return current_style_->defaultfgcolor(); }
  /// Return text's default background color. \see textbgcolor()
  Fl_Color    textbgcolor_default(void)   const { return current_style_->defaultbgcolor(); }
  void        textfgcolor_xterm(uchar val);
  void        textbgcolor_xterm(uchar val);
  /// Set mouse selection foreground color.
  void selectionfgcolor(Fl_Color val) { select_.selectionfgcolor(val); }
  /// Set mouse selection background color.
  void selectionbgcolor(Fl_Color val) { select_.selectionbgcolor(val); }
  /// Get mouse selection foreground color.
  Fl_Color selectionfgcolor(void) const { return select_.selectionfgcolor(); }
  /// Get mouse selection background color.
  Fl_Color selectionbgcolor(void) const { return select_.selectionbgcolor(); }
  // API: Text attrib
  void textattrib(uchar val);
  uchar textattrib() const;
  // API: Redraw style/rate
  RedrawStyle redraw_style(void) const;
  void  redraw_style(RedrawStyle val);
private:
  bool  is_redraw_style(RedrawStyle val) { return redraw_style_ == val; }
public:
  float redraw_rate(void) const;
  void  redraw_rate(float val);
  // API: Show unknown/invalid utf8/ANSI sequences with an error character (¿).
  bool  show_unknown(void) const;
  void  show_unknown(bool val);
  /// Sets the "error character" utf8 string shown for invalid utf8
  /// or bad ANSI sequences if show_unknown() is true. Default: "¿".
  /// \see show_unknown(bool)
  void error_char(const char* val) { error_char_ = val; }
  /// Returns the "error character" utf8 string, which is shown for invalid utf8
  /// or bad ANSI sequences if show_unknown() is true. \see show_unknown(bool)
  const char* error_char(void) const { return error_char_; }
  // API: ANSI sequences
  bool  ansi(void) const;
  void  ansi(bool val);
  // Fl_Simple_Terminal API compatibility
  int   history_lines(void) const;
  void  history_lines(int val);
  // API: printf()
  void  printf(const char *fmt, ...);
  void  vprintf(const char *fmt, va_list ap);
  // Ctor
  Fl_Terminal(int X,int Y,int W,int H,const char*L=0);
  Fl_Terminal(int X,int Y,int W,int H,const char*L,int rows,int cols,int hist);
  // Dtor
  ~Fl_Terminal(void);
  // Debugging features
//DEBUG  void show_ring_info() const { ring_.show_ring_info(); }
//DEBUG  void write_row(FILE *fp, Utf8Char *u8c, int cols) const;
//DEBUG  void show_buffers(RingBuffer *a, RingBuffer *b=0) const;
};
#endif
