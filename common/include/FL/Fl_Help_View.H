//
// Help Viewer widget definitions.
//
// Copyright 1997-2010 by Easy Software Products.
// Image support by Matthias Melcher, Copyright 2000-2009.
// Copyright 2011-2024 by Bill Spitzak and others.
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
   Fl_Help_View widget . */

#ifndef Fl_Help_View_H
#define Fl_Help_View_H

//
// Include necessary header files...
//

#include "Fl.H"
#include "Fl_Group.H"
#include "Fl_Scrollbar.H"
#include "fl_draw.H"
#include "filename.H"

class Fl_Shared_Image;
//
// Fl_Help_Func type - link callback function for files...
//

typedef const char *(Fl_Help_Func)(Fl_Widget *, const char *);

//
// Fl_Help_Block structure...
//

struct Fl_Help_Block {
  const char    *start,         // Start of text
                *end;           // End of text
  uchar         border;         // Draw border?
  Fl_Color      bgcolor;        // Background color
  int           x,              // Indentation/starting X coordinate
                y,              // Starting Y coordinate
                w,              // Width
                h;              // Height
  int           line[32];       // Left starting position for each line
  int           ol;             // is ordered list <OL> element
  int           ol_num;         // item number in ordered list
};

//
// Fl_Help_Link structure...
//
/** Definition of a link for the html viewer. */
struct Fl_Help_Link {
  char          filename[192],  ///< Reference filename
                name[32];       ///< Link target (blank if none)
  int           x,              ///< X offset of link text
                y,              ///< Y offset of link text
                w,              ///< Width of link text
                h;              ///< Height of link text
};

/*
 * Fl_Help_View font stack opaque implementation
 */

/** Fl_Help_View font stack element definition. */
struct FL_EXPORT Fl_Help_Font_Style {
  Fl_Font      f;  ///< Font
  Fl_Fontsize  s;  ///< Font Size
  Fl_Color     c;  ///< Font Color
  void get(Fl_Font &afont, Fl_Fontsize &asize, Fl_Color &acolor) {afont=f; asize=s; acolor=c;} ///< Gets current font attributes
  void set(Fl_Font afont, Fl_Fontsize asize, Fl_Color acolor) {f=afont; s=asize; c=acolor;} ///< Sets current font attributes
  Fl_Help_Font_Style(Fl_Font afont, Fl_Fontsize asize, Fl_Color acolor) {set(afont, asize, acolor);}
  Fl_Help_Font_Style(){} // For in table use
};

/** Fl_Help_View font stack definition. */
const size_t MAX_FL_HELP_FS_ELTS = 100;

struct FL_EXPORT Fl_Help_Font_Stack {
  /** font stack construction, initialize attributes. */
  Fl_Help_Font_Stack() {
    nfonts_ = 0;
  }

  void init(Fl_Font f, Fl_Fontsize s, Fl_Color c) {
    nfonts_ = 0;
    elts_[nfonts_].set(f, s, c);
    fl_font(f, s);
    fl_color(c);
  }
  /** Gets the top (current) element on the stack. */
  void top(Fl_Font &f, Fl_Fontsize &s, Fl_Color &c) { elts_[nfonts_].get(f, s, c); }
  /** Pushes the font style triplet on the stack, also calls fl_font() & fl_color() adequately */
  void push(Fl_Font f, Fl_Fontsize s, Fl_Color c) {
    if (nfonts_ <  MAX_FL_HELP_FS_ELTS-1) nfonts_ ++;
    elts_[nfonts_].set(f, s, c);
    fl_font(f, s); fl_color(c);
  }
  /** Pops from the stack the font style triplet and calls fl_font() & fl_color() adequately */
  void pop(Fl_Font &f, Fl_Fontsize &s, Fl_Color &c) {
    if (nfonts_ > 0) nfonts_ --;
    top(f, s, c);
    fl_font(f, s); fl_color(c);
  }
  /** Gets the current count of font style elements in the stack. */
  size_t count() const {return nfonts_;} // Gets the current number of fonts in the stack

protected:
  size_t nfonts_;               ///< current number of fonts in stack
  Fl_Help_Font_Style elts_[MAX_FL_HELP_FS_ELTS]; ///< font elements
};

/** Fl_Help_Target structure */

struct Fl_Help_Target {
  char          name[32];       ///< Target name
  int           y;              ///< Y offset of target
};

