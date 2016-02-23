#
# GUARDTIME CONFIDENTIAL
#
# Copyright (C) [2015] Guardtime, Inc
# All Rights Reserved
#
# NOTICE:  All information contained herein is, and remains, the
# property of Guardtime Inc and its suppliers, if any.
# The intellectual and technical concepts contained herein are
# proprietary to Guardtime Inc and its suppliers and may be
# covered by U.S. and Foreign Patents and patents in process,
# and are protected by trade secret or copyright law.
# Dissemination of this information or reproduction of this
# material is strictly forbidden unless prior written permission
# is obtained from Guardtime Inc.
# "Guardtime" and "KSI" are trademarks or registered trademarks of
# Guardtime Inc.
#

#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.59])
AC_INIT([ksitool], m4_esyscmd([tr -d [:space:] < VERSION]), [support@guardtime.com])
AC_CONFIG_AUX_DIR(config)
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([foreign -Wall -Werror foreign subdir-objects])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_CHECK_PROG(git_installed,git,"yes", "no")

# Define the git hash of the current version.
AC_MSG_CHECKING([for commit id])
commit_id=""
if test "$git_installed" = "yes" -a -x ".git"; then
    commit_id=$(git log -n 1 --format="%H")
    AC_MSG_RESULT([$commit_id])
    AC_DEFINE_UNQUOTED(COMMIT_ID, "$commit_id", [Commit id])
else
    AC_MSG_RESULT([no])
fi

# Checks for programs.
AC_PROG_CC

AC_PROG_LN_S

# Checks for libraries.
AC_ARG_ENABLE(static,
[  --enable-static      enable static build of ksitool, it is needed to specify static libraries with LIBS environment variable (e.g. "/usr/local/lib/libksi.a /usr/local/opt/openssl/lib/libcrypto.a /usr/local/lib/libcurl.a -lldap -lz")],
:, :)
if test $enable_static = "yes" ; then
    AC_MSG_NOTICE([KSI library will be linked statically])
else
    AC_CHECK_LIB([ksi], [KSI_CTX_new], [], [AC_MSG_FAILURE([Could not find KSI libraries.])])
fi

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h])

# Checks for typedefs, structures, and compiler characteristics.
# AC_CHECK_HEADER_STDBOOL
AC_TYPE_SIZE_T

# Checks for library functions.
AC_CHECK_FUNCS([strchr])




AC_CONFIG_FILES([GNUmakefile src/GNUmakefile packaging/redhat/ksitool.spec])
AC_OUTPUT
