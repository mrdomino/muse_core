/*
 * Copyright 2015 Steven Dee.
 *
 * Redistributable under the terms of the GNU General Public License,
 * version 2. No warranty is implied by this distribution.
 */

#if defined _WIN32 || defined __CYGWIN__
# ifdef BUILDING_MUSE_CORE_DLL
#   define SO_EXPORT __declspec(dllexport)
# else
#   define SO_EXPORT __declspec(dllimport)
# endif
#else
# if __GNUC__ >= 4
#   define SO_EXPORT __attribute__ ((visibility ("default")))
# else
#   define SO_EXPORT
# endif
#endif
