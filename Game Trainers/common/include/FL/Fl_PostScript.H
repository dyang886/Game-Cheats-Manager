//
// Support for graphics output to PostScript file for the Fast Light Tool Kit (FLTK).
//
// Copyright 2010-2020 by Bill Spitzak and others.
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

/** \file Fl_PostScript.H
 \brief declaration of classes Fl_PostScript_File_Device and Fl_EPS_File_Surface.
 */

#ifndef Fl_PostScript_H
#define Fl_PostScript_H

#include <FL/Fl_Paged_Device.H>
#include <FL/fl_draw.H>
#include <stdarg.h>

/** Signature of functions FLTK may use to close FILE variables after PostScript/EPS output.
 A non-null return value indicates output error.
 \see Fl_PostScript_File_Device::close_command() and Fl_EPS_File_Surface::Fl_EPS_File_Surface().
 */
extern "C" {
  typedef int (*Fl_PostScript_Close_Command)(FILE *);
}

class Fl_PostScript_Graphics_Driver;

/**
 To send graphical output to a PostScript file.
 This class is used exactly as the Fl_Printer class except for the begin_job() call,
 two variants of which are usable and allow to specify what page format and layout are desired.

 <br><b>Processing of text</b>: Text uses vectorial fonts under the X11 + Pango and the Wayland platforms.
 With other platforms, only text restricted to the Latin alphabet (and a few other characters
 listed in the table below) and to FLTK standard fonts is vectorized.  All other unicode characters
 or all other fonts (FL_FREE_FONT and above) are output as a bitmap.
 FLTK standard fonts are output using the corresponding PostScript standard fonts.
 The latin alphabet means all unicode characters between U+0020 and U+017F, or, in other words,
 the ASCII, Latin-1 Supplement and Latin Extended-A charts.
 \htmlonly
 <table border=1>
 <caption>Extra characters supported by standard PostScript fonts</caption>
 <tr> <th>Char</th><th>Codepoint</th><th>Name</th>  <th>Char</th><th>Codepoint</th><th>Name</th> <th>Char</th><th>Codepoint</th><th>Name</th></tr>
 <tr><td>ƒ</td><td>U+0192</td><td>florin</td><td>‚</td><td>U+201A</td><td>quotesinglbase</td><td>™</td><td>U+2122</td><td>trademark</td></tr>
 <tr><td>ˆ</td><td>U+02C6</td><td>circumflex</td><td>“</td><td>U+201C</td><td>quotedblleft</td><td>∂</td><td>U+2202</td><td>partialdiff</td></tr>
 <tr><td>ˇ</td><td>U+02C7</td><td>caron</td><td>”</td><td>U+201D</td><td>quotedblright</td><td>Δ</td><td>U+2206</td><td>Delta</td></tr>
 <tr><td>˘</td><td>U+02D8</td><td>breve</td><td>„</td><td>U+201E</td><td>quotedblbase</td><td>∑</td><td>U+2211</td><td>summation</td></tr>
 <tr><td>˙</td><td>U+02D9</td><td>dotaccent</td><td>†</td><td>U+2020</td><td>dagger</td><td>√</td><td>U+221A</td><td>radical</td></tr>
 <tr><td>˚</td><td>U+02DA</td><td>ring</td><td>‡</td><td>U+2021</td><td>daggerdbl</td><td>∞</td><td>U+221E</td><td>infinity</td></tr>
 <tr><td>˛</td><td>U+02DB</td><td>ogonek</td><td>•</td><td>U+2022</td><td>bullet</td><td>≠</td><td>U+2260</td><td>notequal</td></tr>
 <tr><td>˜</td><td>U+02DC</td><td>tilde</td><td>…</td><td>U+2026</td><td>ellipsis</td><td>≤</td><td>U+2264</td><td>lessequal</td></tr>
 <tr><td>˝</td><td>U+02DD</td><td>hungarumlaut</td><td>‰</td><td>U+2030</td><td>perthousand</td><td>≥</td><td>U+2265</td><td>greaterequal</td></tr>
 <tr><td>–</td><td>U+2013</td><td>endash</td><td>‹</td><td>U+2039</td><td>guilsinglleft</td><td>◊</td><td>U+25CA</td><td>lozenge</td></tr>
 <tr><td>—</td><td>U+2014</td><td>emdash</td><td>›</td><td>U+203A</td><td>guilsinglright</td><td>ﬁ</td><td>U+FB01</td><td>fi</td></tr>
 <tr><td>‘</td><td>U+2018</td><td>quoteleft</td><td>/</td><td>U+2044</td><td>fraction</td><td>ﬂ</td><td>U+FB02</td><td>fl</td></tr>
 <tr><td>’</td><td>U+2019</td><td>quoteright</td><td>€</td><td>U+20AC</td><td>Euro</td><td></td><td>U+F8FF</td><td>apple (macOS only)</td></tr>
 </table>
 \endhtmlonly

 \image latex extraPSchars.png "Extra characters supported by standard PostScript fonts" width=16cm

 <b>Processing of transparent Fl_RGB_Image objects</b>: Under the X11 + Pango and the Wayland platforms,
 these objects are output with their exact transparency. With other platforms, these objects
 are drawn blended to white color. Class Fl_EPS_File_Surface 's constructor allows to set another
 background color for blending.

 */
