//
// Color chooser header file for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2019 by Bill Spitzak and others.
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
   Fl_Color_Chooser widget . */

// The color chooser object and the color chooser popup.  The popup
// is just a window containing a single color chooser and some boxes
// to indicate the current and cancelled color.

#ifndef Fl_Color_Chooser_H
#define Fl_Color_Chooser_H

#include <FL/Fl_Group.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Return_Button.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Value_Input.H>

#ifndef FL_DOXYGEN

/** For internal use only */
class FL_EXPORT Flcc_HueBox : public Fl_Widget {
  int px, py;
protected:
  void draw() FL_OVERRIDE;
  int handle_key(int);
public:
  int handle(int) FL_OVERRIDE;
  Flcc_HueBox(int X, int Y, int W, int H) : Fl_Widget(X,Y,W,H) {
  px = py = 0;}
};

/** For internal use only */
class FL_EXPORT Flcc_ValueBox : public Fl_Widget {
  int py;
protected:
  void draw() FL_OVERRIDE;
  int handle_key(int);
public:
  int handle(int) FL_OVERRIDE;
  Flcc_ValueBox(int X, int Y, int W, int H) : Fl_Widget(X,Y,W,H) {
  py = 0;}
};

/** For internal use only */
class FL_EXPORT Flcc_Value_Input : public Fl_Value_Input {
public:
  int format(char*) FL_OVERRIDE;
  Flcc_Value_Input(int X, int Y, int W, int H) : Fl_Value_Input(X,Y,W,H) {}
};

#endif // !FL_DOXYGEN

/** \addtogroup group_comdlg
    @{ */

/**
  \class Fl_Color_Chooser
  \brief The Fl_Color_Chooser widget provides a standard RGB color chooser.

  \image html fl_color_chooser.jpg
  \image latex fl_color_chooser.jpg "fl_color_chooser()" width=5cm

  You can place any number of the widgets into a panel of your own design.
  The diagram shows the widget as part of a color chooser dialog created by
  the fl_color_chooser() function. The Fl_Color_Chooser widget contains the
  hue box, value slider, and rgb input fields from the above diagram (it
  does not have the color chips or the Cancel or OK buttons).
  The callback is done every time the user changes the rgb value. It is not
  done if they move the hue control in a way that produces the \e same rgb
  value, such as when saturation or value is zero.

  The fl_color_chooser() function pops up a window to let the user pick an
  arbitrary RGB color. They can pick the hue and saturation in the "hue box"
  on the left (hold down CTRL to just change the saturation), and the
  brightness using the vertical slider. Or they can type the 8-bit numbers
  into the RGB Fl_Value_Input fields, or drag the mouse across them to adjust
  them.  The pull-down menu lets the user set the input fields to show RGB,
  HSV, or 8-bit RGB (0 to 255).

  The user can press CTRL-C to copy the currently selected color value as
  text in RGB hex format with leading zeroes to the clipboard, for instance
  \p FL_GREEN would be '00FF00' (since FLTK 1.4.0).

  fl_color_chooser() returns non-zero if the user picks ok, and updates the
  RGB values.  If the user picks cancel or closes the window this returns
  zero and leaves RGB unchanged.

  If you use the color chooser on an 8-bit screen, it will allocate all the
  available colors, leaving you no space to exactly represent the color the
  user picks! You can however use fl_rectf() to fill a region with a simulated
  color using dithering.

  Callback reasons can be \c FL_REASON_DRAGGED, \c FL_REASON_CHANGED, or
  \c FL_REASON_RESELECTED.
 */
/** @} */
class FL_EXPORT Fl_Color_Chooser : public Fl_Group {
  Flcc_HueBox huebox;
  Flcc_ValueBox valuebox;
  Fl_Choice choice;
  Flcc_Value_Input rvalue;
  Flcc_Value_Input gvalue;
  Flcc_Value_Input bvalue;
  Fl_Box resize_box;
  double hue_, saturation_, value_;
  double r_, g_, b_;
  void set_valuators();
  static void rgb_cb(Fl_Widget*, void*);
  static void mode_cb(Fl_Widget*, void*);
public:

  int handle(int e) FL_OVERRIDE;

  /**
   Returns which Fl_Color_Chooser variant is currently active
   \return color modes are rgb(0), byte(1), hex(2), or hsv(3)
   */
  int mode() {return choice.value();}

  /**
   Set which Fl_Color_Chooser variant is currently active
   \param[in] newMode color modes are rgb(0), byte(1), hex(2), or hsv(3)
   */
  void mode(int newMode);

  /**
    Returns the current hue.
    0 <= hue < 6. Zero is red, one is yellow, two is green, etc.
    <em>This value is convenient for the internal calculations - some other
    systems consider hue to run from zero to one, or from 0 to 360.</em>
   */
  double hue() const {return hue_;}

  /**
    Returns the saturation.
    0 <= saturation <= 1.
   */
  double saturation() const {return saturation_;}

  /**
    Returns the value/brightness.
    0 <= value <= 1.
   */
  double value() const {return value_;}

  /**
    Returns the current red value.
    0 <= r <= 1.
   */
  double r() const {return r_;}

  /**
    Returns the current green value.
    0 <= g <= 1.
   */
  double g() const {return g_;}

  /**
    Returns the current blue value.
    0 <= b <= 1.
   */
  double b() const {return b_;}

  int hsv(double H, double S, double V);

  int rgb(double R, double G, double B);

  static void hsv2rgb(double H, double S, double V, double& R, double& G, double& B);

  static void rgb2hsv(double R, double G, double B, double& H, double& S, double& V);

  Fl_Color_Chooser(int X, int Y, int W, int H, const char *L = 0);
};

FL_EXPORT int fl_color_chooser(const char* name, double& r, double& g, double& b, int m=-1);
FL_EXPORT int fl_color_chooser(const char* name, uchar& r, uchar& g, uchar& b, int m=-1);

#endif
