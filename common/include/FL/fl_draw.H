//
// Portable drawing function header file for the Fast Light Tool Kit (FLTK).
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

/**
  \file fl_draw.H
  \brief utility header to pull drawing functions together
*/

#ifndef fl_draw_H
#define fl_draw_H

#include <FL/Enumerations.H>       // color names
#include <FL/Fl_Graphics_Driver.H> // fl_graphics_driver + Fl_Region
#include <FL/Fl_Rect.H>

// Image class...
class Fl_Image;
class Fl_Window;

// Label flags...
FL_EXPORT extern char fl_draw_shortcut;

/** \addtogroup fl_attributes
    @{
*/

// Colors:
/**
  Set the color for all subsequent drawing operations.
  For color-mapped displays, a color cell will be allocated out of
  \p fl_colormap the first time you use a color. If the colormap fills up
  then a least-squares algorithm is used to find the closest color.
  If no valid graphical context (fl_gc) is available,
  the foreground is not set for the current window.
  \param[in] c color
*/
inline void fl_color(Fl_Color c) {
  fl_graphics_driver->color(c);
} // select indexed color
/** for back compatibility - use fl_color(Fl_Color c) instead */
inline void fl_color(int c) {
  fl_color((Fl_Color)c);
}
/**
  Set the color for all subsequent drawing operations.
  The closest possible match to the RGB color is used.
  The RGB color is used directly on TrueColor displays.
  For colormap visuals the nearest index in the gray
  ramp or color cube is used.
  If no valid graphical context (fl_gc) is available,
  the foreground is not set for the current window.
  \param[in] r,g,b color components
*/
inline void fl_color(uchar r, uchar g, uchar b) {
  fl_graphics_driver->color(r, g, b);
}
/**
  Return the last fl_color() that was set.
  This can be used for state save/restore.
*/
inline Fl_Color fl_color() {
  return fl_graphics_driver->color();
}
/** @} */

/** \addtogroup fl_drawings
    @{
*/
// clip:
/**
  Intersect the current clip region with a rectangle and push this
  new region onto the stack.
  \param[in] x,y,w,h position and size
*/
inline void fl_push_clip(int x, int y, int w, int h) {
  fl_graphics_driver->push_clip(x, y, w, h);
}
/**
  Intersect the current clip region with a rectangle and push this
  new region onto the stack (deprecated).
  \param[in] x,y,w,h position and size
  \deprecated
    Please use fl_push_clip(int x, int y, int w, int h) instead.
    fl_clip(int, int, int, int) will be removed in FLTK 1.5.
*/
inline void fl_clip(int x, int y, int w, int h) {
  fl_graphics_driver->push_clip(x, y, w, h);
}
/**
  Push an empty clip region onto the stack so nothing will be clipped.
*/
inline void fl_push_no_clip() {
  fl_graphics_driver->push_no_clip();
}
/**
  Restore the previous clip region.

  You must call fl_pop_clip() once for every time you call fl_push_clip().
  Unpredictable results may occur if the clip stack is not empty when
  you return to FLTK.
*/
inline void fl_pop_clip() {
  fl_graphics_driver->pop_clip();
}

/**
  Does the rectangle intersect the current clip region?

  \param[in] x,y,w,h position and size of rectangle

  \returns non-zero if any of the rectangle intersects the current clip
    region. If this returns 0 you don't have to draw the object.

  \note Under X this returns 2 if the rectangle is partially clipped
    and 1 if it is entirely inside the clip region.

  \see fl_clip_box()
*/
inline int fl_not_clipped(int x, int y, int w, int h) {
  return fl_graphics_driver->not_clipped(x, y, w, h);
}

/**
  Intersect a rectangle with the current clip region and return the
  bounding box of the result.

  Returns non-zero if the resulting rectangle is different to the original.
  The given rectangle <tt>(x, y, w, h)</tt> \e should be entirely inside its
  window, otherwise the result may be unexpected, i.e. this function \e may
  not clip the rectangle to the window coordinates and size. In particular
  \p x and \p y \e should not be negative.

  The resulting bounding box can be used to limit the necessary drawing to
  this rectangle.

  Example:
  \code
    void MyGroup::draw() {
      int X = 0, Y = 0, W = 0, H = 0;
      int ret = fl_clip_box(x(), y(), w(), h(), X, Y, W, H);
      if (ret == 0) { // entire group is visible (not clipped)
        // full drawing code here
      } else { // parts of this group are clipped
        // partial drawing code here (uses X, Y, W, and H to test)
      }
    }
  \endcode

  \p W and \p H are set to zero if the rectangle is completely outside the
    clipping region. In this case \p X and \p Y are undefined and should
    not be used. Possible values are <tt>(0, 0)</tt>, <tt>(x, y)</tt>,
    or anything else (platform dependent).

  \note This function is platform-dependent. If the given rectangle is not
    entirely inside the window, the results are not guaranteed to be the
    same on all platforms.

  \param[in]  x,y,w,h position and size of rectangle
  \param[out] X,Y,W,H position and size of resulting bounding box.

  \returns Non-zero if the resulting rectangle is different to the original.

  \see fl_not_clipped()
*/
inline int fl_clip_box(int x, int y, int w, int h, int &X, int &Y, int &W, int &H) {
  return fl_graphics_driver->clip_box(x, y, w, h, X, Y, W, H);
}

/** Undo any clobbering of the clip region done by your program. */
inline void fl_restore_clip() {
  fl_graphics_driver->restore_clip();
}

/**
  Replace the top of the clipping stack with a clipping region of any shape.

  Fl_Region is an operating system specific type.
  \note This function is mostly intended for internal use by the FLTK library
  when drawing to the display.
  Its effect can be null if the current drawing surface is not the display.
  \param[in] r clipping region
*/
inline void fl_clip_region(Fl_Region r) {
  fl_graphics_driver->clip_region(r);
}

