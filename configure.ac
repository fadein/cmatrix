#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([cmatrix], [2.0], [ewfalor@gmail.com])
AC_CONFIG_SRCDIR([cmatrix.c])
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([-Wall -Werror])

# Checks for programs.
AC_PROG_CC_C99

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h stddef.h stdint.h stdlib.h string.h sys/ioctl.h termio.h termios.h unistd.h])

AC_CHECK_LIB([ncurses], [initscr])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_TYPE_PID_T
AC_TYPE_SIZE_T

# Checks for library functions.
AC_CHECK_FUNCS([atexit dup2 getpagesize memset putenv strcasecmp strdup])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
