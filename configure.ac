#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([CMatrix], [2.0], [ewfalor@gmail.com])
AC_CONFIG_SRCDIR([cmatrix.c])
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([-Wall -Werror subdir-objects])

# Checks for programs.
AC_PROG_CC_C99
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AM_PROG_CC_C_O

# Checks for libraries.
AC_CHECK_LIB([ncurses], [initscr])

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h stddef.h stdint.h stdlib.h string.h sys/ioctl.h termio.h termios.h unistd.h])

# Check for curses library.
AC_CHECK_HEADERS([curses.h ncurses.h])
AC_CHECK_LIB([ncurses], [initscr], [CURSES_LIB=-lncurses CURSES_LIB_NAME=ncurses])

if eval "test x$CURSES_LIB_NAME = x"
then
    AC_CHECK_LIB([curses], [initscr], [CURSES_LIB="-lcurses" CURSES_LIB_NAME=curses])
fi

if eval "test x$CURSES_LIB_NAME = x"
then
    AC_CHECK_LIB([termcap], [tgetent], [CURSES_LIB="-ltermcap" CURSES_LIB_NAME=termcap])
fi

if eval "test x$CURSES_LIB_NAME = x"
then
    AC_MSG_WARN([
*** No termcap lib available, consider getting the official ncurses
*** distribution from ftp://ftp.gnu.org/pub/gnu/ncurses if you get
*** errors compiling CMatrix.])
else
    AC_MSG_RESULT("Using $CURSES_LIB_NAME as the termcap library")
fi

AC_CHECK_LIB([$CURSES_LIB_NAME], [use_default_colors], [AC_DEFINE([HAVE_USE_DEFAULT_COLORS], [], [Whether this curses library allows the "default" color, for transparency])])
AC_CHECK_LIB([$CURSES_LIB_NAME], [resizeterm],         [AC_DEFINE([HAVE_RESIZETERM], [], [Can we resize the term with resizeterm()])])
AC_CHECK_LIB([$CURSES_LIB_NAME], [wresize],            [AC_DEFINE([HAVE_WRESIZE], [], [Can we resize the term with wresize()])])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_TYPE_PID_T
AC_TYPE_SIZE_T

# Checks for library functions.
AC_CHECK_FUNCS([atexit dup2 getpagesize memset putenv strcasecmp strdup])

AC_CONFIG_FILES([Makefile])


LIBS="$LIBS $CURSES_LIB"
AC_ARG_ENABLE(debug, [  --enable-debug          Enable debugging (def disabled)],)

AC_SUBST(CURSES_LIB)

AC_OUTPUT