/**
  Return the current clipping region.
  \note This function is mostly intended for internal use by the FLTK library
  when drawing to the display.
  Its return value can be always NULL if the current drawing surface is not the display.
*/
inline Fl_Region fl_clip_region() {
  return fl_graphics_driver->clip_region();
}


// points:
/**
  Draw a single pixel at the given coordinates
*/
inline void fl_point(int x, int y) {
  fl_graphics_driver->point(x, y);
}

// line type:
/**
  Set how to draw lines (the "pen").

  If you change this it is your responsibility to set it back to the default
  using \c fl_line_style(0).

  \image html  fl_line_style.png "fl_line_style() styles"
  \image latex fl_line_style.png "fl_line_style() styles" width=12cm

  \param[in] style A bitmask which is a bitwise-OR of \ref LineStyles "Line Styles",
    a cap style, and a join style. If you don't specify a dash type you
    will get a solid line. If you don't specify a cap or join type
    you will get a system-defined default of whatever value is fastest.
  \param[in] width The thickness of the lines in pixels. Zero results in the
    system defined default, which on both X and Windows is somewhat
    different and nicer than 1.
  \param[in] dashes A pointer to an array of dash lengths, measured in pixels.
    The first location is how long to draw a solid portion, the next
    is how long to draw the gap, then the solid, etc. It is terminated
    with a zero-length entry. A \c NULL pointer or a zero-length
    array results in a solid line. Odd array sizes are not supported
    and result in undefined behavior.

  \note Because of how line styles are implemented on Win32 systems,
    you \e must set the line style \e after setting the drawing
    color. If you set the color after the line style you will lose
    the line style settings.

  \note The \p dashes array does not work under the (unsupported!) operating
    systems Windows 95, 98 or Me, since those operating systems do not
    support complex line styles.
*/
inline void fl_line_style(int style, int width = 0, char *dashes = 0) {
  fl_graphics_driver->line_style(style, width, dashes);
}

/// \anchor LineStyles
///
///  \image html  fl_line_style.png "fl_line_style() styles"
///  \image latex fl_line_style.png "fl_line_style() styles" width=12cm
///
enum {
  FL_SOLID      = 0,      ///< line style: solid line
  FL_DASH       = 1,      ///< line style: 75% dashed line
  FL_DOT        = 2,      ///< line style: 50% pixel dotted
  FL_DASHDOT    = 3,      ///< line style: dash / dot pattern
  FL_DASHDOTDOT = 4,      ///< line style: dash / two dot pattern

  FL_CAP_FLAT   = 0x100,  ///< cap style: end is flat
  FL_CAP_ROUND  = 0x200,  ///< cap style: end is round
  FL_CAP_SQUARE = 0x300,  ///< cap style: end wraps end point

  FL_JOIN_MITER = 0x1000, ///< join style: line join extends to a point
  FL_JOIN_ROUND = 0x2000, ///< join style: line join is rounded
  FL_JOIN_BEVEL = 0x3000  ///< join style: line join is tidied
};

/**
  Turn antialiased line drawings ON or OFF, if supported by platform.
  Currently, only the Windows platform allows to change whether line drawings
  are antialiased. Turning it OFF may accelerate heavy drawing operations.
*/
inline void fl_antialias(int state) {
  fl_graphics_driver->antialias(state);
}

/** Return whether line drawings are currently antialiased. */
inline int fl_antialias() {
  return fl_graphics_driver->antialias();
}

// rectangles tweaked to exactly fill the pixel rectangle:

/**
  Draw a border \e inside the given bounding box.
  This function is meant for quick drawing of simple boxes. The behavior is
  undefined for line widths that are not 1.
*/
inline void fl_rect(int x, int y, int w, int h) {
  fl_graphics_driver->rect(x, y, w, h);
}

/**
 Draw a rounded border \e inside the given bounding box.
 The radius code is optimized for speed and works best for values between
 5 and 15 units.
 */
inline void fl_rounded_rect(int x, int y, int w, int h, int r) {
  fl_graphics_driver->rounded_rect(x, y, w, h, r);
}

/**
  Draw a border \e inside the given bounding box.
  This is the same as fl_rect(int x, int y, int w, int h) but with
  Fl_Rect \p r as input argument.
*/
inline void fl_rect(Fl_Rect r) {
  fl_rect(r.x(), r.y(), r.w(), r.h());
}

/** Draw a dotted rectangle, used to indicate keyboard focus on a widget.

  This method draws the rectangle in the current color and independent of
  the Fl::visible_focus() option. You may need to set the current color
  with fl_color() before you call this.
*/
inline void fl_focus_rect(int x, int y, int w, int h) {
  fl_graphics_driver->focus_rect(x, y, w, h);
}

/** Draw with passed color a border \e inside the given bounding box.
 \warning The current color is changed to \p c upon return.
*/
inline void fl_rect(int x, int y, int w, int h, Fl_Color c) {
  fl_color(c);
  fl_rect(x, y, w, h);
}

/** Color with current color a rectangle that exactly fills the given bounding box. */
inline void fl_rectf(int x, int y, int w, int h) {
  fl_graphics_driver->rectf(x, y, w, h);
}

/** Color with current color a rounded rectangle that exactly fills the given bounding box.
 The radius code is optimized for speed and works best for values between
 5 and 15 units.
*/
inline void fl_rounded_rectf(int x, int y, int w, int h, int r) {
  fl_graphics_driver->rounded_rectf(x, y, w, h, r);
}

/** Color with passed color a rectangle that exactly fills the given bounding box.
 \warning The current color is changed to \p c upon return.
*/
inline void fl_rectf(int x, int y, int w, int h, Fl_Color c) {
  fl_color(c);
  fl_rectf(x, y, w, h);
}

