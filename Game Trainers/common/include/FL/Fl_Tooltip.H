//
// Tooltip header file for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2011 by Bill Spitzak and others.
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
   Fl_Tooltip widget . */

#ifndef Fl_Tooltip_H
#define Fl_Tooltip_H

#include <FL/Fl.H>
#include <FL/Fl_Widget.H>

/**
  The Fl_Tooltip class provides tooltip support for
  all FLTK widgets. It contains only static methods.

  \image html tooltip-options.png "Fl_Tooltip Options"
  \image latex src/tooltip-options.png "Fl_Tooltip Options" width=6cm

*/
class FL_EXPORT Fl_Tooltip {
  friend class Fl_TooltipBox;
public:
  /**    Gets the tooltip delay. The default delay is 1.0 seconds.  */
  static float delay() { return delay_; }
  /**    Sets the tooltip delay. The default delay is 1.0 seconds.  */
  static void delay(float f) { delay_ = f; }
  /**    Gets the time until an open tooltip hides again. The default delay is 12.0 seconds.  */
  static float hidedelay() { return hidedelay_; }
  /**    Sets the time until an open tooltip hides again. The default delay is 12.0 seconds.  */
  static void hidedelay(float f) { hidedelay_ = f; }
  /**
    Gets the tooltip hover delay, the delay between tooltips.
    The default delay is 0.2 seconds.
  */
  static float hoverdelay() { return hoverdelay_; }
  /**
    Sets the tooltip hover delay, the delay between tooltips.
    The default delay is 0.2 seconds.
  */
  static void hoverdelay(float f) { hoverdelay_ = f; }
  /**    Returns non-zero if tooltips are enabled.  */
  static int enabled() { return Fl::option(Fl::OPTION_SHOW_TOOLTIPS); }
  /**    Enables tooltips on all widgets (or disables if <i>b</i> is false).  */
  static void enable(int b = 1) { Fl::option(Fl::OPTION_SHOW_TOOLTIPS, (b!=0));}
  /**    Same as enable(0), disables tooltips on all widgets.  */
  static void disable() { enable(0); }
  static void (*enter)(Fl_Widget* w);
  static void enter_area(Fl_Widget* w, int X, int Y, int W, int H, const char* tip);
  static void (*exit)(Fl_Widget *w);
  /** Gets the current widget target */
  static Fl_Widget* current() {return widget_;}
  static void current(Fl_Widget*);

  /**    Gets the typeface for the tooltip text.  */
  static Fl_Font font() { return font_; }
  /**    Sets the typeface for the tooltip text.  */
  static void font(Fl_Font i) { font_ = i; }
  /**    Gets the size of the tooltip text.  */
  static Fl_Fontsize size() { return (size_ == -1 ? FL_NORMAL_SIZE : size_); }
  /**    Sets the size of the tooltip text.  */
  static void size(Fl_Fontsize s) { size_ = s; }
  /** Gets the background color for tooltips. The default background color is a pale yellow.  */
  static Fl_Color color() { return color_; }
  /** Sets the background color for tooltips. The default background color is a pale yellow.  */
  static void color(Fl_Color c) { color_ = c; }
  /** Gets the color of the text in the tooltip. The default is  black. */
  static Fl_Color textcolor() { return textcolor_; }
  /** Sets the color of the text in the tooltip. The default is  black. */
  static void textcolor(Fl_Color c) { textcolor_ = c; }
  /** Gets the amount of extra space left/right of the tooltip's text. Default is 3. */
  static int margin_width() { return margin_width_; }
  /** Sets the amount of extra space left/right of the tooltip's text. Default is 3. */
  static void margin_width(int v) { margin_width_ = v; }
  /** Gets the amount of extra space above and below the tooltip's text. Default is 3. */
  static int margin_height() { return margin_height_; }
  /** Sets the amount of extra space above and below the tooltip's text. Default is 3. */
  static void margin_height(int v) { margin_height_ = v; }
  /** Gets the maximum width for tooltip's text before it word wraps. Default is 400. */
  static int wrap_width() { return wrap_width_; }
  /** Sets the maximum width for tooltip's text before it word wraps. Default is 400. */
  static void wrap_width(int v) { wrap_width_ = v; }
  /** Returns the window that is used for tooltips */
  static Fl_Window* current_window(void);

  // These should not be public, but Fl_Widget::tooltip() needs them...
  // fabien: made it private with only a friend function access
private:
  friend void Fl_Widget::tooltip(const char *);
  friend void Fl_Widget::copy_tooltip(const char *);
  static void enter_(Fl_Widget* w);
  static void exit_(Fl_Widget *w);
  static void set_enter_exit_once_();

private:
  static float delay_; //!< delay before a tooltip is shown
  static float hidedelay_; //!< delay until tooltip is closed again
  static float hoverdelay_; //!< delay between tooltips
  static Fl_Color color_;
  static Fl_Color textcolor_;
  static Fl_Font font_;
  static Fl_Fontsize size_;
  static Fl_Widget* widget_; //!< Keeps track of the current target widget
  static int margin_width_;     //!< distance around tooltip text left+right
  static int margin_height_;    //!< distance around tooltip text top+bottom
  static int wrap_width_;       //!< maximum width of tooltip text before it word wraps
  static const int draw_symbols_; // 1 = draw @-symbols in tooltips, 0 = no
};

#endif
