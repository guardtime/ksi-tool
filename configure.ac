#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.59])
AC_INIT([ksitool], [1.0.0], [taavi.valjaots@guardtime.com])
#AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC

AC_PROG_LN_S
# Checks for libraries.
AC_CHECK_LIB([crypto], [SHA256_Init], [], [AC_MSG_FAILURE([Could not find OpenSSL 0.9.8+ libraries.])])
AC_CHECK_LIB([curl], [curl_easy_init], [], [AC_MSG_FAILURE([Could nod find Curl libraries.])])
AC_CHECK_LIB([ksi], [KSI_CTX_new], [], [AC_MSG_FAILURE([Could not find KSI libraries.])])


# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h])

# Checks for typedefs, structures, and compiler characteristics.
# AC_CHECK_HEADER_STDBOOL
AC_TYPE_SIZE_T

# Checks for library functions.
AC_CHECK_FUNCS([strchr])




AC_CONFIG_FILES([GNUmakefile src/GNUmakefile redhat/ksitool.spec])
AC_OUTPUT
