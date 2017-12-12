#
# Copyright 2013-2016 Guardtime, Inc.
#
# This file is part of the Guardtime client SDK.
#
# Licensed under the Apache License, Version 2.0 (the "License").
# You may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#     http://www.apache.org/licenses/LICENSE-2.0
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES, CONDITIONS, OR OTHER LICENSES OF ANY KIND, either
# express or implied. See the License for the specific language governing
# permissions and limitations under the License.
# "Guardtime" and "KSI" are trademarks or registered trademarks of
# Guardtime, Inc., and no license to trademarks is granted; Guardtime
# reserves and retains all trademark rights.

#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.59])
AC_INIT([ksi-tools], m4_esyscmd([tr -d [:space:] < VERSION]), [support@guardtime.com])
TOOL_NAME=ksi
AC_CONFIG_AUX_DIR(config)
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([foreign -Wall -Werror foreign subdir-objects])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])


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


AC_DEFINE_UNQUOTED(TOOL_NAME, "$TOOL_NAME", [Commit id])
AC_SUBST(TOOL_NAME)
# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O

AC_PROG_LN_S
# Checks for libraries.
AC_CHECK_LIB([crypto], [SHA256_Init], [], [AC_MSG_FAILURE([Could not find OpenSSL 0.9.8+ libraries.])])
AC_CHECK_LIB([curl], [curl_easy_init], [], [AC_MSG_FAILURE([Could not find Curl libraries.])])

LIBKSI_VER="3.14"
AC_ARG_ENABLE(static-build,         AS_HELP_STRING([--enable-static-build], [Enable static linking of libksi. Note that on some platforms this option may not work and user has to provide the static libksi library file explicitly. See option --without-libksi instead. Default is no static.]),:,:)
AC_ARG_ENABLE(libksi-as-packaging-dependency, AS_HELP_STRING([--enable-libksi-as-packaging-dependency], [Add libksi as dependency to RPM and DEB package. Default is not added.]),:,:)

AC_ARG_WITH(libksi,
AS_HELP_STRING([--without-libksi], [Do not check or add libksi automatically for linking (no -lksi is added). Libksi library file must be provided by user (via LIBS variable) explicitly. Usable when default automatic linking does not work as expected (e.g. unable to link libksi statically). Default is with libksi.]),:,:)


if test "$enable_libksi_as_packaging_dependency" = "yes" ; then
	KSI_RPM_BUILD_DEPENDECY="BuildRequires: libksi-devel >= $LIBKSI_VER"
	KSI_DEB_BUILD_DEPENDECY="Build-Depends: libksi-dev(>=$LIBKSI_VER)"
	KSI_RPM_RUN_DEPENDECY="Requires: libksi >= $LIBKSI_VER"
	KSI_DEB_RUN_DEPENDECY="Depends: libksi(>=$LIBKSI_VER)"
fi

if test "$enable_static_build" = "yes" ; then
	AC_CANONICAL_HOST
	KSI_RPM_RUN_DEPENDECY=""
	KSI_DEB_RUN_DEPENDECY=""

	if test "${host_os#*linux}" != "$host_os" ; then
		dy_link_flag="-Bdynamic"
	else
		dy_link_flag="-dynamic"
	fi

	if test "$with_libksi" != "no" ; then
		AC_MSG_NOTICE([libksi will be linked statically.])
		AC_CHECK_LIB([ksi], [KSI_CTX_new], [LIBS="-Wl,-static -lksi -Wl,$dy_link_flag $LIBS"], [AC_MSG_FAILURE([Could not find libksi (KSI C SDK). Install libksi or specify the library path manually.])])
	else
		AC_MSG_ERROR("Error: flags --without-libksi and --enable-static-build can not be used at the same time!")
	fi

	KSI_PACKAGING_BUILD_FLAGS="--enable-static-build"
else
	if test "$with_libksi" != "no" ; then
		AC_MSG_NOTICE([libksi will be linked dynamically.])
		AC_CHECK_LIB([ksi], [KSI_CTX_new], [], [AC_MSG_FAILURE([Could not find libksi (KSI C SDK). Install libksi or specify the library path manually.])])
	else
		AC_MSG_NOTICE([Note that user must specify libksi explicitly! See configure option (configure -h) --without-libksi and environment variable LIBS.])
	fi
fi

AC_SUBST(KSI_RPM_RUN_DEPENDECY)
AC_SUBST(KSI_DEB_RUN_DEPENDECY)
AC_SUBST(KSI_DEB_BUILD_DEPENDECY)
AC_SUBST(KSI_RPM_BUILD_DEPENDECY)
AC_SUBST(KSI_PACKAGING_BUILD_FLAGS)


AC_CHECK_HEADER([ksi/ksi.h], [], [AC_MSG_FAILURE([Could not find include files (libksi-devel) of libksi (KSI C SDK). Install libksi-devel or specify the headers manually.])])

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h])

# Checks for typedefs, structures, and compiler characteristics.
# AC_CHECK_HEADER_STDBOOL
AC_TYPE_SIZE_T

# Checks for library functions.
AC_CHECK_FUNCS([strchr])

# Add more warnings
CFLAGS+=" -Wall"

AC_CONFIG_FILES([GNUmakefile src/GNUmakefile packaging/redhat/ksi.spec packaging/deb/control packaging/deb/rules])
AC_OUTPUT
