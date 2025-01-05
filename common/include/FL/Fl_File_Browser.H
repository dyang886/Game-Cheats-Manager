//
// FileBrowser definitions.
//
// Copyright 1999-2010 by Michael Sweet.
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
   Fl_File_Browser widget . */

//
// Include necessary header files...
//

#ifndef _Fl_File_Browser_H_
#  define _Fl_File_Browser_H_

#  include "Fl_Browser.H"
#  include "Fl_File_Icon.H"
#  include "filename.H"


//
// Fl_File_Browser class...
//

/** The Fl_File_Browser widget displays a list of filenames, optionally with file-specific icons. */
class FL_EXPORT Fl_File_Browser : public Fl_Browser {

  int           filetype_;
  const char    *directory_;
  uchar         iconsize_;
  const char    *pattern_;
  const char    *errmsg_;

  int   full_height() const FL_OVERRIDE;
  int   item_height(void *) const FL_OVERRIDE;
  int   item_width(void *) const FL_OVERRIDE;
  void  item_draw(void *, int, int, int, int) const FL_OVERRIDE;
  int   incr_height() const FL_OVERRIDE { return (item_height(0) + linespacing()); }

public:
  enum { FILES, DIRECTORIES };

  Fl_File_Browser(int, int, int, int, const char * = 0);
  ~Fl_File_Browser();

  /**    Sets or gets the size of the icons. The default size is 20 pixels.  */
  uchar         iconsize() const { return (iconsize_); }
  /**    Sets or gets the size of the icons. The default size is 20 pixels.  */
  void          iconsize(uchar s) { iconsize_ = s; redraw(); }

  /**
    Sets or gets the filename filter. The pattern matching uses
    the fl_filename_match()
    function in FLTK.
  */
  void  filter(const char *pattern);
  /**
    Sets or gets the filename filter. The pattern matching uses
    the fl_filename_match()
    function in FLTK.
  */
  const char    *filter() const { return (pattern_); }
  int           load(const char *directory, Fl_File_Sort_F *sort = fl_numericsort);
  Fl_Fontsize  textsize() const { return Fl_Browser::textsize(); }
  void          textsize(Fl_Fontsize s) { Fl_Browser::textsize(s); iconsize_ = (uchar)(3 * s / 2); }

  /**
    Sets or gets the file browser type, FILES or
    DIRECTORIES. When set to FILES, both
    files and directories are shown. Otherwise only directories are
    shown.
  */
  int           filetype() const { return (filetype_); }
  /**
    Sets or gets the file browser type, FILES or
    DIRECTORIES. When set to FILES, both
    files and directories are shown. Otherwise only directories are
    shown.
  */
  void          filetype(int t) { filetype_ = t; }
  void errmsg(const char *emsg);
  /**
    Returns OS error messages, or NULL if none. Use when advised.
   */
  const char* errmsg() const { return errmsg_; }
};

#endif // !_Fl_File_Browser_H_
