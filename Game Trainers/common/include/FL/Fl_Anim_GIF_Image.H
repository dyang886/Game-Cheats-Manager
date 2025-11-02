//
// Fl_Anim_GIF_Image class header for the Fast Light Tool Kit (FLTK).
//
// Copyright 2016-2023 by Christian Grabner <wcout@gmx.net>.
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

#ifndef Fl_Anim_Gif_Image_H
#define Fl_Anim_Gif_Image_H

// forward declarations
class Fl_Image;
class Fl_Widget;

#include <FL/Fl_GIF_Image.H>

// Load and display animated GIF images
class FL_EXPORT Fl_Anim_GIF_Image : public Fl_GIF_Image {

  class FrameInfo; // internal helper class

public:

  /**
   When opening an Fl_Anim_GIF_Image there are some options
   that can be passed in a `flags` value.
   */
  enum Flags {
    /**
     This flag indicates to the loader that it should not start
     the animation immediately after successful load, which is
     the default.
     It can be started later using the \ref start() method.
     */
    DONT_START = 1,
    /**
     This flag indicates to the loader that it should not
     resize the canvas widget of the animation to the dimensions
     of the animation, which is the default.
     Needed for special use cases.
     */
    DONT_RESIZE_CANVAS = 2,
    /**
     This flag indicates to the loader that it should not
     set the animation as \ref image() member of the canvas widget,
     which is the default.
     Needed for special use cases.
     */
    DONT_SET_AS_IMAGE = 4,
    /**
     Often frames change just a small area of the animation canvas.
     This flag indicates to the loader to try using less memory
     by storing frame data not as canvas-sized images but use the
     sizes defined in the GIF file.
     The drawbacks are higher cpu usage during playback and maybe
     minor artifacts when resized.
     */
    OPTIMIZE_MEMORY = 8,
    /**
     This flag can be used to print informations about the
     decoding process to the console.
     */
    LOG_FLAG = 64,
    /**
     This flag can be used to print even more informations about
     the decoding process to the console.
     */
    DEBUG_FLAG = 128
  };

  // -- constructors and destructor
  Fl_Anim_GIF_Image(const char *filename, Fl_Widget *canvas = 0, unsigned short flags = 0);
  Fl_Anim_GIF_Image(const char* imagename, const unsigned char *data,
                    const size_t length, Fl_Widget *canvas = 0,
                    unsigned short flags = 0);
  Fl_Anim_GIF_Image();
  ~Fl_Anim_GIF_Image() FL_OVERRIDE;

  // -- file handling
  bool load(const char *name, const unsigned char *imgdata=NULL, size_t imglength=0);
  bool valid() const;

  // -- getters and setters
  void frame_uncache(bool uncache);
  bool frame_uncache() const;
  double delay(int frame_) const;
  void delay(int frame, double delay);
  void canvas(Fl_Widget *canvas, unsigned short flags = 0);
  Fl_Widget *canvas() const;
  int canvas_w() const;
  int canvas_h() const;
  bool is_animated() const;
  const char *name() const;
  void speed(double speed);
  double speed() const;

  // -- animation
  int frames() const;
  void frame(int frame);
  int frame() const;
  Fl_Image *image() const;
  Fl_Image *image(int frame) const;
  bool start();
  bool stop();
  bool next();

  /** Return if the animation is currently running or stopped.
   \return true if the animation is running
   */
  bool playing() const { return valid() && Fl::has_timeout(cb_animate, (void *)this); }

  // -- image data
  Fl_Anim_GIF_Image& resize(int w, int h);
  Fl_Anim_GIF_Image& resize(double scale);
  int frame_x(int frame) const;
  int frame_y(int frame) const;
  int frame_w(int frame) const;
  int frame_h(int frame) const;

  // -- overridden methods
  void color_average(Fl_Color c, float i) FL_OVERRIDE;
  Fl_Image *copy(int W, int H) const FL_OVERRIDE;
  Fl_Image *copy() const { return Fl_Pixmap::copy(); }
  void desaturate() FL_OVERRIDE;
  void draw(int x, int y, int w, int h, int cx = 0, int cy = 0) FL_OVERRIDE;
  void uncache() FL_OVERRIDE;

  // -- debugging and logging
  int debug() const;

  // -- static methods
  static int frame_count(const char *name, const unsigned char *imgdata = NULL, size_t imglength = 0);

  /**
   The loop flag can be used to (dis-)allow loop count.
   If set (which is the default), the animation will be
   stopped after the number of repeats specified in the
   GIF file (typically this count is set to 'forever' anyway).
   If cleared the animation will always be 'forever',
   regardless of what is specified in the GIF file.
   */
  static bool loop;

  /**
   The min_delay value can be used to set a minimum value
   for the frame delay for playback. This is to prevent
   CPU hogs caused by images with very low delay rates.
   This is a global value for all Fl_Anim_GIF_Image objects.
   */
  static double min_delay;

protected:

  bool next_frame();
  void clear_frames();
  void set_frame(int frame);

  static void cb_animate(void *d);
  void scale_frame();
  void set_frame();
  void on_frame_data(Fl_GIF_Image::GIF_FRAME &f) FL_OVERRIDE;
  void on_extension_data(Fl_GIF_Image::GIF_FRAME &f) FL_OVERRIDE;

private:

  char *name_;
  unsigned short flags_;
  Fl_Widget *canvas_;
  bool uncache_;
  bool valid_;
  int frame_; // current frame
  double speed_;
  FrameInfo *fi_;
};

#endif // Fl_Anim_Gif_Image_H
