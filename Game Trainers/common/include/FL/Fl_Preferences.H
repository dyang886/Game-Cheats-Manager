//
// Preferences implementation for the Fast Light Tool Kit (FLTK).
//
// Copyright 2002-2023 by Matthias Melcher.
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
   Fl_Preferences class . */

#ifndef Fl_Preferences_H
#  define Fl_Preferences_H

#  include <stdio.h>
#  include "Fl_Export.H"
#  include "fl_attr.h"

//class Fl_String;
#if (FLTK_USE_STD)
#include <string>
#endif

/**
 \brief Fl_Preferences store user settings between application starts.

 FLTK Preferences are similar to the Registry on Windows and Preferences on
 MacOS, providing a simple method to store customizable user settings between
 application launches. A typical use is storing the last window position or a
 history of previously used documents.

 Preferences are organized in a hierarchy of groups. Every group can contain
 more groups and any number of key/value pairs. Keys can be text strings
 containing ASCII letters, digits, periods, and underscores. Forward slashes
 in a key name are treated as subgroups, i.e. the key 'window/width' would
 actually refer to the key 'width' inside the group 'window'.

 Keys have a unique name within their group. A value can be any string including
 control characters 0x00 to 0x1f, 0x7f, and UTF-8 octets.

 Several methods allow setting and getting numerical values and binary data.

 Preferences files are the same across platforms. User comments in preference
 files are preserved. Filenames are unique for each application by using a
 vendor/application naming scheme. The developer app must provide default values
 for all entries to ensure proper operation should preferences be corrupted
 or not yet exist.

 \note The format of preferences files is not part of the FLTK specification
 and intentionally undocumented. The only valid way to read or write prefs
 files is via the API from your app. The fact that the current
 implementation looks like human-readable text is purely coincidental and
 may change at any time. Preferences files are not meant to be created
 or edited "by hand."

 FLTK preferences are not meant to replace a fully featured database. No merging
 of data takes place. If several instances of an app access the same database at
 the same time, only the most recent changes will persist.

 Preferences should not be used to store document data. The .prefs file should
 be kept small for performance reasons. One application can have multiple
 preferences files. Extensive binary data however should be stored in separate
 files: see \a Fl_Preferences::get_userdata_path() .

 Fl_Preferences are not thread-safe. They can temporarily change the locale
 on some platforms during read and write access, which also changes it
 temporarily in other threads of the same app.

 Typically a preferences database is read at startup, and then reopened and
 written at app shutdown:
 ```
 int appWindowWidth, appWindowHeight;

 void launch() {
   Fl_Preferences app(Fl_Preferences::USER_L, "matthiasm.com", "hello");
   // 'app' constructor will be called, reading data from .prefs file
   Fl_Preferences window(app, "window");
   window.get("width", appWindowWidth, 800);
   window.get("height", appWindowHeight, 600);
   // 'app' destructor will be called. This will write data to the
   // .prefs file if any preferences were changed or added
 }

 void quit() {
   Fl_Preferences app(Fl_Preferences::USER_L, "matthiasm.com", "hello");
   Fl_Preferences window(app, "window");
   window.set("width", appWindowWidth);
   window.set("height", appWindowHeight);
 }
 ```

 \see Fl_Preferences::Fl_Preferences(Root root, const char *vendor, const char *application)

 \see As a special case, Fl_Preferences can be memory mapped and not be associated
 with a file on disk. See
 Fl_Preferences::Fl_Preferences(Fl_Preferences *parent, const char *group)
 and Fl_Preferences::MEMORY for more details on memory mapped preferences.

   \note Starting with FLTK 1.3, preferences databases are expected to
     be in UTF-8 encoding. Previous databases were stored in the
     current character set or code page which renders them incompatible
     for text entries using international characters.

   \note Starting with FLTK 1.4, searching a valid path to store
     the preference files has changed slightly. Please see
     Fl_Preferences::Fl_Preferences(Root, const char*, const char*)
     for details.

   \note Starting with FLTK 1.4, preference files should be created with
     `SYSTEM_L` or `USER_L` to be interchangeable between computers with
     differing locale settings. The legacy modes, `LOCAL` and `SYSTEM`, will
     read and write floating point values using the decimal point of the
     current locale. As a result, a fp-value would be written '3,1415' on a
     German machine, and would be read back as '3.0' on a US machine because
     the comma would not be recognized as an alternative decimal point.
 */
