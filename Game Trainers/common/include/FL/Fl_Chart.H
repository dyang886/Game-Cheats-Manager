//
// Fl_Chart widget header file for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2023 by Bill Spitzak and others.
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

/** \file FL/Fl_Chart.H
  \brief Fl_Chart widget.
*/

#ifndef Fl_Chart_H
#define Fl_Chart_H

#ifndef Fl_Widget_H
#include "Fl_Widget.H"
#endif

// values for type()
#define FL_BAR_CHART            0     ///< type() for Bar Chart variant
#define FL_HORBAR_CHART         1     ///< type() for Horizontal Bar Chart variant
#define FL_LINE_CHART           2     ///< type() for Line Chart variant
#define FL_FILL_CHART           3     ///< type() for Fill Line Chart variant
#define FL_SPIKE_CHART          4     ///< type() for Spike Chart variant
#define FL_PIE_CHART            5     ///< type() for Pie Chart variant
#define FL_SPECIALPIE_CHART     6     ///< type() for Special Pie Chart variant

#define FL_FILLED_CHART FL_FILL_CHART ///< for compatibility

#define FL_CHART_MAX          128     ///< max entries per chart
#define FL_CHART_LABEL_MAX     18     ///< max label length for entry

/** For internal use only. */
struct FL_CHART_ENTRY {
  float val;                        ///< For internal use only.
  unsigned col;                     ///< For internal use only.
  char str[FL_CHART_LABEL_MAX + 1]; ///< For internal use only.
};

/**
  \class Fl_Chart
  \brief Fl_Chart displays simple charts.
  It is provided for Forms compatibility.

  \image html charts.png
  \image latex charts.png  "Fl_Chart" width=10cm

  \todo Refactor Fl_Chart::type() information.

  The type of an Fl_Chart object can be set using type(uchar t) to:

  | Chart Type          | Description                                                                                     |
  |---------------------|-------------------------------------------------------------------------------------------------|
  | FL_BAR_CHART        | Each sample value is drawn as a vertical bar.                                                   |
  | FL_FILLED_CHART     | The chart is filled from the bottom of the graph to the sample values.                          |
  | FL_HORBAR_CHART     | Each sample value is drawn as a horizontal bar.                                                 |
  | FL_LINE_CHART       | The chart is drawn as a polyline with vertices at each sample value.                            |
  | FL_PIE_CHART        | A pie chart is drawn with each sample value being drawn as a proportionate slice in the circle. |
  | FL_SPECIALPIE_CHART | Like \c FL_PIE_CHART, but the first slice is separated from the pie.                            |
  | FL_SPIKE_CHART      | Each sample value is drawn as a vertical line.                                                  |
*/
class FL_EXPORT Fl_Chart : public Fl_Widget {
  int numb;
  int maxnumb;
  int sizenumb;
  FL_CHART_ENTRY *entries;
  double min, max;
  uchar autosize_;
  Fl_Font textfont_;
  Fl_Fontsize textsize_;
  Fl_Color textcolor_;

protected:
  void draw() FL_OVERRIDE;

  // (static) protected draw methods (STR 2022)
  // these methods are documented in src/Fl_Chart.cxx

  static void draw_barchart(int x, int y, int w, int h, int numb, FL_CHART_ENTRY entries[],
                            double min, double max, int autosize, int maxnumb, Fl_Color textcolor);

  static void draw_horbarchart(int x, int y, int w, int h, int numb, FL_CHART_ENTRY entries[],
                               double min, double max, int autosize, int maxnumb,
                               Fl_Color textcolor);

  static void draw_linechart(int type, int x, int y, int w, int h, int numb,
                             FL_CHART_ENTRY entries[], double min, double max, int autosize,
                             int maxnumb, Fl_Color textcolor);

  static void draw_piechart(int x, int y, int w, int h, int numb, FL_CHART_ENTRY entries[],
                            int special, Fl_Color textcolor);

public:
  Fl_Chart(int X, int Y, int W, int H, const char *L = 0);

  ~Fl_Chart();

  void clear();

  void add(double val, const char *str = 0, unsigned col = 0);

  void insert(int ind, double val, const char *str = 0, unsigned col = 0);

  void replace(int ind, double val, const char *str = 0, unsigned col = 0);

  /**
    Gets the lower and upper bounds of the chart values.
    \param[out] a, b are set to lower, upper
   */
  void bounds(double *a, double *b) const {
    *a = min;
    *b = max;
  }

  void bounds(double a, double b);

  /**
    Returns the number of data values in the chart.
  */
  int size() const { return numb; }

  /**
    Sets the widget size (width, height).

    This is the same as calling Fl_Widget::size(int W, int H);

    \param[in]  W,H   new width and height of the widget
  */
  void size(int W, int H) { Fl_Widget::size(W, H); }

  /**
    Gets the maximum number of data values for a chart.
   */
  int maxsize() const { return maxnumb; }

  void maxsize(int m);

  /** Gets the chart's text font */
  Fl_Font textfont() const { return textfont_; }

  /** Sets the chart's text font to \p s. */
  void textfont(Fl_Font s) { textfont_ = s; }

  /** Gets the chart's text size */
  Fl_Fontsize textsize() const { return textsize_; }

  /** Sets the chart's text size to \p s. */
  void textsize(Fl_Fontsize s) { textsize_ = s; }

  /** Gets the chart's text color */
  Fl_Color textcolor() const { return textcolor_; }

  /** Sets the chart's text color to \p n. */
  void textcolor(Fl_Color n) { textcolor_ = n; }

  /**
    Gets whether the chart will automatically adjust the bounds of the chart.
    \returns non-zero if auto-sizing is enabled and zero if disabled.
   */
  uchar autosize() const { return autosize_; }

  /**
    Sets whether the chart will automatically adjust the bounds of the chart.
    \param[in] n non-zero to enable automatic resizing, zero to disable.
   */
  void autosize(uchar n) { autosize_ = n; }
};

#endif
