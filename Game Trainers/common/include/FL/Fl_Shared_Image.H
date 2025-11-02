//
// Shared image header file for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2024 by Bill Spitzak and others.
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

/** \file
   Fl_Shared_Image class. */

#ifndef Fl_Shared_Image_H
#  define Fl_Shared_Image_H

#  include "Fl_Image.H"

#undef SHIM_DEBUG

/** Test function (typedef) for adding new shared image formats.

  This defines the function type you can use to add a handler for unknown
  image formats that can be opened and loaded as an Fl_Shared_Image.

  fl_register_images() adds all image formats known to FLTK.
  Call Fl_Shared_Image::add_handler() to add your own check function to
  the list of known image formats.

  Your function will be passed the filename (\p name), some \p header
  bytes already read from the image file and the size \p headerlen of the
  data read. The max value of size is implementation dependent. If your
  handler function needs to check more bytes you must open the image file
  yourself.

  The provided buffer \p header must not be overwritten.

  If your handler function can identify the file type you must open the
  file and return a valid Fl_Image or derived type, otherwise you must
  return \c NULL.
  Example:
  \code
    static Fl_Image *check_my_image(const char *name,
                                    uchar *header,
                                    int headerlen) {
      // (test image type using header and headerlen)
      if (known) {
        // (load image data from file \p name)
        return new Fl_RGB_Image(data, ...);
      } else
        return 0;
    }
    // add your handler:
    Fl_Shared_Image::add_handler(check_my_image);
  \endcode

  \param[in]    name        filename to be checked and opened if applicable
  \param[in]    header      portion of the file that has already been read
  \param[in]    headerlen   length of provided \p header data

  \returns      valid Fl_Image or \c NULL.

  \see Fl_Shared_Image::add_handler()
*/
typedef Fl_Image *(*Fl_Shared_Handler)(const char *name,
                                       uchar *header,
                                       int headerlen);

/**
  This class supports caching, loading, and drawing of image files.

  Most applications will also want to link against the fltk_images library
  and call the fl_register_images() function to support standard image
  formats such as BMP, GIF, JPEG, PNG, and SVG (unless the library was built
  with the option removing SVG support).

  Images can be requested (loaded) with Fl_Shared_Image::get(), find(),
  and some other methods. All images are cached in an internal list of
  shared images and should be released when they are no longer needed.
  A refcount is used to determine if a released image is to be destroyed
  with delete.

  \see fl_register_image()
  \see Fl_Shared_Image::get()
  \see Fl_Shared_Image::find()
  \see Fl_Shared_Image::release()
*/
class FL_EXPORT Fl_Shared_Image : public Fl_Image {

  friend class Fl_JPEG_Image;
  friend class Fl_PNG_Image;
  friend class Fl_SVG_Image;
  friend class Fl_Graphics_Driver;

protected:

  static Fl_Shared_Image **images_;     // Shared images
  static int    num_images_;            // Number of shared images
  static int    alloc_images_;          // Allocated shared images
  static Fl_Shared_Handler *handlers_;  // Additional format handlers
  static int    num_handlers_;          // Number of format handlers
  static int    alloc_handlers_;        // Allocated format handlers

  const char    *name_;                 // Name of image file
  int           original_;              // Original image?
  int           refcount_;              // Number of times this image has been used
  Fl_Image      *image_;                // The image that is shared
  int           alloc_image_;           // Was the image allocated?

  static int    compare(Fl_Shared_Image **i0, Fl_Shared_Image **i1);

  // Use get() and release() to load/delete images in memory...
  Fl_Shared_Image();
  Fl_Shared_Image(const char *n, Fl_Image *img = 0);
  virtual ~Fl_Shared_Image();
  void add();
  void update();
  Fl_Shared_Image *copy_(int W, int H) const;

public:
#ifdef SHIM_DEBUG
  static void print_pool();
#endif

  /** Returns the filename of the shared image */
  const char    *name() { return name_; }

  /** Returns the number of references of this shared image.
    When reference is below 1, the image is deleted.
  */
  int           refcount() { return refcount_; }

  /** Returns whether this is an original image.
    Images loaded from a file or from memory are marked \p original as
    opposed to images created as a copy of another image with different
    size (width or height).
    \note This is useful for debugging (rarely used in user code).
    \since FLTK 1.4.0
  */
  int original() { return original_; }

  void  release() FL_OVERRIDE;
  virtual void  reload();

  Fl_Shared_Image *as_shared_image() FL_OVERRIDE {
    return this;
  }

  Fl_Image *copy(int W, int H) const FL_OVERRIDE;
  Fl_Image *copy() const;
  Fl_Image *copy();

  void color_average(Fl_Color c, float i) FL_OVERRIDE;
  void desaturate() FL_OVERRIDE;
  void draw(int X, int Y, int W, int H, int cx = 0, int cy = 0) FL_OVERRIDE;
  void draw(int X, int Y) { draw(X, Y, w(), h(), 0, 0); }
  void uncache() FL_OVERRIDE;

  static Fl_Shared_Image *find(const char *name, int W = 0, int H = 0);
  static Fl_Shared_Image *get(const char *name, int W = 0, int H = 0);
  static Fl_Shared_Image *get(Fl_RGB_Image *rgb, int own_it = 1);
  static Fl_Shared_Image **images();
  static int            num_images();
  static void           add_handler(Fl_Shared_Handler f);
  static void           remove_handler(Fl_Shared_Handler f);

  /**
    Returns a pointer to the internal Fl_Image object.

    The output is a pointer to the \p internal image ('Fl_Image' or subclass)
    which can be used to inspect or copy the image.

    <b>Do not try to modify the image!</b> You can copy the image though
    if you want or need to change any attributes, size etc. If all you
    need to do is to resize the image you should use
    Fl_Shared_Image::copy(int, int) instead.

    \note The internal image (pointer) is protected for good reasons, e.g.
      to prevent access to the image so it can't be modified by user code.
      \b DO \b NOT cast away the 'const' attribute to modify the image.

    User code should rarely need this method. Use with caution.

    \return  const Fl_Image* image, the internal Fl_Image

    \since 1.4.0
  */
  const Fl_Image *image() const { return image_; }

}; // class Fl_Shared_Image

//
// The following function is provided in the fltk_images library and
// registers all of the "extra" image file formats that are not part
// of the core FLTK library...
//

FL_EXPORT extern void fl_register_images();

#endif // !Fl_Shared_Image_H