class FL_EXPORT Fl_Preferences {

public:
  /**
     Define the scope of the preferences.
   */
  enum Root {
    UNKNOWN_ROOT_TYPE = -1,            ///< Returned if storage could not be determined.
    SYSTEM            =  0,            ///< Preferences are used system-wide. Deprecated, see SYSTEM_L
    USER,                              ///< Preferences apply only to the current user. Deprecated, see USER_L
    MEMORY,                            ///< Returned if querying memory mapped preferences
    ROOT_MASK     = 0x00FF,            ///< Mask for the values above
    CORE          = 0x0100,            ///< OR'd by FLTK to read and write core library preferences and options
    C_LOCALE      = 0x1000,            ///< This flag should always be set to ensure that floating point values
                                       ///< are written and read correctly independently of the current locale
    CLEAR         = 0x2000,            ///< Don't read a possibly existing database. Instead, start with an empty set of preferences.
    SYSTEM_L      = SYSTEM | C_LOCALE, ///< Preferences are used system-wide, locale independent
    USER_L        = USER | C_LOCALE,   ///< Preferences apply only to the current user, locale independent
    CORE_SYSTEM_L = CORE | SYSTEM_L,   ///< Same as CORE | SYSTEM | C_LOCALE
    CORE_USER_L   = CORE | USER_L,     ///< Same as CORE | USER | C_LOCALE
    CORE_SYSTEM   = CORE | SYSTEM,     ///< Deprecated, same as CORE | SYSTEM. Use CORE_SYSTEM_L instead.
    CORE_USER     = CORE | USER        ///< Deprecated, same as CORE | USER. Use CORE_USER_L instead.
  };

  /**
   Every Fl_Preferences-Group has a unique ID.

   ID's can be retrieved from an Fl_Preferences-Group and can then be used
   to create more Fl_Preference references to the same data set, as long as the
   database remains open.
   */
  typedef void *ID;

  static const char *new_UUID();

  /** Set this if no call to Fl_Preferences shall access the file system.
   @see Fl_Preferences::file_access(unsigned int)
   @see Fl_Preferences::file_access()
   */
  static const unsigned int NONE = 0x0000;
  /** Set this if it is OK for applications to read user preference files. */
  static const unsigned int USER_READ_OK = 0x0001;
  /** Set this if it is OK for applications to create and write user preference files. */
  static const unsigned int USER_WRITE_OK = 0x0002;
  /** Set this if it is OK for applications to read, create, and write user preference files. */
  static const unsigned int USER_OK = USER_READ_OK | USER_WRITE_OK;
  /** Set this if it is OK for applications to read system wide preference files. */
  static const unsigned int SYSTEM_READ_OK = 0x0004;
  /** Set this if it is OK for applications to create and write system wide preference files. */
  static const unsigned int SYSTEM_WRITE_OK = 0x0008;
  /** Set this if it is OK for applications to read, create, and write system wide preference files. */
  static const unsigned int SYSTEM_OK = SYSTEM_READ_OK | SYSTEM_WRITE_OK;
  /** Set this if it is OK for applications to read, create, and write any kind of preference files. */
  static const unsigned int APP_OK = SYSTEM_OK | USER_OK;
  /** Set this if it is OK for FLTK to read preference files. USER_READ_OK and/or SYSTEM_READ_OK must also be set. */
  static const unsigned int CORE_READ_OK = 0x0010;
  /** Set this if it is OK for FLTK to create or write preference files. USER_WRITE_OK and/or SYSTEM_WRITE_OK must also be set. */
  static const unsigned int CORE_WRITE_OK = 0x0020;
  /** Set this if it is OK for FLTK to read, create, or write preference files. */
  static const unsigned int CORE_OK = CORE_READ_OK | CORE_WRITE_OK;
  /** Set this to allow FLTK and applications to read preference files. */
  static const unsigned int ALL_READ_OK = USER_READ_OK | SYSTEM_READ_OK | CORE_READ_OK;
  /** Set this to allow FLTK and applications to create and write preference files. */
  static const unsigned int ALL_WRITE_OK = USER_WRITE_OK | SYSTEM_WRITE_OK | CORE_WRITE_OK;
  /** Set this to give FLTK and applications permission to read, write, and create preference files. */
  static const unsigned int ALL = ALL_READ_OK | ALL_WRITE_OK;

