//
// Printing support for the Fast Light Tool Kit (FLTK).
//
// Copyright 2010-2016 by Bill Spitzak and others.
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

/** \file Fl_Printer.H
 \brief declaration of class Fl_Printer.
 */

#ifndef Fl_Printer_H
#define Fl_Printer_H

#include <FL/Fl_Paged_Device.H>

/**
 \brief OS-independent print support.
 Fl_Printer allows to use all drawing, color, text, image, and clip FLTK functions, and to have them operate
 on printed page(s). There are two main, non exclusive, ways to use it.
 <ul><li>Print any widget (standard, custom, Fl_Window, Fl_Gl_Window) as it appears
 on screen, with optional translation, scaling and rotation. This is done by calling print_widget(),
 print_window() or print_window_part().
 <li>Use a series of FLTK graphics commands (e.g., font, text, lines, colors, clip, image) to
 compose a page appropriately shaped for printing.
 </ul>
 In both cases, begin by begin_job(), begin_page(), printable_rect() and origin() calls
 and finish by end_page() and end_job() calls.
 <p>Example of use: print a widget centered in a page
 \code
 #include <FL/Fl_Printer.H>
 #include <FL/fl_draw.H>
 int width, height;
 Fl_Widget *widget = ...  // a widget we want printed
 Fl_Printer *printer = new Fl_Printer();
 if (printer->begin_job(1) == 0) {
  printer->begin_page();
  printer->printable_rect(&width, &height);
  fl_color(FL_BLACK);
  fl_line_style(FL_SOLID, 2);
  fl_rect(0, 0, width, height);
  fl_font(FL_COURIER, 12);
  time_t now; time(&now); fl_draw(ctime(&now), 0, fl_height());
  printer->origin(width/2, height/2);
  printer->print_widget(widget, -widget->w()/2, -widget->h()/2);
  printer->end_page();
  printer->end_job();
 }
 delete printer;
 \endcode
 <p>Recommended method to refresh GUI while printing :
 \code
 printer->begin_job(0);
 ……
 Fl_Surface_Device::push_current(Fl_Display_Device::display_device());
 Fl::check(); // or any operation that draws to display
 Fl_Surface_Device::pop_current();
 ……
 printer->end_job();
 \endcode
 <b>Platform specifics</b>
 <ul>
 <li>X11 and Wayland platforms:
 <ul><li>FLTK expresses all graphics data using (Level 2) PostScript and sends that to the selected printer.
 See class Fl_PostScript_File_Device for a description of how text and transparent images appear in print.
 <li>If the GTK library is available at run-time, class Fl_Printer runs GTK's printer dialog which allows to set
 printer, paper size and orientation.
 <li>If the GTK library is not available, or if Fl::option(Fl::OPTION_PRINTER_USES_GTK) has been turned off,
 class Fl_Printer runs FLTK's print dialog.
 <ul>
 <li>Unless it has been previously changed, the default paper size is A4.
 To change that, press the "Properties" button of the "Print" dialog window
 opened by an Fl_Printer::begin_job() call. This opens a "Printer Properties" window where it's
 possible to select the adequate paper size. Finally press the "Save" button therein to assign
 the chosen paper size to the chosen printer for this and all further print operations.
 <li>Use the static public attributes of this class to set the print dialog to other languages
 than English. For example, the "Printer:" dialog item Fl_Printer::dialog_printer can be set to French with:
 \code
 Fl_Printer::dialog_printer = "Imprimante:";
 \endcode
 before creation of the Fl_Printer object.
 <li>Use Fl_PostScript_File_Device::file_chooser_title to customize the title of the file chooser dialog that opens
 when using the "Print To File" option of the print dialog.
 </ul>
 </ul>
 <li>Windows platform: Transparent Fl_RGB_Image 's don't print with exact transparency on most printers
 (a workaround is to use print_window_part() ).
 Fl_RGB_Image 's don't rotate() well.
 <li>Mac OS X platform: all graphics requests print as on display and accept rotation and scaling.
 </ul>
 */
class FL_EXPORT Fl_Printer : public Fl_Paged_Device {
private:
  Fl_Paged_Device *printer;
  /** Each platform implements this function its own way */
  static Fl_Paged_Device* newPrinterDriver(void);
public:
  /** The constructor */
  Fl_Printer(void);
  int begin_job(int pagecount = 0, int *frompage = NULL, int *topage = NULL, char **perr_message = NULL) FL_OVERRIDE;
  int begin_page(void) FL_OVERRIDE;
  int printable_rect(int *w, int *h) FL_OVERRIDE;
  void margins(int *left, int *top, int *right, int *bottom) FL_OVERRIDE;
  void origin(int *x, int *y) FL_OVERRIDE;
  void origin(int x, int y) FL_OVERRIDE;
  void scale(float scale_x, float scale_y = 0.) FL_OVERRIDE;
  void rotate(float angle) FL_OVERRIDE;
  void translate(int x, int y) FL_OVERRIDE;
  void untranslate(void) FL_OVERRIDE;
  int end_page (void) FL_OVERRIDE;
  void end_job (void) FL_OVERRIDE;
  void set_current(void) FL_OVERRIDE;
  bool is_current() FL_OVERRIDE;

  /** \name These attributes are useful for the Linux/Unix platform only.
   \{
   */
  static const char *dialog_title;
  static const char *dialog_printer;
  static const char *dialog_range;
  static const char *dialog_copies;
  static const char *dialog_all;
  static const char *dialog_pages;
  static const char *dialog_from;
  static const char *dialog_to;
  static const char *dialog_properties;
  static const char *dialog_copyNo;
  static const char *dialog_print_button;
  static const char *dialog_cancel_button;
  static const char *dialog_print_to_file;
  static const char *property_title;
  static const char *property_pagesize;
  static const char *property_mode;
  static const char *property_use;
  static const char *property_save;
  static const char *property_cancel;
  /** \} */
  /** The destructor */
  ~Fl_Printer(void);
};

#endif // Fl_Printer_H
