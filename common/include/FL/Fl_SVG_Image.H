//
// SVG Image header file for the Fast Light Tool Kit (FLTK).
//
// Copyright 2017-2022 by Bill Spitzak and others.
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

#ifndef FL_SVG_IMAGE_H
#define FL_SVG_IMAGE_H

#include <FL/Fl_Image.H>

struct NSVGimage;

/** The Fl_SVG_Image class supports loading, caching and drawing of scalable vector graphics (SVG) images.
 The FLTK library performs parsing and rasterization of SVG data using a modified version
 of the \c nanosvg software (https://github.com/memononen/nanosvg).
 The software modification allows the option to change the image ratio
 while performing rasterization.

 Use Fl_Image::fail() to check if the Fl_SVG_Image failed to load. fail() returns ERR_FILE_ACCESS
 if the file could not be opened or read, and ERR_FORMAT if the SVG format could not be decoded.
 If the image has loaded correctly, w(), h(), and d() should return values greater than zero.

 Rasterization is not done until the image is first drawn or resize() or normalize() is called. Therefore,
 \ref array is NULL until then. The delayed rasterization ensures an Fl_SVG_Image is always rasterized
 to the exact screen resolution at which it is drawn.

 The Fl_SVG_Image class draws images computed by \c nanosvg with the following known limitations

  - text between \c <text\> and </text\> marks,
  - \c image elements, and
  - <use\> statements

 are not rendered.

 The FLTK library can optionally be built without SVG support; in that case,
 class Fl_SVG_Image is unavailable.

 Example of displaying a hard-coded svg file:
 \code
  #include <FL/Fl.H>
  #include <FL/Fl_Window.H>
  #include <FL/Fl_Box.H>
  #include <FL/Fl_SVG_Image.H>

  // A black rotated rectangle
  const char *svg_data = "<svg viewBox=\"0 0 200 200\" version = \"1.1\">\n"
                         "<rect x=\"25\" y=\"50\" width=\"150\" height=\"100\" fill=\"black\" "
                         "transform=\"rotate(45 100 100)\"> </svg>\n";

  int main(int argc, char **argv) {
    Fl_SVG_Image *svg = new Fl_SVG_Image(0, svg_data);       // create SVG object
    Fl_Window    *win = new Fl_Window(720, 486, "svg test");
    Fl_Box       *box = new Fl_Box(0, 0, win->w(), win->h());
    box->image(svg);  // assign svg object to Fl_Box
    win->end();
    win->show(argc,argv);
    return(Fl::run());
  }
 \endcode

 Example of displaying an svg image from a file:
 \code
  #include <errno.h>   // errno
  #include <string.h>  // strerror
  #include <FL/Fl.H>
  #include <FL/Fl_Window.H>
  #include <FL/Fl_Box.H>
  #include <FL/Fl_SVG_Image.H>
  #include <FL/fl_message.H>
  int main(int argc, char **argv) {
    Fl_Window *win = new Fl_Window(720, 486, "svg test");
    Fl_Box    *box = new Fl_Box(0, 0, win->w(), win->h());

    // Load svg image from disk, assign to a box
    const char *svgpath = "/var/tmp/simple.svg";
    Fl_SVG_Image *svg = new Fl_SVG_Image(svgpath);  // load SVG object from disk
    switch (svg->fail()) {
      case Fl_Image::ERR_FILE_ACCESS:
        // File couldn't load? show path + os error to user
        fl_alert("%s: %s", svgpath, strerror(errno));
        return 1;
      case Fl_Image::ERR_FORMAT:
        // Parsing error
        fl_alert("%s: couldn't decode image", svgpath);
        return 1;
    }
    box->image(svg);  // assign svg object to box

    win->end();
    win->show(argc,argv);
    return(Fl::run());
  }
 \endcode

 Example of fitting an svg image to a resizable Fl_Box:
 \code
  #include <FL/Fl_Window.H>
  #include <FL/Fl_SVG_Image.H>
  #include <FL/Fl_Box.H>

  class resizable_box : public Fl_Box {
  public:
    resizable_box(int w, int h) : Fl_Box(0, 0, w, h, NULL) {}
    virtual void resize(int x, int y, int w, int h) {
      image()->scale(w, h, 1, 1); // p3 = proportional, p4 = can_expand
      Fl_Box::resize(x, y, w, h);
    }
  };

  int main(int argc, char **argv) {
    Fl_Window *win = new Fl_Window(130, 130);
    resizable_box *box = new resizable_box(win->w(), win->h());
    Fl_SVG_Image *svg = new Fl_SVG_Image("/path/to/image.svg");
    box->image(svg);
    svg->scale(box->w(), box->h());
    win->end();
    win->resizable(win);
    win->show(argc, argv);
    return Fl::run();
  }
 \endcode

 */
class FL_EXPORT Fl_SVG_Image : public Fl_RGB_Image {
private:
  typedef struct {
    NSVGimage* svg_image;
    int ref_count;
  } counted_NSVGimage;
  counted_NSVGimage* counted_svg_image_;
  bool rasterized_;
  int raster_w_, raster_h_;
  bool to_desaturate_;
  Fl_Color average_color_;
  float average_weight_;
  float svg_scaling_(int W, int H);
  void rasterize_(int W, int H);
  void cache_size_(int &width, int &height) FL_OVERRIDE;
  void init_(const char *name, const unsigned char *filedata, size_t length);
  Fl_SVG_Image(const Fl_SVG_Image *source);
public:
  /** Set this to \c false to allow image re-scaling that alters the image aspect ratio.
   Upon object creation, proportional is set to \c true, and the aspect ratio is kept constant.*/
  bool proportional;
  Fl_SVG_Image(const char *filename);
  Fl_SVG_Image(const char *sharedname, const char *svg_data);
  Fl_SVG_Image(const char *sharedname, const unsigned char *svg_data, size_t length);
  virtual ~Fl_SVG_Image();
  Fl_Image *copy(int W, int H) const FL_OVERRIDE;
  Fl_Image *copy() const {
    return Fl_Image::copy();
  }
  void resize(int width, int height);
  void desaturate() FL_OVERRIDE;
  void color_average(Fl_Color c, float i) FL_OVERRIDE;
  void draw(int X, int Y, int W, int H, int cx = 0, int cy = 0) FL_OVERRIDE;
  void draw(int X, int Y) { draw(X, Y, w(), h(), 0, 0); }
  Fl_SVG_Image *as_svg_image() FL_OVERRIDE { return this; }
  void normalize() FL_OVERRIDE;
};

#endif // FL_SVG_IMAGE_H
