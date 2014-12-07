#ifndef IX_DEFS_H_
#define IX_DEFS_H_

#if defined _WIN32 || defined __CYGWIN__
# ifdef BUILDING_DLL
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

#endif  /* IX_DEFS_H_ */