/** Color with current color a rectangle that exactly fills the given bounding box. */
inline void fl_rectf(Fl_Rect r) {
  fl_graphics_driver->rectf(r.x(), r.y(), r.w(), r.h());
}

/** Color with passed color a rectangle that exactly fills the given bounding box.
 \warning The current color is changed to \p c upon return.
*/
inline void fl_rectf(Fl_Rect r, Fl_Color c) {
  fl_color(c);
  fl_rectf(r);
}

/**
  Color a rectangle with "exactly" the passed <tt>r,g,b</tt> color.
  On screens with less than 24 bits of color this is done by drawing a
  solid-colored block using fl_draw_image() so that the correct color
  shade is produced. On other screens, the current color is changed
  to \p fl_color(r,g,b) upon return.
*/
inline void fl_rectf(int x, int y, int w, int h, uchar r, uchar g, uchar b) {
  fl_graphics_driver->colored_rectf(x, y, w, h, r, g, b);
}

/**
  Color a rectangle with "exactly" the passed <tt>r,g,b</tt> color.
  This is the same as fl_rectf(int x, int y, int w, int h, uchar r, uchar g, uchar b)
  but with Fl_Rect \p bb (bounding box) as argument instead of (x, y, w, h).
  \see fl_rectf(int x, int y, int w, int h, uchar r, uchar g, uchar b)
*/
inline void fl_rectf(Fl_Rect bb, uchar r, uchar g, uchar b) {
  fl_graphics_driver->colored_rectf(bb.x(), bb.y(), bb.w(), bb.h(), r, g, b);
}

// line segments:
/**
  Draw a line from (x,y) to (x1,y1)
*/
inline void fl_line(int x, int y, int x1, int y1) {
  fl_graphics_driver->line(x, y, x1, y1);
}
/**
  Draw a line from (x,y) to (x1,y1) and another from (x1,y1) to (x2,y2)
*/
inline void fl_line(int x, int y, int x1, int y1, int x2, int y2) {
  fl_graphics_driver->line(x, y, x1, y1, x2, y2);
}

// closed line segments:
/**
  Outline a 3-sided polygon with lines
*/
inline void fl_loop(int x, int y, int x1, int y1, int x2, int y2) {
  fl_graphics_driver->loop(x, y, x1, y1, x2, y2);
}
/**
  Outline a 4-sided polygon with lines
*/
inline void fl_loop(int x, int y, int x1, int y1, int x2, int y2, int x3, int y3) {
  fl_graphics_driver->loop(x, y, x1, y1, x2, y2, x3, y3);
}

// filled polygons
/**
  Fill a 3-sided polygon.
*/
inline void fl_polygon(int x, int y, int x1, int y1, int x2, int y2) {
  fl_graphics_driver->polygon(x, y, x1, y1, x2, y2);
}
/**
  Fill a 4-sided polygon. The polygon must be convex.
*/
inline void fl_polygon(int x, int y, int x1, int y1, int x2, int y2, int x3, int y3) {
  fl_graphics_driver->polygon(x, y, x1, y1, x2, y2, x3, y3);
}

// draw rectilinear lines, horizontal segment first:
/**
  Draw a horizontal line from (x,y) to (x1,y).
*/
inline void fl_xyline(int x, int y, int x1) {
  fl_graphics_driver->xyline(x, y, x1);
}
/**
  Draw a horizontal line from (x,y) to (x1,y), then vertical from (x1,y) to (x1,y2).
*/
inline void fl_xyline(int x, int y, int x1, int y2) {
  fl_graphics_driver->xyline(x, y, x1, y2);
}
/**
  Draw a horizontal line from (x,y) to (x1,y), then a vertical from (x1,y) to (x1,y2)
  and then another horizontal from (x1,y2) to (x3,y2).
*/
inline void fl_xyline(int x, int y, int x1, int y2, int x3) {
  fl_graphics_driver->xyline(x, y, x1, y2, x3);
}

// draw rectilinear lines, vertical segment first:
/**
  Draw a vertical line from (x,y) to (x,y1)
*/
inline void fl_yxline(int x, int y, int y1) {
  fl_graphics_driver->yxline(x, y, y1);
}
/**
  Draw a vertical line from (x,y) to (x,y1), then a horizontal from (x,y1) to (x2,y1).
*/
inline void fl_yxline(int x, int y, int y1, int x2) {
  fl_graphics_driver->yxline(x, y, y1, x2);
}
/**
  Draw a vertical line from (x,y) to (x,y1), then a horizontal from (x,y1)
  to (x2,y1), then another vertical from (x2,y1) to (x2,y3).
*/
inline void fl_yxline(int x, int y, int y1, int x2, int y3) {
  fl_graphics_driver->yxline(x, y, y1, x2, y3);
}

