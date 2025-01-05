//
// FLTK native OS file chooser widget
//
// Copyright 2004 Greg Ercolano.
// Copyright 2005-2024 by Bill Spitzak and others.
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
   Fl_Native_File_Chooser widget. */

/* Implementation note:

 class Fl_Native_File_Chooser  <== public API used by applications

 class Fl_Native_File_Chooser_Driver   <== virtual API that a platform may implement
                                           this API has a do-nothing default implementation

        class Fl_Native_File_Chooser_FLTK_Driver   <== this API implementation is the default FLTK file chooser
                class Fl_GTK_Native_File_Chooser_Driver  <== this API implementation runs a GTK file chooser
                class Fl_Kdialog_Native_File_Chooser_Driver  <== this API implementation runs a KDE file chooser
                                        it is determined at run-time if the GTK dynamic libraries are available
                                        and the KDE file chooser runs under the KDE desktop

        class Fl_Quartz_Native_File_Chooser_Driver  <== this API implementation runs a Mac OS X file chooser

        class Fl_WinAPI_Native_File_Chooser_Driver  <== this API implementation runs a Windows file chooser


 Each platform must implement the constructor of the Fl_Native_File_Chooser class.
 This particular implementation:

 Fl_Native_File_Chooser::Fl_Native_File_Chooser(int val) {
  platform_fnfc = new Fl_Native_File_Chooser_FLTK_Driver(val);
 }

 can be used by any platform.
 No more code is required. The cross-platform Fl_Native_File_Chooser_FLTK.cxx file must be compiled in libfltk,
 and the default FLTK file chooser will be used.

 This other implementation:
 Fl_Native_File_Chooser::Fl_Native_File_Chooser(int val) {
   platform_fnfc = 0;
 }
 can be used by a platform that needs no file chooser.
 */

#ifndef FL_NATIVE_FILE_CHOOSER_H
#define FL_NATIVE_FILE_CHOOSER_H

#include <FL/Fl_Export.H>
#include <FL/Fl_File_Chooser.H>

class Fl_Native_File_Chooser_Driver;

/**
 This class lets an FLTK application easily and consistently access
 the operating system's native file chooser. Some operating systems
 have very complex and specific file choosers that many users want
 access to specifically, instead of FLTK's default file chooser(s).

 In cases where there is no native file browser, FLTK's own file browser
 is used instead.

 To use this widget, use the following include in your code:
 \code
 #include <FL/Fl_Native_File_Chooser.H>
 \endcode

 The following example shows how to pick a single file:
 \code
 // Create and post the local native file chooser
 #include <FL/Fl_Native_File_Chooser.H>
 [..]
 Fl_Native_File_Chooser fnfc;
 fnfc.title("Pick a file");
 fnfc.type(Fl_Native_File_Chooser::BROWSE_FILE);
 fnfc.filter("Text\t*.txt\n"
             "C Files\t*.{cxx,h,c}");
 fnfc.directory("/var/tmp");           // default directory to use
 // Show native chooser
 switch ( fnfc.show() ) {
   case -1: printf("ERROR: %s\n", fnfc.errmsg());    break;  // ERROR
   case  1: printf("CANCEL\n");                      break;  // CANCEL
   default: printf("PICKED: %s\n", fnfc.filename()); break;  // FILE CHOSEN
 }
 \endcode

 The Fl_Native_File_Chooser widget transmits UTF-8 encoded filenames to its user. It is
 recommended to open files that may have non-ASCII names with the fl_fopen() or
 fl_open() utility functions that handle these names in a cross-platform way
 (whereas the standard fopen()/open() functions fail on the Windows platform
 to open files with a non-ASCII name).

 <B>Platform Specific Caveats</B>

 - Under X11/Wayland the dialog is chosen as follows:
 -# If command \p zenity is available at run-time and if \p Fl::option(OPTION_FNFC_USES_ZENITY) is
 turned on, the \p zenity -based dialog opens. This is expected to be more appropriate
 than other dialog forms for sandboxed apps, but member function filter_value() is not effective.
 -# Else if the app runs under the KDE desktop
 and if \p Fl::option(OPTION_FNFC_USES_KDIALOG) is turned on,
 and if command \p kdialog is available at run-time, the \p kdialog -based dialog opens.
 Member function filter_value() is not effective with this dialog.
 -# Else if the GTK library is available at run-time on the computer and if \p Fl::option(OPTION_FNFC_USES_GTK) is
 not turned off, the GTK-styled dialog opens. Call fl_register_images() to add  a "Preview" button to this dialog.
 Use the static public attributes of class Fl_File_Chooser to localize the browser.
 -# Otherwise, FLTK's own dialog produced by the Fl_File_Chooser class opens.
 Call fl_register_images() to add  a "Preview" button to it.
 It's best if you also call Fl_File_Icon::load_system_icons()
 at the start of main(), to enable the nicer looking file browser widgets.
 Use the static public attributes of class Fl_File_Chooser to localize the browser.

 - Some operating systems support certain OS specific options; see
 Fl_Native_File_Chooser::options() for a list.

 \image html Fl_Native_File_Chooser.png "The Fl_Native_File_Chooser on different platforms."
 \image latex Fl_Native_File_Chooser.png "The Fl_Native_File_Chooser on different platforms" width=14cm

 */