class FL_EXPORT Fl_PostScript_File_Device : public Fl_Paged_Device {
private:
  // memorize the display's current font to restore it when the object ceases being current
  Fl_Font display_font_;
  Fl_Fontsize display_size_;
protected:
  /**
   \brief Returns the PostScript driver of this drawing surface.
   */
  inline Fl_PostScript_Graphics_Driver *driver() { return (Fl_PostScript_Graphics_Driver*)Fl_Surface_Device::driver(); }
public:
  /** The constructor. */
    Fl_PostScript_File_Device();
  /** The destructor. */
  ~Fl_PostScript_File_Device();
  /** Don't use with this class. */
  int begin_job(int pagecount, int* from, int* to, char **perr_message) FL_OVERRIDE;
  /** Begins the session where all graphics requests will go to a local PostScript file.
   Opens a file dialog to select an output PostScript file.
   This member function makes end_job() close the resulting PostScript file and display an
   alert message with fl_alert() in case of any output error.
   @param pagecount The total number of pages to be created. Use 0 if this number is unknown when this function is called.
   @param format Desired page format.
   @param layout Desired page layout.
   @return 0 if OK, 1 if user cancelled the file dialog, 2 if fopen failed on user-selected output file.
   */
 int begin_job(int pagecount = 0, enum Fl_Paged_Device::Page_Format format = Fl_Paged_Device::A4,
                enum Fl_Paged_Device::Page_Layout layout = Fl_Paged_Device::PORTRAIT);
  /** Synonym of begin_job().
   For API compatibility with FLTK 1.3.x */
  int start_job(int pagecount = 0, enum Fl_Paged_Device::Page_Format format = Fl_Paged_Device::A4,
                enum Fl_Paged_Device::Page_Layout layout = Fl_Paged_Device::PORTRAIT) {
    return begin_job(pagecount, format, layout);
  }
  /** Begins the session where all graphics requests will go to FILE pointer.
   This member function prevents end_job() from closing \p ps_output, so the user can check with \p ferror(ps_output)
   for output errors.
   @param ps_output A writable FILE pointer that will receive PostScript output and that should not be closed
   until after end_job() has been called.
   @param pagecount The total number of pages to be created. Use 0 if this number is unknown when this function is called.
   @param format Desired page format.
   @param layout Desired page layout.
   @return always 0.
   */
  int begin_job(FILE *ps_output, int pagecount = 0, enum Fl_Paged_Device::Page_Format format = Fl_Paged_Device::A4,
                enum Fl_Paged_Device::Page_Layout layout = Fl_Paged_Device::PORTRAIT);
  /** Synonym of begin_job().
   For API compatibility with FLTK 1.3.x */
  int start_job(FILE *ps_output, int pagecount = 0, enum Fl_Paged_Device::Page_Format format = Fl_Paged_Device::A4,
                enum Fl_Paged_Device::Page_Layout layout = Fl_Paged_Device::PORTRAIT) {
    return begin_job(ps_output, pagecount, format, layout);
  }