// circular lines and pie slices (code in fl_arci.C):
/**
  Draw ellipse sections using integer coordinates.

  These functions match the rather limited circle drawing code provided by X
  and Windows. The advantage over using fl_arc with floating point coordinates
  is that they are faster because they often use the hardware, and they draw
  much nicer small circles, since the small sizes are often hard-coded bitmaps.

  If a complete circle is drawn it will fit inside the passed bounding box.
  The two angles are measured in degrees counter-clockwise from 3 o'clock and
  are the starting and ending angle of the arc, \p a2 must be greater or equal
  to \p a1.

  fl_arc() draws a series of lines to approximate the arc. Notice that the
  integer version of fl_arc() has a different number of arguments than the
  double version fl_arc(double x, double y, double r, double start, double end)

  \param[in] x,y,w,h bounding box of complete circle
  \param[in] a1,a2 start and end angles of arc measured in degrees
  counter-clockwise from 3 o'clock. \p a2 must be greater
  than or equal to \p a1.

  \image html  fl_pie_arc_diagram.png "fl_pie() and fl_arc()"
  \image latex fl_pie_arc_diagram.png "fl_pie() and fl_arc()" width=4cm
*/
inline void fl_arc(int x, int y, int w, int h, double a1, double a2) {
  fl_graphics_driver->arc(x, y, w, h, a1, a2);
}
/**
  Draw filled ellipse sections using integer coordinates.

  Like fl_arc(), but fl_pie() draws a filled-in pie slice.
  This slice may extend outside the line drawn by fl_arc();
  to avoid this use w - 1 and h - 1.

  \param[in] x,y,w,h bounding box of complete circle
  \param[in] a1,a2 start and end angles of arc measured in degrees
    counter-clockwise from 3 o'clock. \p a2 must be greater
    than or equal to \p a1.

  \image html  fl_pie_arc_diagram.png "fl_pie() and fl_arc()"
  \image latex fl_pie_arc_diagram.png "fl_pie() and fl_arc()" width=4cm
*/
inline void fl_pie(int x, int y, int w, int h, double a1, double a2) {
  fl_graphics_driver->pie(x, y, w, h, a1, a2);
}
/** fl_chord declaration is a place holder - the function does not yet exist */
FL_EXPORT void fl_chord(int x, int y, int w, int h, double a1, double a2); // nyi

// scalable drawing code (code in fl_vertex.cxx and fl_arc.cxx):
/**
  Save the current transformation matrix on the stack.
  The maximum depth of the stack is 32.
*/
inline void fl_push_matrix() {
  fl_graphics_driver->push_matrix();
}
/**
  Restore the current transformation matrix from the stack.
*/
inline void fl_pop_matrix() {
  fl_graphics_driver->pop_matrix();
}
/**
  Concatenate scaling transformation onto the current one.
  \param[in] x,y scale factors in x-direction and y-direction
*/
inline void fl_scale(double x, double y) {
  fl_graphics_driver->mult_matrix(x, 0, 0, y, 0, 0);
}
/**
  Concatenate scaling transformation onto the current one.
  \param[in] x scale factor in both x-direction and y-direction
*/
inline void fl_scale(double x) {
  fl_graphics_driver->mult_matrix(x, 0, 0, x, 0, 0);
}
/**
  Concatenate translation transformation onto the current one.
  \param[in] x,y translation factor in x-direction and y-direction
*/
inline void fl_translate(double x, double y) {
  fl_graphics_driver->translate(x, y);
}
/**
  Concatenate rotation transformation onto the current one.
  \param[in] d - rotation angle, counter-clockwise in degrees (not radians)
*/
inline void fl_rotate(double d) {
  fl_graphics_driver->rotate(d);
}
/**
  Set the transformation matrix to identity.
 */
inline void fl_load_identity() {
  fl_graphics_driver->load_identity();
}
/**
 Set the current transformation matrix.
 \param[in] a,b,c,d,x,y transformation matrix elements
 */