class FL_EXPORT Fl_Native_File_Chooser {
private:
  Fl_Native_File_Chooser_Driver *platform_fnfc;
public:
  enum Type {
    BROWSE_FILE = 0,                    ///< browse files (lets user choose one file)
    BROWSE_DIRECTORY,                   ///< browse directories (lets user choose one directory)
    BROWSE_MULTI_FILE,                  ///< browse files (lets user choose multiple files)
    BROWSE_MULTI_DIRECTORY,             ///< browse directories (lets user choose multiple directories)
    BROWSE_SAVE_FILE,                   ///< browse to save a file
    BROWSE_SAVE_DIRECTORY               ///< browse to save a directory
  };
  enum Option {
    NO_OPTIONS     = 0x0000,            ///< no options enabled
    SAVEAS_CONFIRM = 0x0001,            ///< Show native 'Save As' overwrite confirm dialog
    NEW_FOLDER     = 0x0002,            ///< Show 'New Folder' icon (if supported)
    PREVIEW        = 0x0004,            ///< enable preview mode (if supported)
    USE_FILTER_EXT = 0x0008             ///< Chooser filter presets the output file extension (if supported)
  };
  static const char *file_exists_message;

  /**
   The constructor. Internally allocates the native widgets.
   Optional \p val presets the type of browser this will be,
   which can also be changed with type().
   */
  Fl_Native_File_Chooser(int val = BROWSE_FILE); // each platorm implements it
  ~Fl_Native_File_Chooser();
  void type(int t);
  int type() const ;
  void options(int o);
  int options() const;
  int count() const;
  const char *filename() const ;
  const char *filename(int i) const ;
  void directory(const char *val) ;
  const char *directory() const;
  void title(const char *t);
  const char* title() const;
  const char *filter() const ;
  void filter(const char *f);
  int filters() const ;
  void filter_value(int i) ;
  int filter_value() const ;
  void preset_file(const char*f) ;
  const char* preset_file() const;
  const char *errmsg() const ;
  int show() ;
  };

/**
 \cond DriverDev
 \addtogroup DriverDeveloper
 \{
 */

/** Represents the interface between FLTK and a native file chooser.
 This class is only for internal use by the FLTK library.
 A platform that wants to provide a native file chooser implements all virtual methods
 of this class. Each platform supported by FLTK must also implement the constructor of the
 Fl_Native_File_Chooser class.
 */
class Fl_Native_File_Chooser_Driver {
protected:
  static void chrcat(char *s, char c);
  static char *strapp(char *s, const char *val);
  static char *strfree(char *val);
  static char *strnew(const char *val);
public:
  Fl_Native_File_Chooser_Driver(int) {}
  virtual ~Fl_Native_File_Chooser_Driver() {}
  virtual void type(int) {}
  virtual int type() const  {return 0;}
  virtual void options(int) {}
  virtual int options() const {return 0;}
  virtual int count() const {return 0;}
  virtual const char *filename() const  {return 0;}
  virtual const char *filename(int) const  {return 0;}
  virtual void directory(const char *)  {}
  virtual const char *directory() const {return 0;}
  virtual void title(const char *) {}
  virtual const char* title() const {return 0;}
  virtual const char *filter() const  {return 0;}
  virtual void filter(const char *) {}
  virtual int filters() const  {return 0;}
  virtual void filter_value(int)  {}
  virtual int filter_value() const  {return 0;}
  virtual void preset_file(const char*)  {}
  virtual const char* preset_file() const {return 0;}
  virtual const char *errmsg() const  {return 0;}
  virtual int show()  {return 1;}
};

/** A cross-platform implementation of Fl_Native_File_Chooser_Driver.
 This implementation uses a Fl_File_Chooser object as file chooser.

 Any platform can support the Fl_Native_File_Chooser class by implementing
 its constructor as follows:
 \code
 Fl_Native_File_Chooser::Fl_Native_File_Chooser(int type) {
   platform_fnfc = new Fl_Native_File_Chooser_FLTK_Driver(type);
 }
\endcode
 */
class Fl_Native_File_Chooser_FLTK_Driver : public Fl_Native_File_Chooser_Driver {
private:
  void errmsg(const char *msg);
  int type_fl_file(int val);
  int exist_dialog();
  void parse_filter();
protected:
  int   _btype;                 // kind-of browser to show()
  int   _options;               // general options
  int   _nfilters;
  char *_filter;                // user supplied filter
  char *_parsedfilt;            // parsed filter
  int   _filtvalue;             // selected filter
  char *_preset_file;
  char *_prevvalue;             // Returned filename
  char *_directory;
  char *_errmsg;                // error message
  Fl_File_Chooser *_file_chooser;
public:
  Fl_Native_File_Chooser_FLTK_Driver(int val);
  virtual ~Fl_Native_File_Chooser_FLTK_Driver();
  void type(int t) FL_OVERRIDE;
  int type() const FL_OVERRIDE;
  void options(int o) FL_OVERRIDE;
  int options() const FL_OVERRIDE;
  int count() const FL_OVERRIDE;
  const char *filename() const FL_OVERRIDE;
  const char *filename(int i) const FL_OVERRIDE;
  void directory(const char *val) FL_OVERRIDE;
  const char *directory() const FL_OVERRIDE;
  void title(const char *t) FL_OVERRIDE;
  const char* title() const FL_OVERRIDE;
  const char *filter() const FL_OVERRIDE;
  void filter(const char *f) FL_OVERRIDE;
  int filters() const FL_OVERRIDE;
  void filter_value(int i) FL_OVERRIDE;
  int filter_value() const FL_OVERRIDE;
  void preset_file(const char*f) FL_OVERRIDE;
  const char* preset_file() const FL_OVERRIDE;
  const char *errmsg() const FL_OVERRIDE;
  int show() FL_OVERRIDE;
};

/**
 \}
 \endcond
 */


#endif /*FL_NATIVE_FILE_CHOOSER_H*/