/**
  The Fl_Help_View widget displays HTML text. Most HTML 2.0
  elements are supported, as well as a primitive implementation of tables.
  GIF, JPEG, and PNG images are displayed inline.

  Supported HTML tags:
     - A: HREF/NAME
     - B
     - BODY: BGCOLOR/TEXT/LINK
     - BR
     - CENTER
     - CODE
     - DD
     - DL
     - DT
     - EM
     - FONT: COLOR/SIZE/FACE=(helvetica/arial/sans/times/serif/symbol/courier)
     - H1/H2/H3/H4/H5/H6
     - HEAD
     - HR
     - I
     - IMG: SRC/WIDTH/HEIGHT/ALT
     - KBD
     - LI
     - OL
     - P
     - PRE
     - STRONG
     - TABLE: TH/TD/TR/BORDER/BGCOLOR/COLSPAN/ALIGN=CENTER|RIGHT|LEFT
     - TITLE
     - TT
     - U
     - UL
     - VAR

  Supported color names:
     - black,red,green,yellow,blue,magenta,fuchsia,cyan,aqua,white,gray,grey,lime,maroon,navy,olive,purple,silver,teal.

  Supported urls:
     - Internal: file:
     - External: http: ftp: https: ipp: mailto: news:

  Quoted char names:
     - Aacute aacute Acirc acirc acute AElig aelig Agrave agrave amp Aring aring Atilde atilde Auml auml
     - brvbar bull
     - Ccedil ccedil cedil cent copy curren
     - dagger deg divide
     - Eacute eacute Ecirc ecirc Egrave egrave ETH eth Euml euml euro
     - frac12 frac14 frac34
     - gt
     - Iacute iacute Icirc icirc iexcl Igrave igrave iquest Iuml iuml
     - laquo lt
     - macr micro middot
     - nbsp not Ntilde ntilde
     - Oacute oacute Ocirc ocirc Ograve ograve ordf ordm Oslash oslash Otilde otilde Ouml ouml
     - para permil plusmn pound
     - quot
     - raquo reg
     - sect shy sup1 sup2 sup3 szlig
     - THORN thorn times trade
     - Uacute uacute Ucirc ucirc Ugrave ugrave uml Uuml uuml
     - Yacute yacute
     - yen Yuml yuml

  \note You can't effectively set the box() to FL_NO_BOX, this would result
    in FL_DOWN_BOX being used as the boxtype of the widget. This is unexpected
    but can't be changed for backwards compatibility. If you don't want a frame
    around the widget you can use FL_FLAT_BOX instead.
*/
class FL_EXPORT Fl_Help_View : public Fl_Group {        // Help viewer widget

  enum { RIGHT = -1, CENTER, LEFT };    ///< Alignments

  char          title_[1024];           ///< Title string
  Fl_Color      defcolor_,              ///< Default text color
                bgcolor_,               ///< Background color
                textcolor_,             ///< Text color
                linkcolor_;             ///< Link color
  Fl_Font       textfont_;              ///< Default font for text
  Fl_Fontsize  textsize_;               ///< Default font size
  const char    *value_;                ///< HTML text value
  Fl_Help_Font_Stack fstack_;           ///< font stack management
  int           nblocks_,               ///< Number of blocks/paragraphs
                ablocks_;               ///< Allocated blocks
  Fl_Help_Block *blocks_;               ///< Blocks

  Fl_Help_Func  *link_;                 ///< Link transform function

  int           nlinks_,                ///< Number of links
                alinks_;                ///< Allocated links
  Fl_Help_Link  *links_;                ///< Links

  int           ntargets_,              ///< Number of targets
                atargets_;              ///< Allocated targets
  Fl_Help_Target *targets_;             ///< Targets

#if FL_ABI_VERSION >= 10401
  char          directory_[2 * FL_PATH_MAX + 15]; ///< Directory for current file
#else
  char          directory_[FL_PATH_MAX]; ///< Directory for current file
#endif

  char          filename_[FL_PATH_MAX]; ///< Current filename
  int           topline_,               ///< Top line in document
                leftline_,              ///< Lefthand position
                size_,                  ///< Total document length
                hsize_,                 ///< Maximum document width
                scrollbar_size_;        ///< Size for both scrollbars
  Fl_Scrollbar  scrollbar_,             ///< Vertical scrollbar for document
                hscrollbar_;            ///< Horizontal scrollbar

  static int    selection_first_;
  static int    selection_last_;
  static int    selection_push_first_;
  static int    selection_push_last_;
  static int    selection_drag_first_;
  static int    selection_drag_last_;
  static int    selected_;
  static int    draw_mode_;
  static int    mouse_x_;
  static int    mouse_y_;
  static int    current_pos_;
  static Fl_Help_View *current_view_;
  static Fl_Color hv_selection_color_;
  static Fl_Color hv_selection_text_color_;


  void initfont(Fl_Font &f, Fl_Fontsize &s, Fl_Color &c) { f = textfont_; s = textsize_; c = textcolor_; fstack_.init(f, s, c); }
  void pushfont(Fl_Font f, Fl_Fontsize s) {fstack_.push(f, s, textcolor_);}
  void pushfont(Fl_Font f, Fl_Fontsize s, Fl_Color c) {fstack_.push(f, s, c);}
  void popfont(Fl_Font &f, Fl_Fontsize &s, Fl_Color &c) {fstack_.pop(f, s, c);}