  int begin_page (void) FL_OVERRIDE;
  int printable_rect(int *w, int *h) FL_OVERRIDE;
  void margins(int *left, int *top, int *right, int *bottom) FL_OVERRIDE;
  void origin(int *x, int *y) FL_OVERRIDE;
  void origin(int x, int y) FL_OVERRIDE;
  void scale (float scale_x, float scale_y = 0.) FL_OVERRIDE;
  void rotate(float angle) FL_OVERRIDE;
  void translate(int x, int y) FL_OVERRIDE;
  void untranslate(void) FL_OVERRIDE;
  int end_page (void) FL_OVERRIDE;
  /** Completes all PostScript output.
   This also closes with \p fclose() the underlying file() unless close_command() was used to set another function.
   */
  void end_job(void) FL_OVERRIDE;
  /** Label of the PostScript file chooser window */
  static const char *file_chooser_title;
  /** Returns the underlying FILE* receiving all PostScript data */
  FILE *file();
  /** Sets the function end_job() calls to close the file() */
  void close_command(Fl_PostScript_Close_Command cmd);
  void set_current() FL_OVERRIDE;
  void end_current() FL_OVERRIDE;
};

/** Encapsulated PostScript drawing surface.
 This drawing surface allows to store any FLTK graphics in vectorial form in an "Encapsulated PostScript" file.
 \n Usage example:
 \code
   Fl_Window *win = ...// Window to draw to an .eps file
   int ww = win->decorated_w();
   int wh = win->decorated_h();
   FILE *eps = fl_fopen("/path/to/mywindow.eps", "w");
   if (eps) {
     Fl_EPS_File_Surface *surface = new Fl_EPS_File_Surface(ww, wh, eps, win->color());
     Fl_Surface_Device::push_current(surface);
     surface->draw_decorated_window(win);
     Fl_Surface_Device::pop_current();
     delete surface; // the .eps file is not complete until the destructor was run
   }
 \endcode
 */
class FL_EXPORT Fl_EPS_File_Surface : public Fl_Widget_Surface {
protected:
  /** Returns the PostScript driver of this drawing surface. */
  inline Fl_PostScript_Graphics_Driver *driver() { return (Fl_PostScript_Graphics_Driver*)Fl_Surface_Device::driver(); }
public:
  /**
  Constructor.
  \param width,height Width and height of the EPS drawing area
  \param eps_output A writable FILE pointer where the Encapsulated PostScript data will be sent
   \param background Color expected to cover the background of the EPS drawing area.
   This parameter affects only the drawing of transparent Fl_RGB_Image objects:
   transparent areas of RGB images are blended with the \p background color.
   Under the X11 + pango platform, transparent RGB images are correctly blended to their background,
   thus this parameter has no effect.
   \param closef If not NULL, the destructor or close() will call \p closef(eps_output) after all
    EPS data has been sent. If NULL, \p fclose(eps_output) is called instead. This allows to close the FILE
    pointer by, e.g., \p pclose, or, using a function such as \p "int keep_open(FILE*){return 0;}", to keep it open after
    completion of all output to \p eps_output. Function \p closef should return non zero to indicate an error.
  */
  Fl_EPS_File_Surface(int width, int height, FILE *eps_output,
                      Fl_Color background = FL_WHITE, Fl_PostScript_Close_Command closef = NULL);
  /**
   Destructor.
   By default, the destructor closes with function \p fclose() the underlying FILE. See the constructor for how
   to close it differently or to keep it open. Use close() before object destruction to receive the status code
   of output operations. If close() is not used and if EPS output results in error, the destructor displays an alert message
   with fl_alert().
   */
  ~Fl_EPS_File_Surface();
  int printable_rect(int *w, int *h) FL_OVERRIDE;
  /** Returns the underlying FILE pointer */
  FILE *file();
  void origin(int x, int y) FL_OVERRIDE;
  void origin(int *px, int *py) FL_OVERRIDE;
  void translate(int x, int y) FL_OVERRIDE;
  void untranslate() FL_OVERRIDE;
  /** Completes all EPS output.
   The only operation possible with the Fl_EPS_File_Surface object after calling close() is its destruction.
   \return The status code of output operations to the FILE object. 0 indicates success. */
  int close();
};

#endif // Fl_PostScript_H
