//
// MacOS system menu bar header file for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2017 by Bill Spitzak and others.
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
 Definition of class Fl_Sys_Menu_Bar.
 */


#ifndef Fl_Sys_Menu_Bar_H
#define Fl_Sys_Menu_Bar_H

#include <FL/Fl_Menu_Bar.H>

class Fl_Sys_Menu_Bar_Driver;

/**
 A class to create and modify menus that appear on macOS in the menu bar at the top of the screen.

 On other than macOS platforms, Fl_Sys_Menu_Bar is a synonym of class Fl_Menu_Bar.

 On the macOS  platform, replace Fl_Menu_Bar with Fl_Sys_Menu_Bar, and
 a system menu at the top of the screen will be available. This menu will match an array
 of Fl_Menu_Item's exactly as in all other FLTK menus (except for the submenu with the
 application's own name and the 'Window' menu; see below). There is, though, an important difference between
 an Fl_Sys_Menu_Bar object under macOS and under other platforms: only a single object
 from this class can be created, because macOS uses a single system menu bar. Therefore,
 porting to macOS an app that creates, on other platforms,  several Fl_Menu_Bar objects, one for each of several windows,
 is more complex that just replacing Fl_Menu_Bar by Fl_Sys_Menu_Bar.

  On the macOS platform, the system menu bar of any FLTK app begins with the Application
  menu which the FLTK library automatically constructs. Functions
  Fl_Mac_App_Menu::custom_application_menu_items() and Fl_Sys_Menu_Bar::about() can be used to further customize
  the Application menu. The FLTK library also automatically constructs and handles a Window menu which can be
  further customized (or even removed) calling
  Fl_Sys_Menu_Bar::window_menu_style(window_menu_style_enum style).
  Other member functions of this class allow the app to generate the rest of the system menu bar.
  It is recommended to localize the system menu bar using the standard Mac OS X localization procedure
  (see \ref osissues_localize).

 Changes to the menu state are immediately visible in the menubar when they are made
 using member functions of the Fl_Sys_Menu_Bar class. Other changes (e.g., by a call to
 Fl_Menu_Item::set()) should be followed by a call to update() to be
 visible in the menubar across all platforms. macOS global variable \ref fl_sys_menu_bar points to
 the unique, current system menu bar.

 A few FLTK menu features are not supported by the Mac System menu:
  \li no symbolic labels
  \li no embossed labels
  \li no font sizes

 As described above, the submenu with the application's own name (usually
 the second submenu from the left, immediately following the "Apple" submenu)
 is a special case, and can be managed with
 Fl_Mac_App_Menu::custom_application_menu_items().
 For example, to make your own "Appname -> Preferences" dialog, you might use:

 \code
 #include <FL/platform.H>        // for Fl_Mac_App_Menu class
 #include <FL/Fl_Sys_Menu_Bar.H> // for Fl_Menu_Item
 :
 void prefs_cb(Fl_Widget *w, void *data) {
   // ..Open your preferences dialog here..
 }
 :
 int main(..) {
    :
    // Items to add to the application menu
    static Fl_Menu_Item appitems[] = {
       { "Preferences", 0, prefs_cb, 0, 0 },
       { 0 }, { 0 }
     };
     Fl_Mac_App_Menu::custom_application_menu_items(appitems);  // adds it
  }
 \endcode

 ..the result being:

 \image html mac-app-menu-preferences.png  "Mac Application submenu"
 \image latex mac-app-menu-preferences.png "Mac Application submenu" width=4cm

  */
class FL_EXPORT Fl_Sys_Menu_Bar : public Fl_Menu_Bar {
  static Fl_Sys_Menu_Bar_Driver *driver();
protected:
  void draw() FL_OVERRIDE;
public:
  /** Possible styles of the Window menu in the system menu bar */
  typedef enum {
    no_window_menu = 0,     ///< No Window menu in the system menu bar
    tabbing_mode_none,      ///< No tabbed windows, but the system menu bar contains a Window menu
    tabbing_mode_automatic, ///< Windows are created by themselves but can be tabbed later
    tabbing_mode_preferred  ///< Windows are tabbed when created
  } window_menu_style_enum;
  Fl_Sys_Menu_Bar(int x,int y,int w,int h,const char *l=0);
  virtual ~Fl_Sys_Menu_Bar();
  /** Return the system menu's array of Fl_Menu_Item's
   */
  const Fl_Menu_Item *menu() const {return Fl_Menu_::menu();}
  void menu(const Fl_Menu_Item *m);
  void update() FL_OVERRIDE;
  void play_menu(const Fl_Menu_Item *) FL_OVERRIDE;
  int add(const char* label, int shortcut, Fl_Callback*, void *user_data=0, int flags=0);
  /** Adds a new menu item.
   \see Fl_Menu_::add(const char* label, int shortcut, Fl_Callback*, void *user_data=0, int flags=0)
   */
  int add(const char* label, const char* shortcut, Fl_Callback* cb, void *user_data=0, int flags=0) {
    return add(label, fl_old_shortcut(shortcut), cb, user_data, flags);
    }
  int add(const char* str);
  int insert(int index, const char* label, int shortcut, Fl_Callback *cb, void *user_data=0, int flags=0);
  /** Insert a new menu item.
   \see Fl_Menu_::insert(int index, const char* label, const char* shortcut, Fl_Callback *cb, void *user_data=0, int flags=0)
   */
  int insert(int index, const char* label, const char* shortcut, Fl_Callback *cb, void *user_data=0, int flags=0) {
    return insert(index, label, fl_old_shortcut(shortcut), cb, user_data, flags);
    }
  void remove(int n);
  void replace(int index, const char *name);
  void clear();
  int clear_submenu(int index);
  void  mode (int i, int fl);
  /** Gets the flags of item i.
   */
  int mode(int i) const { return Fl_Menu_::mode(i); }
  void shortcut (int i, int s);
  void setonly (Fl_Menu_Item *item);
  static void about(Fl_Callback *cb, void *data);

  static window_menu_style_enum window_menu_style();
  static void window_menu_style(window_menu_style_enum style);
  static void create_window_menu();
};

/** The system menu bar.
 */
extern Fl_Sys_Menu_Bar *fl_sys_menu_bar;

#endif // Fl_Sys_Menu_Bar_H
