/*
 * Filename header file for the Fast Light Tool Kit (FLTK).
 *
 * Copyright 1998-2023 by Bill Spitzak and others.
 *
 * This library is free software. Distribution and use rights are outlined in
 * the file "COPYING" which should have been included with this file.  If this
 * file is missing or damaged, see the license at:
 *
 *     https://www.fltk.org/COPYING.php
 *
 * Please see the following page on how to report bugs and issues:
 *
 *     https://www.fltk.org/bugs.php
 */

/*
 * Note to devs:
 * Under Windows, we include filename.H from numericsort.c; this should probably change.
 * This implies that we need C-style comments and '#ifdef __cplusplus ... #endif'
 */

/** \file
  File names and URI utility functions.
*/

#ifndef FL_FILENAME_H
#  define FL_FILENAME_H

#include "Fl_Export.H"
#include <FL/platform_types.h>

#ifdef __cplusplus

// The following include is not (yet) used in FLTK 1.4
// In FLTK 1.5 or 4.0 using std::string would be default.
// #include <string>

#endif /* __cplusplus */

/** \addtogroup filenames File names and URI utility functions
 File names and URI functions defined in <FL/filename.H>
    @{ */

#  define FL_PATH_MAX 2048 /**< all path buffers should use this length */
/** Gets the file name from a path.
    Similar to basename(3), exceptions shown below.
    \code
    #include <FL/filename.H>
    [..]
    const char *out;
    out = fl_filename_name("/usr/lib");     // out="lib"
    out = fl_filename_name("/usr/");        // out=""      (basename(3) returns "usr" instead)
    out = fl_filename_name("/usr");         // out="usr"
    out = fl_filename_name("/");            // out=""      (basename(3) returns "/" instead)
    out = fl_filename_name(".");            // out="."
    out = fl_filename_name("..");           // out=".."
    \endcode
    \return a pointer to the char after the last slash, or to \p filename if there is none.
 */
FL_EXPORT const char *fl_filename_name(const char * filename);
FL_EXPORT const char *fl_filename_ext(const char *buf);
FL_EXPORT char *fl_filename_setext(char *to, int tolen, const char *ext);
FL_EXPORT int fl_filename_expand(char *to, int tolen, const char *from);
FL_EXPORT int fl_filename_absolute(char *to, int tolen, const char *from);
FL_EXPORT int fl_filename_relative(char *to, int tolen, const char *from);
FL_EXPORT int fl_filename_match(const char *name, const char *pattern);
FL_EXPORT int fl_filename_isdir(const char *name);

#  if defined(__cplusplus)

FL_EXPORT int fl_filename_absolute(char *to, int tolen, const char *from, const char *cwd);
FL_EXPORT int fl_filename_relative(char *to, int tolen, const char *from, const char *cwd);


// FIXME: We can't do this in 1.4.x - enable this block in 1.5 or higher.
// See fluid/fluid_filename.{h|cxx} for an implementation using Fl_String.

// FL_EXPORT std::string fl_filename_name(const std::string &filename);
// FL_EXPORT std::string fl_filename_path(const std::string &filename);
// FL_EXPORT std::string fl_filename_ext(const std::string &filename);
// FL_EXPORT std::string fl_filename_setext(const std::string &filename, const std::string &new_extension);
// FL_EXPORT std::string fl_filename_expand(const std::string &from);
// FL_EXPORT std::string fl_filename_absolute(const std::string &from);
// FL_EXPORT std::string fl_filename_absolute(const std::string &from, const std::string &base);
// FL_EXPORT std::string fl_filename_relative(const std::string &from);
// FL_EXPORT std::string fl_filename_relative(const std::string &from, const std::string &base);
// FL_EXPORT std::string fl_getcwd();

#  endif /* defined(__cplusplus) */

#  if defined(__cplusplus) && !defined(FL_DOXYGEN)
/*
 * Under Windows, we include filename.H from numericsort.c; this should probably change...
 */

inline char *fl_filename_setext(char *to, const char *ext) { return fl_filename_setext(to, FL_PATH_MAX, ext); }
inline int fl_filename_expand(char *to, const char *from) { return fl_filename_expand(to, FL_PATH_MAX, from); }
inline int fl_filename_absolute(char *to, const char *from) { return fl_filename_absolute(to, FL_PATH_MAX, from); }
inline int fl_filename_relative(char *to, const char *from) { return fl_filename_relative(to, FL_PATH_MAX, from); }
#  endif /* __cplusplus */

#  if defined (__cplusplus)
extern "C" {
#  endif /* __cplusplus */

#  if !defined(FL_DOXYGEN)
FL_EXPORT int fl_alphasort(struct dirent **, struct dirent **);
FL_EXPORT int fl_casealphasort(struct dirent **, struct dirent **);
FL_EXPORT int fl_casenumericsort(struct dirent **, struct dirent **);
FL_EXPORT int fl_numericsort(struct dirent **, struct dirent **);
#  endif

typedef int (Fl_File_Sort_F)(struct dirent **, struct dirent **); /**< File sorting function. \see fl_filename_list() */

#  if defined(__cplusplus)
}

/*
 * Portable "scandir" function.  Ugly but necessary...
 */

FL_EXPORT int fl_filename_list(const char *d, struct dirent ***l,
                               Fl_File_Sort_F *s = fl_numericsort);
FL_EXPORT void fl_filename_free_list(struct dirent ***l, int n);

/*
 * Generic function to open a Uniform Resource Identifier (URI) using a
 * system-defined program (added in FLTK 1.1.8)
 */

FL_EXPORT int   fl_open_uri(const char *uri, char *msg = (char *)0,
                            int msglen = 0);

FL_EXPORT void fl_decode_uri(char *uri);

#  endif /* __cplusplus */

/*
 * Note: FLTK 1.0.x compatibility definitions (FLTK_1_0_COMPAT) dropped in 1.4.0
 */

#endif /* FL_FILENAME_H */

/** @} */
