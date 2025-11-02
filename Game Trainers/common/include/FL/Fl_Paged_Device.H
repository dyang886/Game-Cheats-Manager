//
// Printing support for the Fast Light Tool Kit (FLTK).
//
// Copyright 2010-2024 by Bill Spitzak and others.
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

/** \file Fl_Paged_Device.H
 \brief declaration of class Fl_Paged_Device.
 */

#ifndef Fl_Paged_Device_H
#define Fl_Paged_Device_H

#include <FL/Fl_Widget_Surface.H>


/** \brief Number of elements in enum Page_Format */
#define NO_PAGE_FORMATS 30 /* MSVC6 compilation fix */

/**
 \brief Represents page-structured drawing surfaces.

 This class has no public constructor: don't instantiate it; use Fl_Printer
 or Fl_PostScript_File_Device instead.
 */
class FL_EXPORT Fl_Paged_Device : public Fl_Widget_Surface {
protected:
  /** \brief The constructor */
  Fl_Paged_Device() : Fl_Widget_Surface(NULL) {}
public:
  /**
   Possible page formats.
   All paper formats with pre-defined width and height. The Fl_Paged_Device::page_formats array gives these widths and heights.
   */
  enum Page_Format {
    A0 = 0, /**<  A0 format */
    A1, /**<  A1 format */
    A2, /**<  A2 format */
    A3, /**<  A3 format */
    A4, /**<  A4 format */
    A5, /**<  A5 format */
    A6, /**<  A6 format */
    A7, /**<  A7 format */
    A8, /**<  A8 format */
    A9, /**<  A9 format */
    B0, /**<  B0 format */
    B1, /**<  B1 format */
    B2, /**<  B2 format */
    B3, /**<  B3 format */
    B4, /**<  B4 format */
    B5, /**<  B5 format */
    B6, /**<  B6 format */
    B7, /**<  B7 format */
    B8, /**<  B8 format */
    B9, /**<  B9 format */
    B10, /**<  B10 format */
    C5E,
    DLE,
    EXECUTIVE, /**<   Executive format */
    FOLIO, /**<  Folio  format */
    LEDGER, /**<  Ledger  format */
    LEGAL, /**<  Legal format */
    LETTER, /**<  Letter format */
    TABLOID, /**<   Tabloid format */
    ENVELOPE,
    MEDIA = 0x1000
  };
  /**
   \brief Possible page layouts.
   */
  enum Page_Layout {
    PORTRAIT = 0,  /**< Portrait orientation */
    LANDSCAPE = 0x100,   /**< Landscape orientation */
    REVERSED = 0x200,  /**< Reversed orientation */
    ORIENTATION = 0x300 /**<  orientation */
  };

  /** \brief width, height and name of a page format
 */
  typedef struct {
    /** \brief width in points */
    int width;
    /** \brief height in points */
    int height;
    /** \brief format name */
    const char *name;
  } page_format;
  /** \brief width, height and name of all elements of the enum \ref Page_Format.
   */
  static const page_format page_formats[NO_PAGE_FORMATS];
  /** \brief The destructor */
  virtual ~Fl_Paged_Device() {}
  virtual int begin_job(int pagecount = 0, int *frompage = NULL, int *topage = NULL, char **perr_message = NULL);
  /** Synonym of begin_job(int pagecount, int *frompage, int *topage, char **perr_message).
   For API compatibility with FLTK 1.3.x */
  int start_job(int pagecount = 0, int *frompage = NULL, int *topage = NULL, char **perr_message = NULL) {
    return begin_job(pagecount, frompage, topage, perr_message);
  }
  virtual int begin_page(void);
  /** Synonym of begin_page().
   For API compatibility with FLTK 1.3.x */
  int start_page() {return begin_page();}
  virtual void margins(int *left, int *top, int *right, int *bottom);
  virtual void scale(float scale_x, float scale_y = 0.);
  virtual void rotate(float angle);
  /** Synonym of draw(Fl_Widget*, int, int) */
  void print_widget(Fl_Widget* widget, int delta_x = 0, int delta_y = 0) {draw(widget, delta_x, delta_y);}
  /** Synonym of draw_decorated_window(Fl_Window*, int, int) */
  void print_window(Fl_Window *win, int x_off = 0, int y_off = 0) {
    draw_decorated_window(win, x_off, y_off);
  }
  virtual int end_page (void);
  virtual void end_job (void);
};

#endif // Fl_Paged_Device_H