inline void fl_load_matrix(double a, double b, double c, double d, double x, double y) {
  fl_graphics_driver->load_matrix(a, b, c, d, x, y);
}
/**
  Concatenate another transformation onto the current one.

  \param[in] a,b,c,d,x,y transformation matrix elements such that
 <tt> X' = aX + cY + x </tt> and <tt> Y' = bX +dY + y </tt>
*/
inline void fl_mult_matrix(double a, double b, double c, double d, double x, double y) {
  fl_graphics_driver->mult_matrix(a, b, c, d, x, y);
}
/**
  Start drawing a list of points. Points are added to the list with fl_vertex().
*/
inline void fl_begin_points() {
  fl_graphics_driver->begin_points();
}
/**
  Start drawing a list of lines.
*/
inline void fl_begin_line() {
  fl_graphics_driver->begin_line();
}
/**
  Start drawing a closed sequence of lines.
*/
inline void fl_begin_loop() {
  fl_graphics_driver->begin_loop();
}
/**
  Start drawing a convex filled polygon.
*/
inline void fl_begin_polygon() {
  fl_graphics_driver->begin_polygon();
}
/**
  Add a single vertex to the current path.
  \param[in] x,y coordinate
*/
inline void fl_vertex(double x, double y) {
  fl_graphics_driver->vertex(x, y);
}
/**
  Add a series of points on a Bézier curve to the path.
  The curve ends (and two of the points) are at X0,Y0 and X3,Y3.
  \param[in] X0,Y0 curve start point
  \param[in] X1,Y1 curve control point
  \param[in] X2,Y2 curve control point
  \param[in] X3,Y3 curve end point
*/
inline void fl_curve(double X0, double Y0, double X1, double Y1, double X2, double Y2, double X3, double Y3) {
  fl_graphics_driver->curve(X0, Y0, X1, Y1, X2, Y2, X3, Y3);
}
/**
  Add a series of points to the current path on the arc of a circle.

  The arc is drawn counter-clockwise from 3 o'clock. If \p end is less than
  \p start then it draws the arc in a clockwise direction. To draw an arc across
  the 3 o'clock line, \p start and \p end can be greater than 360 or less than 0.
  For example, to draw a counter-clockwise arc from 6 to 12 o'clock, \p start
  would be -90 deg, and \p end would be at +90 deg.

  You can get elliptical paths by using scale and rotate before calling fl_arc().

  \param[in] x, y, r center and radius of circular arc
  \param[in] start, end angles of start and end of arc measured in degrees

  \image html  fl_arc_xyr_diagram.png "fl_arc(x,y,r,a1,a2)"
  \image latex fl_arc_xyr_diagram.png "fl_arc(x,y,r,a1,a2)" width=6cm

  Examples:
  \code
    // Draw an arc of points
    fl_begin_points();
    fl_arc(100.0, 100.0, 50.0, 0.0, 180.0);
    fl_end_points();

    // Draw arc with a line
    fl_begin_line();
    fl_arc(200.0, 100.0, 50.0, 0.0, 180.0);
    fl_end_line();

    // Draw filled arc
    fl_begin_polygon();
    fl_arc(300.0, 100.0, 50.0, 0.0, 180.0);
    fl_end_polygon();
  \endcode
*/
inline void fl_arc(double x, double y, double r, double start, double end) {
  fl_graphics_driver->arc(x, y, r, start, end);
}
/**
  fl_circle(x,y,r) is equivalent to fl_arc(x,y,r,0,360), but may be faster.
 \param[in] x,y,r center and radius of circle

 \note fl_circle() is best used as part of the \ref drawing_complex  API, that is,
 flanked by fl_begin_XXX() and fl_end_XXX() calls where XXX can be 'loop' or 'polygon'
 to draw, respectively a circle or a disk. Transformation functions (e.g., fl_scale(double, double))
 can be also used for fl_circle() to draw empty of filled ellipses.
 It must be the \e only thing in the path: if you want a circle as part of
 a complex polygon you must use fl_arc().
 <br>Nevertheless, fl_circle() can also be used by itself to draw circles.
*/
inline void fl_circle(double x, double y, double r) {
  fl_graphics_driver->circle(x, y, r);
}
/**
  End list of points, and draw.
*/
inline void fl_end_points() {
  fl_graphics_driver->end_points();
}
/**
  End list of lines, and draw.
*/
inline void fl_end_line() {
  fl_graphics_driver->end_line();
}
/**
  End closed sequence of lines, and draw.
*/
inline void fl_end_loop() {
  fl_graphics_driver->end_loop();
}
/**
  End convex filled polygon, and draw.
*/
inline void fl_end_polygon() {
  fl_graphics_driver->end_polygon();
}
/**
  Start drawing a complex filled polygon.

  The polygon may be concave, may have holes in it, or may be several
  disconnected pieces. Call fl_gap() to separate loops of the path.

  To outline the polygon, use fl_begin_loop() and replace each fl_gap()
  with fl_end_loop();fl_begin_loop() pairs.

  \note For portability, you should only draw polygons that appear the same
    whether "even/odd" or "non-zero" winding rules are used to fill them.
    Holes should be drawn in the opposite direction to the outside loop.
*/
inline void fl_begin_complex_polygon() {
  fl_graphics_driver->begin_complex_polygon();
}
/**
  Separate loops of the path.

  It is unnecessary but harmless to call fl_gap() before the first vertex,
  after the last vertex, or several times in a row.
*/
inline void fl_gap() {
  fl_graphics_driver->gap();
}
/**
  End complex filled polygon, and draw.
*/
inline void fl_end_complex_polygon() {
  fl_graphics_driver->end_complex_polygon();
}
// get and use transformed positions:
/**
  Transform coordinate using the current transformation matrix.
  \param[in] x,y coordinate
*/
inline double fl_transform_x(double x, double y) {
  return fl_graphics_driver->transform_x(x, y);
}
/**
  Transform coordinate using the current transformation matrix.
  \param[in] x,y coordinate
*/
inline double fl_transform_y(double x, double y) {
  return fl_graphics_driver->transform_y(x, y);
}
/**
  Transform distance using current transformation matrix.
  \param[in] x,y coordinate
*/
inline double fl_transform_dx(double x, double y) {
  return fl_graphics_driver->transform_dx(x, y);
}
/**
  Transform distance using current transformation matrix.
  \param[in] x,y coordinate
*/
inline double fl_transform_dy(double x, double y) {
  return fl_graphics_driver->transform_dy(x, y);
}
/**
  Add coordinate pair to the vertex list without further transformations.
  \param[in] xf,yf transformed coordinate
*/
inline void fl_transformed_vertex(double xf, double yf) {
  fl_graphics_driver->transformed_vertex(xf, yf);
}

/** Copy a rectangular area of the given offscreen buffer into the current drawing destination.
  \param x,y        position where to draw the copied rectangle
  \param w,h        size of the copied rectangle
  \param pixmap     offscreen buffer containing the rectangle to copy
  \param srcx,srcy  origin in offscreen buffer of rectangle to copy
*/
inline void fl_copy_offscreen(int x, int y, int w, int h, Fl_Offscreen pixmap, int srcx, int srcy) {
  fl_graphics_driver->copy_offscreen(x, y, w, h, pixmap, srcx, srcy);
}

FL_EXPORT Fl_Offscreen fl_create_offscreen(int w, int h);
FL_EXPORT void fl_begin_offscreen(Fl_Offscreen b);
FL_EXPORT void fl_end_offscreen(void);
FL_EXPORT void fl_delete_offscreen(Fl_Offscreen bitmap);
FL_EXPORT void fl_rescale_offscreen(Fl_Offscreen &ctx);

/** @} */

/** \addtogroup  fl_attributes
    @{ */
/* NOTE: doxygen comments here to avoid triplication in os-specific sources */

// Fonts:
/*
  Set the current font, which is then used in various drawing routines.
  Implemented and documented in src/fl_draw.cxx
*/
FL_EXPORT void fl_font(Fl_Font face, Fl_Fontsize fsize);

/**
  Return the \p face set by the most recent call to fl_font().
  This can be used to save/restore the font.
*/
inline Fl_Font fl_font() {
  return fl_graphics_driver->font();
}
/**
  Return the \p size set by the most recent call to fl_font().
  This can be used to save/restore the font.
*/
inline Fl_Fontsize fl_size() {
  return fl_graphics_driver->size();
}

