//
// Declaration of class Fl_PDF_File_Surface for the Fast Light Tool Kit (FLTK).
//
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

#ifndef PDF_FILE_SURFACE_H
#define PDF_FILE_SURFACE_H

#include <FL/Fl_Paged_Device.H>

/**
 To send graphical output to a PDF file.
 Class Fl_PDF_File_Surface is used exactly as the Fl_Printer class except for its 2 member functions begin_job()  and begin_document().
 <p><b>Platform notes:</b>
 - Windows:  requires "Microsoft Print to PDF" available in Windows 10 and later.
 - Wayland/X11: requires the FLTK library was built with FLTK_USE_PANGO=1.
 - macOS:  requires macOS 10.9 or later.
 <p>If the running platform doesn't fulfill the requirement above, the program runs but doesn't output any PDF.
*/
class FL_EXPORT Fl_PDF_File_Surface : public Fl_Paged_Device {
private:
  const char **out_filename_;
  Fl_Paged_Device *platform_surface_;
  static Fl_Paged_Device *new_platform_pdf_surface_(const char ***);
public:
  /** \name These attributes are useful for the Wayland/X11 platform only.
   \{
   */
  static const char * format_dialog_title;
  static const char * format_dialog_page_size;
  static const char * format_dialog_orientation;
  static const char * format_dialog_default;
  /** \} */
  Fl_PDF_File_Surface();
  ~Fl_PDF_File_Surface();
  /** Prepare to draw to a PDF document identified with a file chooser.
   A dialog opens to select the location and name of the output PDF document
   as well as its page format and orientation.
   \param defaultfilename Default name for the PDF document
   \param perr NULL or address of a string that receives a message in case of error.
   To be deleted[] after use.
   \return 0 for success, 1 when the user cancelled the operation, 2 when an error occurred.
   */
  int begin_job(const char* defaultfilename, char **perr = NULL);
  /** Don't use for this class */
  int begin_job(int, int *, int *, char **) FL_OVERRIDE {return 1;}
  /** Prepare to draw to a PDF document identified by its pathname.
   \param pathname Path name for the PDF document
   \param format The paper format for the PDF document
   \param layout The orientation for the PDF document
   \param perr NULL or address of a string that receives a message in case of error.
   To be deleted[] after use.
   \return 0 for success, 2 when an error occurred.
   */
  int begin_document(const char* pathname,
                enum Fl_Paged_Device::Page_Format format = Fl_Paged_Device::A4,
                enum Fl_Paged_Device::Page_Layout layout = Fl_Paged_Device::PORTRAIT,
                char **perr = NULL);
  int printable_rect(int *w, int *h) FL_OVERRIDE { return platform_surface_->printable_rect(w,h); }
  void margins(int *left, int *top, int *right, int *bottom) FL_OVERRIDE {
    platform_surface_->margins(left,top,right,bottom);
  }
  void origin(int x, int y) FL_OVERRIDE {platform_surface_->origin(x, y);}
  void origin(int *x, int *y) FL_OVERRIDE {platform_surface_->origin(x, y);}
  void scale(float s_x, float s_y = 0) FL_OVERRIDE {platform_surface_->scale(s_x, s_y);}
  void rotate(float angle) FL_OVERRIDE {platform_surface_->rotate(angle);}
  void translate(int x, int y) FL_OVERRIDE {platform_surface_->translate(x, y);}
  void untranslate() FL_OVERRIDE {platform_surface_->untranslate();};
  int begin_page(void) FL_OVERRIDE {return platform_surface_->begin_page();}
  int end_page(void) FL_OVERRIDE {return platform_surface_->end_page();}
  void end_job(void) FL_OVERRIDE {return platform_surface_->end_job();}
  /** Returns the name of the PDF document */
  inline const char *pdf_filename() { return *out_filename_; }
  void set_current() FL_OVERRIDE { if (platform_surface_) platform_surface_->set_current(); }
  bool is_current() FL_OVERRIDE { return surface() == platform_surface_; }
};

#endif // PDF_FILE_SURFACE_H