  static void file_access(unsigned int flags);
  static unsigned int file_access();
  static Root filename( char *buffer, size_t buffer_size, Root root, const char *vendor, const char *application );

  Fl_Preferences( Root root, const char *vendor, const char *application );
  Fl_Preferences( const char *path, const char *vendor, const char *application, Root flags );
  Fl_Preferences( Fl_Preferences &parent, const char *group );
  Fl_Preferences( Fl_Preferences *parent, const char *group );
  Fl_Preferences( Fl_Preferences &parent, int groupIndex );
  Fl_Preferences( Fl_Preferences *parent, int groupIndex );
  Fl_Preferences(const Fl_Preferences&);
  Fl_Preferences( ID id );
  virtual ~Fl_Preferences();

  FL_DEPRECATED("since 1.4.0 - use Fl_Preferences(path, vendor, application, flags) instead",
                Fl_Preferences( const char *path, const char *vendor, const char *application ) );

  Root filename( char *buffer, size_t buffer_size);

  /** Return an ID that can later be reused to open more references to this dataset.
   */
  ID id() { return (ID)node; }

  /** Remove the group with this ID from a database.
   */
  static char remove(ID id_) { return ((Node*)id_)->remove(); }

  /** Return the name of this entry.
   */
  const char *name() { return node->name(); }

  /** Return the full path to this entry.
   */
  const char *path() { return node->path(); }

  int groups();
  const char *group( int num_group );
  char group_exists( const char *key );
  char delete_group( const char *group );
  char delete_all_groups();

  int entries();
  const char *entry( int index );
  char entry_exists( const char *key );
  char delete_entry( const char *entry );
  char delete_all_entries();

  char clear();

  char set( const char *entry, int value );
  char set( const char *entry, float value );
  char set( const char *entry, float value, int precision );
  char set( const char *entry, double value );
  char set( const char *entry, double value, int precision );
  char set( const char *entry, const char *value );
  char set( const char *entry, const void *value, int size );

  char get( const char *entry, int &value, int defaultValue );
  char get( const char *entry, float &value,  float defaultValue );
  char get( const char *entry, double &value, double defaultValue );
  char get( const char *entry, char *&value,  const char *defaultValue );
  char get( const char *entry, char *value,   const char *defaultValue, int maxSize );
  char get( const char *entry, void *&value,  const void *defaultValue, int defaultSize );
  char get( const char *entry, void *value,   const void *defaultValue, int defaultSize, int maxSize );
  char get( const char *entry, void *value,   const void *defaultValue, int defaultSize, int *size );

//  char set( const char *entry, const Fl_String &value );
//  char get( const char *entry, Fl_String &value, const Fl_String &defaultValue );

#if (FLTK_USE_STD)
  char set( const char *entry, const std::string &value );
  char get( const char *entry, std::string &value, const std::string &defaultValue );
#endif

  int size( const char *entry );

  char get_userdata_path( char *path, int pathlen );

  int flush();

  int dirty();