// Information you can get about the current font:
/**
  Return the recommended minimum line spacing for the current font.
  You can also use the value of \p size passed to fl_font().
*/
inline int fl_height() {
  return fl_graphics_driver->height();
}
FL_EXPORT int fl_height(int font, int size);
/**
  Return the recommended distance above the bottom of a fl_height() tall
  box to draw the text at so it looks centered vertically in that box.
*/
inline int fl_descent() {
  return fl_graphics_driver->descent();
}
/** Return the typographical width of a nul-terminated string
    using the current font face and size.
*/
FL_EXPORT double fl_width(const char *txt);

/** Return the typographical width of a sequence of \p n characters
    using the current font face and size.
*/
inline double fl_width(const char *txt, int n) {
  return fl_graphics_driver->width(txt, n);
}
/** Return the typographical width of a single character
    using the current font face and size.

  \note If a valid fl_gc is NOT found then it uses the first window gc,
  or the screen gc if no fltk window is available when called.
*/
inline double fl_width(unsigned int c) {
  return fl_graphics_driver->width(c);
}
/** Determine the minimum pixel dimensions of a nul-terminated string
    using the current fl_font().

  Usage: given a string "txt" drawn using fl_draw(txt, x, y) you would determine
  its pixel extents on the display using fl_text_extents(txt, dx, dy, wo, ho)
  such that a bounding box that exactly fits around the text could be drawn with
  fl_rect(x+dx, y+dy, wo, ho). Note the dx, dy values hold the offset of the first
  "colored in" pixel of the string, from the draw origin.

  Note the desired font and font size must be set with fl_font() before calling
  this function.

  This differs slightly from fl_measure() in that the dx/dy values are also
  returned.

  No FLTK symbol expansion will be performed.

  Example use:
  \code
    int dx,dy,W,H;
    fl_font(FL_HELVETICA, 12);                   // set font face+size first
    fl_text_extents("Some text", dx, dy, W, H);  // get width and height of string
    printf("text's width=%d, height=%d\n", W, H);
  \endcode
*/
FL_EXPORT void fl_text_extents(const char *, int &dx, int &dy, int &w, int &h);

/** Determine the minimum pixel dimensions of a sequence of \p n characters
    (bytes) using the current fl_font().

  \note The string length is measured in bytes, not (UTF-8) characters.
  \see fl_text_extents(const char*, int& dx, int& dy, int& w, int& h)
*/
inline void fl_text_extents(const char *t, int n, int &dx, int &dy, int &w, int &h) {
  fl_graphics_driver->text_extents(t, n, dx, dy, w, h);
}

// font encoding:
// Note: doxygen comments here to avoid duplication for os-specific cases
/**
  Convert text from Windows/X11 latin1 character set to local encoding.
  \param[in] t character string (latin1 encoding)
  \param[in] n optional number of characters (bytes) to convert (default is all)
  \returns pointer to internal buffer containing converted characters
*/
FL_EXPORT const char *fl_latin1_to_local(const char *t, int n = -1);
/**
  Convert text from local encoding to Windows/X11 latin1 character set.
  \param[in] t character string (local encoding)
  \param[in] n optional number of characters (bytes) to convert (default is all)
  \returns pointer to internal buffer containing converted characters
*/
FL_EXPORT const char *fl_local_to_latin1(const char *t, int n = -1);
/**
  Convert text from Mac Roman character set to local encoding.
  \param[in] t character string (Mac Roman encoding)
  \param[in] n optional number of characters to convert (default is all)
  \returns pointer to internal buffer containing converted characters
*/
FL_EXPORT const char *fl_mac_roman_to_local(const char *t, int n = -1);
/**
  Convert text from local encoding to Mac Roman character set.
  \param[in] t character string (local encoding)
  \param[in] n optional number of characters to convert (default is all)
  \returns pointer to internal buffer containing converted characters
*/
FL_EXPORT const char *fl_local_to_mac_roman(const char *t, int n = -1);
/** @} */

/** \addtogroup  fl_drawings
    @{ */

FL_EXPORT float fl_override_scale();

FL_EXPORT void fl_restore_scale(float s);

/**
  Draw a nul-terminated UTF-8 string starting at the given \p x, \p y location.

  Text is aligned to the left and to the baseline of the font.
  To align to the bottom, subtract fl_descent() from \p y.
  To align to the top, subtract fl_descent() and add fl_height().
  This version of fl_draw provides direct access to the text drawing
  function of the underlying OS. It does not apply any special handling
  to control characters.
*/
FL_EXPORT void fl_draw(const char *str, int x, int y);
/**
  Draw a nul-terminated UTF-8 string starting at the given \p x, \p y
  location and rotating \p angle degrees counter-clockwise.
  This version of fl_draw provides direct access to the text drawing
  function of the underlying OS and is supported by all fltk platforms except
  X11 without Xft.
*/
FL_EXPORT void fl_draw(int angle, const char *str, int x, int y);
/**
  Draws starting at the given \p x, \p y location a UTF-8 string of length \p n bytes.
*/
inline void fl_draw(const char *str, int n, int x, int y) {
  fl_graphics_driver->draw(str, n, x, y);
}
/**
  Draw at the given \p x, \p y location a UTF-8 string of length \p n bytes
  rotating \p angle degrees counter-clockwise.

  \note When using X11 (Unix, Linux, Cygwin et al.) this needs Xft to work.
        Under plain X11 (w/o Xft) rotated text is not supported by FLTK.
        A warning will be issued to stderr at runtime (only once) if you
        use this method with an angle other than 0.
*/
inline void fl_draw(int angle, const char *str, int n, int x, int y) {
  fl_graphics_driver->draw(angle, str, n, x, y);
}
/**
  Draw a UTF-8 string of length \p n bytes right to left starting at the given \p x, \p y location.
*/
inline void fl_rtl_draw(const char *str, int n, int x, int y) {
  fl_graphics_driver->rtl_draw(str, n, x, y);
}
FL_EXPORT void fl_measure(const char *str, int &x, int &y, int draw_symbols = 1);
/**
  Fancy string drawing function which is used to draw all the labels.

  The string is formatted and aligned inside the passed box.
  Handles '\\t' and '\\n', expands all other control characters to '^X',
  and aligns inside or against the edges of the box.
  See Fl_Widget::align() for values of \p align. The value FL_ALIGN_INSIDE
  is ignored, as this function always prints inside the box.
  If \p img is provided and is not \p NULL, the image is drawn above or
  below the text as specified by the \p align value.
  The \p draw_symbols argument specifies whether or not to look for symbol
  names starting with the '\@' character'

 \param[in] str UTF-8 string, can start and end with an '\@sym' symbol,
            can contain '\\n'
 \param[in] x,y,w,h bounding box
 \param[in] align label and image alignment in bounding box
 \param[in] img pointer to image
 \param[in] draw_symbols if true, interprete leading and trailing '\@sym'
            as graphical symbols
 \param[in] spacing spacing between text and image
 */
