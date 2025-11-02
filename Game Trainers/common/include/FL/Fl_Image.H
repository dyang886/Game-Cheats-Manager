//
// Image header file for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2022 by Bill Spitzak and others.
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
   Fl_Image, Fl_RGB_Image classes. */

#ifndef Fl_Image_H
#define Fl_Image_H

#include "Enumerations.H"
#include "Fl_Widget.H" // for fl_uintptr_t

class Fl_Widget;
class Fl_Pixmap;
struct Fl_Menu_Item;
struct Fl_Label;
class Fl_RGB_Image;


/** \enum Fl_RGB_Scaling
 The scaling algorithm to use for RGB images.
*/
enum Fl_RGB_Scaling {
  FL_RGB_SCALING_NEAREST = 0, ///< default RGB image scaling algorithm
  FL_RGB_SCALING_BILINEAR     ///< more accurate, but slower RGB image scaling algorithm
};


/**
  Base class for image caching, scaling and drawing.

  Fl_Image is the base class used for caching, scaling and drawing all kinds of
  images in FLTK. This class keeps track of common image data such as the pixels,
  colormap, width, height, and depth. Virtual methods are used to provide
  type-specific image handling.

  Each image possesses two (width, height) pairs:
   -# The width and height of the raw image data are returned by data_w() and
      data_h(). These values are set when the image is created and remain unchanged.
   -# The width and height of the area filled by the image when it gets drawn are
      returned by w() and h(). These values are equal to data_w() and data_h() when
      the image is created and can be changed by the scale() member function.

  Since the Fl_Image class does not support image drawing by itself, calling the
  Fl_Image::draw() method results in a box with an X in it being drawn instead.
*/
class FL_EXPORT Fl_Image {
  friend class Fl_Graphics_Driver;
public:
  static const int ERR_NO_IMAGE       = -1;
  static const int ERR_FILE_ACCESS    = -2;
  static const int ERR_FORMAT         = -3;
  static const int ERR_MEMORY_ACCESS  = -4;

private:
  int w_, h_, d_, ld_, count_;
  int data_w_, data_h_;
  const char * const *data_;
  static Fl_RGB_Scaling RGB_scaling_; // method used when copying RGB images
  static Fl_RGB_Scaling scaling_algorithm_; // method used to rescale RGB source images before drawing
  // Forbid use of copy constructor and assign operator
  Fl_Image & operator=(const Fl_Image &);
  Fl_Image(const Fl_Image &);
  // Presently redefined in Fl_SVG_Image
  virtual void cache_size_(int &/*width*/, int &/*height*/) {}

protected:

  /**
   Sets the width of the image data.
   This protected function sets both image widths: the width of the image data returned by data_w() and
   the image drawing width in FLTK units returned by w().
   */
  void w(int W) {w_ = W; data_w_ = W;}
  /**
   Sets the height of the image data.
   This protected function sets both image heights: the height of the image data returned by data_h() and
   the image drawing height in FLTK units returned by h().
   */
  void h(int H) {h_ = H; data_h_ = H;}
  /**
   Sets the current image depth.
   */
  void d(int D) {d_ = D;}
  /**
    Sets the current line data size in bytes.

    Color images may contain extra data (padding) that is included after every
    line of color image data and is normally not present.

    If \p LD is zero, then line data size is assumed to be data_w() * d() bytes.

    If \p LD is non-zero, then it must be positive and larger than data_w() * d()
    to account for the extra data per line.
   */
  void ld(int LD) {ld_ = LD;}
  /**
    Sets the current data pointer and count of pointers in the array.

    There can be 0, 1, or more pointers to actual image data in an image.

   \see const char* const* data(), count(), w(), h(), data_w(), data_h(), d(), ld()
  */
  void data(const char * const *p, int c) {data_ = p; count_ = c;}
  void draw_empty(int X, int Y);

  static void labeltype(const Fl_Label *lo, int lx, int ly, int lw, int lh, Fl_Align la);
  static void measure(const Fl_Label *lo, int &lw, int &lh);
  int draw_scaled(int X, int Y, int W, int H);

public:

  /**
   Returns the current image drawing width in FLTK units.
   The values of w() and data_w() are identical unless scale() has been called
   after which they may differ.
   */
  int w() const {return w_;}
  /**
   Returns the current image drawing height in FLTK units.
   The values of h() and data_h() are identical unless scale() has been called
   after which they may differ.
   */
  int h() const {return h_;}
  /**
   Returns the width of the image data.
   */
  int data_w() const {return data_w_;}
  /**
   Returns the height of the image data.
   */
  int data_h() const {return data_h_;}
  /**
   Returns the image depth.
   The return value will be 0 for bitmaps, 1 for
   pixmaps, and 1 to 4 for color images.</P>
   */
  int d() const {return d_;}
  /**
   Returns the current line data size in bytes.
   \see ld(int)
   */
  int ld() const {return ld_;}
  /**
    Returns the number of data values associated with the image.

    The value will be 0 for images with no associated data, 1 for
    bitmap and color images, and greater than 2 for pixmap images.

    \see data()
  */
  int count() const {return count_;}
  /**
    Returns a pointer to the current image data array.

    There can be 0, 1, or more pointers to actual image data in an image.

    Use the count() method to find the size of the data array. You must
    not dereference the data() pointer if count() equals zero.

    \note data() \b may return NULL.

    Example:

    Fl_RGB_Image has exactly one pointer which points at the R, G, B [, A]
    data array of the image. The total size of this array depends on
    several attributes like data_w(), data_h(), d() and ld() and is basically
    data_w() * data_h() * d() but there are exceptions if ld() is non-zero:
    see description of ld(). Since FLTK 1.4.0 w() and h() are no longer
    significant for the image data size if scale() has been called on the
    image to set a different display size.

    Other image types have different numbers and types of data pointers
    which are implementation details and not documented here.

    \see count(), w(), h(), data_w(), data_h(), d(), ld()
  */
  const char * const *data() const {return data_;}
  int fail() const;
  /**
    Releases an Fl_Image - the same as '\p delete \p this'.

    This virtual method is for almost all image classes the same as calling
    \code
      delete image;
    \endcode
    where image is an \p Fl_Image \p * pointer.

    However, for subclass Fl_Shared_Image and its subclasses this virtual
    method is reimplemented and maintains shared images.

    This virtual method makes it possible to \p destroy all image types in
    the same way by calling
    \code
      image->release();
    \endcode

    Reasoning: If you have an 'Fl_Image *' base class pointer and don't know
    if the object is one of the class Fl_Shared_Image or any other subclass
    of Fl_Image (for instance Fl_RGB_Image) then you can't just use operator
    delete since this is not appropriate for Fl_Shared_Image objects.

    The virtual method release() handles this properly.

    \since 1.4.0 in the base class Fl_Image and virtual in Fl_Shared_Image
  */
  virtual void release() {
    delete this;
  }

  /** Returns whether an image is an Fl_Shared_Image or not.

    This virtual method returns a pointer to an Fl_Shared_Image if this
    object is an instance of Fl_Shared_Image or NULL if not. This can be
    used to detect if a given Fl_Image object is a shared image, i.e.
    derived from Fl_Shared_Image.

    \since 1.4.0
  */
  virtual class Fl_Shared_Image *as_shared_image() {
    return 0;
  }

  Fl_Image(int W, int H, int D);
  virtual ~Fl_Image();
  virtual Fl_Image *copy(int W, int H) const;
  /**
    Creates a copy of the image in the same size.

    The new image should be released when you are done with it.

    This does exactly the same as 'Fl_Image::copy(int W, int H) const' where
    \p W and \p H are the width and height of the source image, respectively.
    This applies also to all subclasses of Fl_Image in the FLTK library.

    The following two copy() calls are equivalent:
    \code
      Fl_Image *img1 = new Fl_Image(...);
      // ...
      Fl_Image *img2 = img1->copy();
      Fl_Image *img3 = img1->copy(img1->w(), img1->h())
    \endcode

    For details see 'Fl_Image::copy(int w, int h) const'.
    \see Fl_Image::release()

    \note Since FLTK 1.4.0 this method is 'const'. If you derive your own class
      from Fl_Image or any subclass your overridden methods of 'Fl_Image::copy() const'
      and 'Fl_Image::copy(int, int) const' \b must also be 'const' for inheritance
      to work properly. This is different than in FLTK 1.3.x and earlier where these
      methods have not been 'const'.
  */
  Fl_Image *copy() const { return copy(w(), h()); }
  virtual void color_average(Fl_Color c, float i);
  /**
   The inactive() method calls
   color_average(\ref FL_BACKGROUND_COLOR, 0.33f) to produce
   an image that appears grayed out.

   An internal copy is made of the original image before
   changes are applied, to avoid modifying the original image.

   \note The RGB color of \ref FL_BACKGROUND_COLOR may change when the
   connection to the display is made. See fl_open_display().
   */
  void inactive() { color_average(FL_GRAY, .33f); }
  virtual void desaturate();
  virtual void label(Fl_Widget*w);
  virtual void label(Fl_Menu_Item*m);
  /**
   Draws the image to the current drawing surface with a bounding box.
   Arguments <tt>X,Y,W,H</tt> specify
   a bounding box for the image, with the origin
   (upper-left corner) of the image offset by the \c cx
   and \c cy arguments.

   In other words:  <tt>fl_push_clip(X,Y,W,H)</tt> is applied,
   the image is drawn with its upper-left corner at <tt>X-cx,Y-cy</tt> and its own width and height,
   <tt>fl_pop_clip</tt><tt>()</tt> is applied.
   */
  virtual void draw(int X, int Y, int W, int H, int cx=0, int cy=0); // platform dependent
  /**
   Draws the image to the current drawing surface.
   \param X, Y specify the upper-lefthand corner of the image.
   */
  void draw(int X, int Y) {draw(X, Y, w(), h(), 0, 0);} // platform dependent
  virtual void uncache();