  /** \cond PRIVATE */
  static const char *newUUID() { return new_UUID(); }
  char groupExists( const char *key ) { return group_exists(key); }
  char deleteGroup( const char *group ) { return delete_group(group); }
  char deleteAllGroups() { return delete_all_groups(); }
  char entryExists( const char *key ) { return entry_exists(key); }
  char deleteEntry( const char *entry ) { return delete_entry(entry); }
  char deleteAllEntries() { return delete_all_entries(); }
  char getUserdataPath( char *path, int pathlen ) { return get_userdata_path(path, pathlen); }
  /** \endcond */

  /**
     'Name' provides a simple method to create numerical or more complex
     procedural names for entries and groups on the fly.

     Example: prefs.set(Fl_Preferences::Name("File%d",i),file[i]);.

     See test/preferences.cxx as a sample for writing arrays into preferences.

     'Name' is actually implemented as a class inside Fl_Preferences. It casts
     into const char* and gets automatically destroyed after the enclosing call
     ends.
   */
  class FL_EXPORT Name {

    char *data_;

  public:
    Name( unsigned int n );
    Name( const char *format, ... );

    /**
       Return the Name as a "C" string.
       \internal
     */
    operator const char *() { return data_; }
    ~Name();
  };

  /** \internal An entry associates a preference name to its corresponding value */
  struct Entry {
    char *name, *value;
  };

private:
  Fl_Preferences() : node(0), rootNode(0) { }
  Fl_Preferences &operator=(const Fl_Preferences&);

  static char nameBuffer[128];
  static char uuidBuffer[40];
  static Fl_Preferences *runtimePrefs;
  static unsigned int fileAccess_;

public:  // older Sun compilers need this (public definition of the following classes)
  class RootNode;

  class FL_EXPORT Node {        // a node contains a list to all its entries
                                // and all means to manage the tree structure
    Node *first_child_, *next_;
    union {                     // these two are mutually exclusive
      Node *parent_;            // top_ bit clear
      RootNode *root_node_;     // top_ bit set
    };
    char *path_;
    Entry *entry_;
    int nEntry_, NEntry_;
    unsigned char dirty_:1;
    unsigned char top_:1;
    unsigned char indexed_:1;
    // indexing routines
    Node **index_;
    int nIndex_, NIndex_;
    void createIndex();
    void updateIndex();
    void deleteIndex();
  public:
    static int lastEntrySet;
  public:
    Node( const char *path );
    ~Node();
    // node methods
    int write( FILE *f );
    const char *name();
    const char *path() { return path_; }
    Node *find( const char *path );
    Node *search( const char *path, int offset=0 );
    Node *childNode( int ix );
    Node *addChild( const char *path );
    void setParent( Node *parent );
    Node *parent() { return top_?0L:parent_; }
    void setRoot(RootNode *r) { root_node_ = r; top_ = 1; }
    RootNode *findRoot();
    char remove();
    char dirty();
    void clearDirtyFlags();
    void deleteAllChildren();
    // entry methods
    int nChildren();
    const char *child( int ix );
    void set( const char *name, const char *value );
    void set( const char *line );
    void add( const char *line );
    const char *get( const char *name );
    int getEntry( const char *name );
    char deleteEntry( const char *name );
    void deleteAllEntries();
    int nEntry() { return nEntry_; }
    Entry &entry(int i) { return entry_[i]; }
  };
  friend class Node;

  class FL_EXPORT RootNode {    // the root node manages file paths and basic reading and writing
    Fl_Preferences *prefs_;
    char *filename_;
    char *vendor_, *application_;
    Root root_type_;
  public:
    RootNode( Fl_Preferences *, Root root, const char *vendor, const char *application );
    RootNode( Fl_Preferences *, const char *path, const char *vendor, const char *application, Root flags );
    RootNode( Fl_Preferences * );
    ~RootNode();
    int read();
    int write();
    char getPath( char *path, int pathlen );
    char *filename() { return filename_; }
    Root root() { return root_type_; }
  };
  friend class RootNode;

protected:
  Node *node;
  RootNode *rootNode;
};

#endif // !Fl_Preferences_H