FL_EXPORT void fl_draw(const char *str, int x, int y, int w, int h,
                       Fl_Align align, Fl_Image *img = 0,
                       int draw_symbols = 1, int spacing = 0);
/**
 The same as fl_draw(const char*,int,int,int,int,Fl_Align,Fl_Image*,int) with
 the addition of the \p callthis parameter, which is a pointer to a text drawing
 function such as fl_draw(const char*, int, int, int) to do the real work.

 \param[in] str UTF-8 string, can start and end with an '\@sym' symbol,
 can contain '\\n'
 \param[in] x,y,w,h bounding box
 \param[in] align label and image alignment in bounding box
 \param[in] callthis pointer to text drawing function
 \param[in] img pointer to image
 \param[in] draw_symbols if true, interprete leading and trailing '\@sym'
 as graphical symbols
 \param[in] spacing spacing between text and image
 */
FL_EXPORT void fl_draw(const char *str, int x, int y, int w, int h, Fl_Align align,
                       void (*callthis)(const char *, int, int, int),
                       Fl_Image *img = 0, int draw_symbols = 1, int spacing = 0);

// boxtypes:

FL_EXPORT void fl_frame(const char *s, int x, int y, int w, int h);
FL_EXPORT void fl_frame2(const char *s, int x, int y, int w, int h);
FL_EXPORT void fl_draw_box(Fl_Boxtype, int x, int y, int w, int h, Fl_Color);
FL_EXPORT void fl_draw_box_focus(Fl_Boxtype, int x, int y, int w, int h, Fl_Color, Fl_Color);

// basic GUI objects (check marks, arrows, more to come ...):

// Draw a check mark in the given color inside the bounding box bb.
void fl_draw_check(Fl_Rect bb, Fl_Color col);

// Draw one or more "arrows" (triangles)
FL_EXPORT void fl_draw_arrow(Fl_Rect bb, Fl_Arrow_Type t, Fl_Orientation o, Fl_Color color);

// Draw a potentially small, filled circle
FL_EXPORT void fl_draw_circle(int x, int y, int d, Fl_Color color);

// Draw the full "radio button" of a radio menu entry or radio button
// This requires scheme specific handling (particularly gtk+ scheme)
FL_EXPORT void fl_draw_radio(int x, int y, int d, Fl_Color color);

// images:

/**
  Draw an 8-bit per color RGB or luminance image.
  \param[in] buf points at the "r" data of the top-left pixel.
                 Color data must be in <tt>r,g,b</tt> order.
                 Luminance data is only one <tt>gray</tt> byte.
  \param[in] X,Y position where to put top-left corner of image
  \param[in] W,H size of the image
  \param[in] D   delta to add to the pointer between pixels. It may be
                 any value greater than or equal to 1, or it can be
                 negative to flip the image horizontally
  \param[in] L   delta to add to the pointer between lines (if 0 is
                 passed it uses \p W * \p D), and may be larger than
                 \p W * \p D to crop data, or negative to flip the
                 image vertically

  It is highly recommended that you put the following code before the
  first <tt>show()</tt> of \e any window in your program to get rid of
  the dithering if possible:
  \code
  Fl::visual(FL_RGB);
  \endcode

  Gray scale (1-channel) images may be drawn. This is done if
  <tt>abs(D)</tt> is less than 3, or by calling fl_draw_image_mono().
  Only one 8-bit sample is used for each pixel, and on screens with
  different numbers of bits for red, green, and blue only gray colors
  are used. Setting \p D greater than 1 will let you display one channel
  of a color image.

  \par Note:
  The X version does not support all possible visuals. If FLTK cannot
  draw the image in the current visual it will abort. FLTK supports
  any visual of 8 bits or less, and all common TrueColor visuals up
  to 32 bits.
*/
inline void fl_draw_image(const uchar *buf, int X, int Y, int W, int H, int D = 3, int L = 0) {
  fl_graphics_driver->draw_image_general_(buf, X, Y, W, H, D, L);
}

/**
  Draw a gray-scale (1 channel) image.
  \see fl_draw_image(const uchar* buf, int X,int Y,int W,int H, int D, int L)
*/
inline void fl_draw_image_mono(const uchar *buf, int X, int Y, int W, int H, int D = 1, int L = 0) {
  fl_graphics_driver->draw_image_mono_general_(buf, X, Y, W, H, D, L);
}