  // used by fl_define_FL_IMAGE_LABEL() to avoid 'friend' declaration
  static Fl_Labeltype define_FL_IMAGE_LABEL();

  // set RGB image scaling method
  static void RGB_scaling(Fl_RGB_Scaling);
  // get RGB image scaling method
  static Fl_RGB_Scaling RGB_scaling();

  // set the image drawing size
  virtual void scale(int width, int height, int proportional = 1, int can_expand = 0);
  /** Sets what algorithm is used when resizing a source image to draw it.
   The default algorithm is FL_RGB_SCALING_BILINEAR.
   Drawing an Fl_Image is sometimes performed by first resizing the source image
   and then drawing the resized copy. This occurs, e.g., when drawing to screen under X11
   without Xrender support after having called scale().
   This function controls what method is used when the image to be resized is an Fl_RGB_Image.
   \version 1.4
   */
  static void scaling_algorithm(Fl_RGB_Scaling algorithm) {scaling_algorithm_ = algorithm; }
  /** Gets what algorithm is used when resizing a source image to draw it. */
  static Fl_RGB_Scaling scaling_algorithm() {return scaling_algorithm_;}
  static bool register_images_done;
};

class Fl_SVG_Image;

/**
  The Fl_RGB_Image class supports caching and drawing
  of full-color images with 1 to 4 channels of color information.
  Images with an even number of channels are assumed to contain
  alpha information, which is used to blend the image with the
  contents of the screen.

  Fl_RGB_Image is defined in
  &lt;FL/Fl_Image.H&gt;, however for compatibility reasons
  &lt;FL/Fl_RGB_Image.H&gt; should be included.
*/
class FL_EXPORT Fl_RGB_Image : public Fl_Image {
  friend class Fl_Graphics_Driver;
  static size_t max_size_;
public:

  /** Points to the start of the object's data array
   \see class Fl_SVG_Image which delays initialization of this member variable.
   */
  const uchar *array;
  /** If non-zero, the object's data array is delete[]'d when deleting the object.
   */
  int alloc_array;

private:
  // These two variables are used to cache the image and mask for the main display graphics driver
  fl_uintptr_t id_;
  fl_uintptr_t mask_;
  int cache_w_, cache_h_; // size of image when cached
public:

  Fl_RGB_Image(const uchar *bits, int W, int H, int D=3, int LD=0);
  Fl_RGB_Image(const uchar *bits, int bits_length, int W, int H, int D, int LD);
  Fl_RGB_Image(const Fl_Pixmap *pxm, Fl_Color bg=FL_GRAY);
  ~Fl_RGB_Image() FL_OVERRIDE;
  Fl_Image *copy(int W, int H) const FL_OVERRIDE;
  Fl_Image *copy() const { return Fl_Image::copy(); }
  void color_average(Fl_Color c, float i) FL_OVERRIDE;
  void desaturate() FL_OVERRIDE;
  void draw(int X, int Y, int W, int H, int cx=0, int cy=0) FL_OVERRIDE;
  void draw(int X, int Y) {draw(X, Y, w(), h(), 0, 0);}
  void label(Fl_Widget*w) FL_OVERRIDE;
  void label(Fl_Menu_Item*m) FL_OVERRIDE;
  void uncache() FL_OVERRIDE;
  int cache_w() {return cache_w_;}
  int cache_h() {return cache_h_;}
  /** Sets the maximum allowed image size in bytes when creating an Fl_RGB_Image object.

   The image size in bytes of an Fl_RGB_Image object is the value of the product w() * h() * d().
   If this product exceeds size, the created object of a derived class of Fl_RGB_Image
   won't be loaded with the image data.
   This does not apply to direct RGB image creation with
   Fl_RGB_Image::Fl_RGB_Image(const uchar *bits, int W, int H, int D, int LD).
   The default max_size() value is essentially infinite.
   */
  static void max_size(size_t size) { max_size_ = size;}
  /** Returns the maximum allowed image size in bytes when creating an Fl_RGB_Image object.

   \sa  void Fl_RGB_Image::max_size(size_t)
   */
  static size_t max_size() {return max_size_;}
  /** Returns whether an image is an Fl_SVG_Image or not.
  This virtual method returns a pointer to the Fl_SVG_Image if this object is an instance of Fl_SVG_Image or NULL if not. */
  virtual Fl_SVG_Image *as_svg_image() { return NULL; }
  /** Makes sure the object is fully initialized.
   In particular, makes sure member variable \ref array is non-null. */
  virtual void normalize() {}
};

#endif // !Fl_Image_H
