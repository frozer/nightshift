m4_include([version.m4])
AC_INIT([nightshift],VERSION_NUMBER,[n.halcyon@gmail.com])
AM_INIT_AUTOMAKE

LT_INIT

# Checks for programs.
AC_PROG_CC

AC_PROG_MAKE_SET

# Checks for libraries.
AX_PTHREAD

# Checks for libraries.
AC_CHECK_LIB(mosquitto,mosquitto_publish, [], [
  echo "Error! You need to have libmosquitto around."
  exit -1
])

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h inttypes.h stdint.h netinet/in.h stdlib.h string.h sys/socket.h unistd.h pthread.h byteswap.h mosquitto.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([bzero socket strerror strtol])

AC_CONFIG_FILES([
  dozord/Makefile
  libdozor/Makefile
  liblogger/Makefile
  tools/Makefile
  Makefile
])
AC_CONFIG_MACRO_DIRS([m4])
AC_OUTPUT
