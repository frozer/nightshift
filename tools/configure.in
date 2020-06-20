AC_PREREQ([2.69])
AC_INIT([parser],[1.0.1],[n.halcyon@gmail.com])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([parser.c])
AC_CONFIG_HEADERS([config.h])
AM_PROG_LIBTOOL

# Checks for programs.
AC_PROG_CC

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([strerror strtol])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
