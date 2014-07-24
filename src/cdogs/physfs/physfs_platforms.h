#ifndef _INCL_PHYSFS_PLATFORMS
#define _INCL_PHYSFS_PLATFORMS

#ifndef __PHYSICSFS_INTERNAL__
#error Do not include this header from your applications.
#endif

/*
 * These only define the platforms to determine which files in the platforms
 *  directory should be compiled. For example, technically BeOS can be called
 *  a "unix" system, but since it doesn't use unix.c, we don't define
 *  PHYSFS_PLATFORM_UNIX on that system.
 */

#if (defined __HAIKU__)
#  define PHYSFS_PLATFORM_HAIKU
#  define PHYSFS_PLATFORM_BEOS
#  define PHYSFS_PLATFORM_POSIX
#elif ((defined __BEOS__) || (defined __beos__))
#  define PHYSFS_PLATFORM_BEOS
#  define PHYSFS_PLATFORM_POSIX
#elif (defined _WIN32_WCE) || (defined _WIN64_WCE)
#  define PHYSFS_PLATFORM_POCKETPC
#elif (((defined _WIN32) || (defined _WIN64)) && (!defined __CYGWIN__))
#  define PHYSFS_PLATFORM_WINDOWS
#elif (defined OS2)
#  define PHYSFS_PLATFORM_OS2
#elif ((defined __MACH__) && (defined __APPLE__))
/* To check if iphone or not, we need to include this file */
# include <TargetConditionals.h> 
# if ((TARGET_IPHONE_SIMULATOR) || (TARGET_OS_IPHONE))
#    define PHYSFS_PLATFORM_UNIX
#    define PHYSFS_PLATFORM_POSIX
#    define PHYSFS_NO_CDROM_SUPPORT
#  else
#    define PHYSFS_PLATFORM_MACOSX
#    define PHYSFS_PLATFORM_POSIX
#  endif
#  define _GNU_SOURCE
#elif defined(macintosh)
#  error Classic Mac OS support was dropped from PhysicsFS 2.0. Move to OS X.
#elif defined(unix) || defined(__unix__)
#  define PHYSFS_PLATFORM_UNIX
#  define PHYSFS_PLATFORM_POSIX
#  define _POSIX_C_SOURCE 200809L
#  define _GNU_SOURCE
#else
#  error Unknown platform.
#endif

#endif  /* include-once blocker. */