  Fl_Help_Block *add_block(const char *s, int xx, int yy, int ww, int hh, uchar border = 0);
  void          add_link(const char *n, int xx, int yy, int ww, int hh);
  void          add_target(const char *n, int yy);
  static int    compare_targets(const Fl_Help_Target *t0, const Fl_Help_Target *t1);
  int           do_align(Fl_Help_Block *block, int line, int xx, int a, int &l);
protected:
  void          draw() FL_OVERRIDE;
private:
  void          format();
  void          format_table(int *table_width, int *columns, const char *table);
  void          free_data();
  int           get_align(const char *p, int a);
  const char    *get_attr(const char *p, const char *n, char *buf, int bufsize);
  Fl_Color      get_color(const char *n, Fl_Color c);
  Fl_Shared_Image *get_image(const char *name, int W, int H);
  int           get_length(const char *l);
public:
  int           handle(int) FL_OVERRIDE;
private:

  void          hv_draw(const char *t, int x, int y, int entity_extra_length = 0);
  char          begin_selection();
  char          extend_selection();
  void          end_selection(int c=0);
  void          clear_global_selection();
  Fl_Help_Link  *find_link(int, int);
  void          follow_link(Fl_Help_Link*);

public:

  static const char *copy_menu_text;

  Fl_Help_View(int xx, int yy, int ww, int hh, const char *l = 0);
  ~Fl_Help_View();
  /** Returns the current directory for the text in the buffer. */
  const char    *directory() const { if (directory_[0]) return (directory_);
                                        else return ((const char *)0); }
  /** Returns the current filename for the text in the buffer. */
  const char    *filename() const { if (filename_[0]) return (filename_);
                                        else return ((const char *)0); }
  int           find(const char *s, int p = 0);
  /**
    This method assigns a callback function to use when a link is
    followed or a file is loaded (via Fl_Help_View::load()) that
    requires a different file or path.

    The callback function receives a pointer to the Fl_Help_View
    widget and the URI or full pathname for the file in question.
    It must return a pathname that can be opened as a local file or NULL:

    \code
    const char *fn(Fl_Widget *w, const char *uri);
    \endcode

    The link function can be used to retrieve remote or virtual
    documents, returning a temporary file that contains the actual
    data. If the link function returns NULL, the value of
    the Fl_Help_View widget will remain unchanged.

    If the link callback cannot handle the URI scheme, it should
    return the uri value unchanged or set the value() of the widget
    before returning NULL.
  */
  void          link(Fl_Help_Func *fn) { link_ = fn; }
  int           load(const char *f);
  void          resize(int,int,int,int) FL_OVERRIDE;
  /** Gets the size of the help view. */
  int           size() const { return (size_); }
  void          size(int W, int H) { Fl_Widget::size(W, H); }
  /** Sets the default text color. */
  void          textcolor(Fl_Color c) { if (textcolor_ == defcolor_) textcolor_ = c; defcolor_ = c; }
  /** Returns the current default text color. */
  Fl_Color      textcolor() const { return (defcolor_); }
  /** Sets the default text font. */
  void          textfont(Fl_Font f) { textfont_ = f; format(); }
  /** Returns the current default text font. */
  Fl_Font       textfont() const { return (textfont_); }
  /** Sets the default text size. */
  void          textsize(Fl_Fontsize s) { textsize_ = s; format(); }
  /** Gets the default text size. */
  Fl_Fontsize  textsize() const { return (textsize_); }
  /** Returns the current document title, or NULL if there is no title. */
  const char    *title() { return (title_); }
  void          topline(const char *n);
  void          topline(int);
  /** Returns the current top line in pixels. */
  int           topline() const { return (topline_); }
  void          leftline(int);
  /** Gets the left position in pixels. */
  int           leftline() const { return (leftline_); }
  void          value(const char *val);
  /** Returns the current buffer contents. */
  const char    *value() const { return (value_); }
  void          clear_selection();
  void          select_all();
  /**
    Gets the current size of the scrollbars' troughs, in pixels.

    If this value is zero (default), this widget will use the
    Fl::scrollbar_size() value as the scrollbar's width.

    \returns Scrollbar size in pixels, or 0 if the global Fl::scrollbar_size() is being used.
    \see Fl::scrollbar_size(int)
  */
  int scrollbar_size() const {
      return(scrollbar_size_);
  }
  /**
    Sets the pixel size of the scrollbars' troughs to \p newSize, in pixels.

    Normally you should not need this method, and should use
    Fl::scrollbar_size(int) instead to manage the size of ALL
    your widgets' scrollbars. This ensures your application
    has a consistent UI, is the default behavior, and is normally
    what you want.

    Only use THIS method if you really need to override the global
    scrollbar size. The need for this should be rare.

    Setting \p newSize to the special value of 0 causes the widget to
    track the global Fl::scrollbar_size(), which is the default.

    \param[in] newSize Sets the scrollbar size in pixels.\n
                    If 0 (default), scrollbar size tracks the global Fl::scrollbar_size()
    \see Fl::scrollbar_size()
  */
  void scrollbar_size(int newSize) {
      scrollbar_size_ = newSize;
  }

  // Check if the user selected text in this view.
  int text_selected();

  // If text is selected in this view, copy it to a clipboard.
  int copy(int clipboard=1);
};

#endif // !Fl_Help_View_H
