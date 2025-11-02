//
// Multi-label header file for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2023 by Bill Spitzak and others.
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

#ifndef Fl_Multi_Label_H
#define Fl_Multi_Label_H

class Fl_Widget;
struct Fl_Menu_Item;

/** Allows a mixed text and/or graphics label to be applied to an Fl_Menu_Item or Fl_Widget.

  Most regular FLTK widgets now support the ability to associate both images and text
  with a label but some special cases, notably the non-widget Fl_Menu_Item objects, do not.
  Fl_Multi_Label may be used to create menu items that have an icon and text, which would
  not normally be possible for an Fl_Menu_Item.
  For example, Fl_Multi_Label is used in the New->Code submenu in fluid, and others.

  \image html Fl_Multi_Label-menu-item.png "Menu items with icons using Fl_Multi_Label"
  \image latex Fl_Multi_Label-menu-item.png "Menu items with icons using Fl_Multi_Label" width=4cm

  Each Fl_Multi_Label holds two elements, \p labela and \p labelb; each may hold either a
  text label (const char*) or an image (Fl_Image*). When displayed, \p labela is drawn first
  and \p labelb is drawn immediately to its right.

  More complex labels can be constructed by setting labelb as another Fl_Multi_Label and
  thus chaining up a series of label elements.

  When assigning a label element to one of \p labela or \p labelb, they should be
  explicitly cast to (const char*) if they are not of that type already.

  \note An Fl_Multi_Label object and all its components (label text, images, chained
    Fl_Multi_Label's and their linked objects) must exist during the lifetime of the
    widget or menu item they are assigned to. It is the responsibility of the user's
    code to release these linked objects if necessary after the widget or menu is deleted.

  Example Use: Fl_Menu_Bar
  \code
    Fl_Pixmap *image = new Fl_Pixmap(..);     // image for menu item; any Fl_Image based widget
    Fl_Menu_Bar *menu = new Fl_Menu_Bar(..);  // can be any Fl_Menu_ oriented widget (Fl_Choice, Fl_Menu_Button..)

    // Create a menu item
    int i = menu->add("File/New", ..);
    Fl_Menu_Item *item = (Fl_Menu_Item*)&(menu->menu()[i]);

    // Create a multi label, assign it an image + text
    Fl_Multi_Label *ml = new Fl_Multi_Label;

    // Left side of label is an image
    ml->typea  = FL_IMAGE_LABEL;
    ml->labela = (const char*)image;           // any Fl_Image widget: Fl_Pixmap, Fl_PNG_Image, etc..

    // Right side of label is label text
    ml->typeb  = FL_NORMAL_LABEL;
    ml->labelb = item->label();

    // Assign the multilabel to the menu item
    // ml->label(item); // deprecated since 1.4.0; backwards compatible with 1.3.x
    item->label(ml);    // new method since 1.4.0
  \endcode

  \see Fl_Label and Fl_Labeltype and examples/howto-menu-with-images.cxx
*/
struct FL_EXPORT Fl_Multi_Label {
  /** Holds the "leftmost" of the two elements in the composite label.
      Typically this would be assigned either a text string (const char*),
      a (Fl_Image*) or a (Fl_Multi_Label*). */
  const char* labela;
  /** Holds the "rightmost" of the two elements in the composite label.
      Typically this would be assigned either a text string (const char*),
      a (Fl_Image*) or a (Fl_Multi_Label*). */
  const char* labelb;
  /** Holds the "type" of labela.
    Typically this is set to FL_NORMAL_LABEL for a text label,
    FL_IMAGE_LABEL for an image (based on Fl_image) or FL_MULTI_LABEL
    if "chaining" multiple Fl_Multi_Label elements together. */
  uchar typea;
  /** Holds the "type" of labelb.
    Typically this is set to FL_NORMAL_LABEL for a text label,
    FL_IMAGE_LABEL for an image (based on Fl_image) or FL_MULTI_LABEL
    if "chaining" multiple Fl_Multi_Label elements together. */
  uchar typeb;

  // This method is used to associate a Fl_Multi_Label with a Fl_Widget.
  void label(Fl_Widget*);

  // This method is used to associate a Fl_Multi_Label with a Fl_Menu_Item.
  void label(Fl_Menu_Item*);
};

#endif
