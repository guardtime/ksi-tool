#
# GUARDTIME CONFIDENTIAL
#
# Copyright (C) [2015 - 2016] Guardtime, Inc
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
AC_INIT([ksi], m4_esyscmd([tr -d [:space:] < VERSION]), [support@guardtime.com])
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

AC_DEFINE_UNQUOTED(TOOL_NAME, "AC_PACKAGE_NAME", [Commit id])

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O

AC_PROG_LN_S
# Checks for libraries.
libksi_ver="3.8"
AC_ARG_ENABLE(static-build,         
[  --enable-static-build              enable static linking of ksi library)],:,:)
AC_ARG_ENABLE(use-installed-libksi, 
[  --enable-use-installed-libksi      use installed ksi library)],:,:)
if test "$enable_static_build" = "yes" ; then
	AC_CANONICAL_HOST

	if test "${host_os#*linux}" != "$host_os"; then
		dy_link_flag="-Bdynamic"
	else
		dy_link_flag="-dynamic"
	fi

	AC_MSG_NOTICE([KSI library will be linked statically.])
	AC_CHECK_LIB([ksi], [KSI_CTX_new], [LIBS="-Wl,-static -lksi -Wl,$dy_link_flag $LIBS"], [AC_MSG_FAILURE([Could not find libksi (KSI C SDK). Install libksi or specify the library path manually.])])
	KSI_RPM_BUILD_FLAGS="--enable-static-build"
	
	if test "$enable_use_installed_libksi" = "yes" ; then
		KSI_BUILD_DEPENDECY="BuildRequires: libksi-devel >= $libksi_ver"
	fi
else
	AC_MSG_NOTICE([KSI library will be linked dynamically.])
	AC_CHECK_LIB([ksi], [KSI_CTX_new], [], [AC_MSG_FAILURE([Could not find libksi (KSI C SDK). Install libksi or specify the library path manually.])])
	KSI_BUILD_DEPENDECY="BuildRequires: libksi-devel >= $libksi_ver"
fi

AC_SUBST(KSI_RPM_DEPENDECY)
AC_SUBST(KSI_BUILD_DEPENDECY)
AC_SUBST(KSI_RPM_BUILD_FLAGS)

AC_CHECK_LIB([crypto], [SHA256_Init], [], [AC_MSG_FAILURE([Could not find OpenSSL 0.9.8+ libraries.])])
AC_CHECK_LIB([curl], [curl_easy_init], [], [AC_MSG_FAILURE([Could nod find Curl libraries.])])

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h])
AC_CHECK_HEADER([ksi/ksi.h], [], [AC_MSG_FAILURE([Could not find include files (libksi-devel) of libksi (KSI C SDK). Install libksi-devel or specify the headers manually.])])

# Checks for typedefs, structures, and compiler characteristics.
# AC_CHECK_HEADER_STDBOOL
AC_TYPE_SIZE_T

# Checks for library functions.
AC_CHECK_FUNCS([strchr])

# Add more warnings
CFLAGS+=" -Wall"

AC_CONFIG_FILES([GNUmakefile src/GNUmakefile packaging/redhat/ksi.spec])
AC_OUTPUT