/**
  Draw an image using a callback function to generate image data.

  You can generate the image as it is being drawn, or do arbitrary
  decompression of stored data, provided it can be decompressed to
  individual scan lines.

  \param[in] cb   callback function to generate scan line data
  \param[in] data user data passed to callback function
  \param[in] X,Y  screen position of top left pixel
  \param[in] W,H  image width and height
  \param[in] D    data size per pixel in bytes (must be greater than 0)

  \see fl_draw_image(const uchar* buf, int X, int Y, int W, int H, int D, int L)

  The callback function \p cb is called with the <tt>void*</tt> \p data
  user data pointer to allow access to a structure of information about
  the image, and the \p x, \p y, and \p w of the scan line desired from
  the image. 0,0 is the upper-left corner of the image, not \p x, \p y.
  A pointer to a buffer to put the data into is passed. You must copy
  \p w pixels from scanline \p y, starting at pixel \p x, to this buffer.

  Due to cropping, less than the whole image may be requested. So \p x
  may be greater than zero, the first \p y may be greater than zero,
  and \p w may be less than \p W. The buffer is long enough to store
  the entire \p W * \p D pixels, this is for convenience with some
  decompression schemes where you must decompress the entire line at
  once: decompress it into the buffer, and then if \p x is not zero,
  copy the data over so the \p x'th pixel is at the start of the buffer.

  You can assume the \p y's will be consecutive, except the first one
  may be greater than zero.

  If \p D is 4 or more, you must fill in the unused bytes with zero.
*/
inline void fl_draw_image(Fl_Draw_Image_Cb cb, void *data, int X, int Y, int W, int H, int D = 3) {
  fl_graphics_driver->draw_image(cb, data, X, Y, W, H, D);
}

/**
  Draw a gray-scale image using a callback function to generate image data.
  \see fl_draw_image(Fl_Draw_Image_Cb cb, void* data, int X,int Y,int W,int H, int D)
*/
inline void fl_draw_image_mono(Fl_Draw_Image_Cb cb, void *data, int X, int Y, int W, int H, int D = 1) {
  fl_graphics_driver->draw_image_mono(cb, data, X, Y, W, H, D);
}

/**
  Check whether platform supports true alpha blending for RGBA images.
  \returns 1 if true alpha blending supported by platform
  \returns 0 not supported so FLTK will use screen door transparency
*/
inline char fl_can_do_alpha_blending() {
  return Fl_Graphics_Driver::default_driver().can_do_alpha_blending();
}

FL_EXPORT uchar *fl_read_image(uchar *p, int X, int Y, int W, int H, int alpha = 0);
FL_EXPORT Fl_RGB_Image *fl_capture_window(Fl_Window *win, int x, int y, int w, int h);

// pixmaps:
/**
  Draw XPM image data, with the top-left corner at the given position.
  The image is dithered on 8-bit displays so you won't lose color
  space for programs displaying both images and pixmaps.

  \param[in] data pointer to XPM image data
  \param[in] x,y  position of top-left corner
  \param[in] bg   background color

  \returns 0 if there was any error decoding the XPM data.
*/
FL_EXPORT int fl_draw_pixmap(const char *const *data, int x, int y, Fl_Color bg = FL_GRAY);
/**
  Draw XPM image data, with the top-left corner at the given position.
  \see fl_draw_pixmap(const char* const* data, int x, int y, Fl_Color bg)
*/
inline int fl_draw_pixmap(/*const*/ char *const *data, int x, int y, Fl_Color bg = FL_GRAY) {
  return fl_draw_pixmap((const char *const *)data, x, y, bg);
}
FL_EXPORT int fl_measure_pixmap(/*const*/ char *const *data, int &w, int &h);
FL_EXPORT int fl_measure_pixmap(const char *const *cdata, int &w, int &h);

// other:
FL_EXPORT void fl_scroll(int X, int Y, int W, int H, int dx, int dy,
                         void (*draw_area)(void *, int, int, int, int), void *data);
FL_EXPORT const char *fl_shortcut_label(unsigned int shortcut);
FL_EXPORT const char *fl_shortcut_label(unsigned int shortcut, const char **eom);
FL_EXPORT unsigned int fl_old_shortcut(const char *s);
FL_EXPORT void fl_overlay_rect(int x, int y, int w, int h);
FL_EXPORT void fl_overlay_clear();
FL_EXPORT void fl_cursor(Fl_Cursor);
FL_EXPORT void fl_cursor(Fl_Cursor, Fl_Color fg, Fl_Color bg = FL_WHITE);
FL_EXPORT const char *fl_expand_text(const char *from, char *buf, int maxbuf, double maxw,
                                     int &n, double &width, int wrap, int draw_symbols = 0);

// XIM:
FL_EXPORT void fl_set_status(int X, int Y, int W, int H);
/** Inform text input methods about the current text insertion cursor.
 \param font Font currently in use in text input.
 \param size Size of the current font.
 \param X,Y Position of the bottom of the current text insertion cursor.
 \param W,H Width and height of the current text insertion cursor.
 \param win Points to the Fl_Window object containing the current text widget, or NULL.
 */
FL_EXPORT void fl_set_spot(int font, int size, int X, int Y, int W, int H, Fl_Window *win = 0);
/** Resets marked text.

 In many languages, typing a character can involve multiple keystrokes. For
 example, the Ä can be composed of two dots (¨) on top of the
 character, followed by the letter A (on a Mac with U.S. keyboard, you'd
 type Alt-U, Shift-A. To inform the user that the dots may be followed by
 another character, the ¨ is underlined).

 Call this function if character composition needs to be aborted for some
 reason. One such example would be the text input widget losing focus.
 */
FL_EXPORT void fl_reset_spot(void);


// XForms symbols:
FL_EXPORT int fl_draw_symbol(const char *label, int x, int y, int w, int h, Fl_Color);
FL_EXPORT int fl_add_symbol(const char *name, void (*drawit)(Fl_Color), int scalable);
/** @} */

#endif
