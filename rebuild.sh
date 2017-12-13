#!/bin/sh

#
# Copyright 2013-2017 Guardtime, Inc.
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

set -e


help_txt() {
	echo "Usage:"
	echo "  $0 [-c options] [-m options] [-s] [-p path] [-l path -i path] [-d|-r]"
	echo ""

	echo "Description:"
	echo "  This is KSI tool general build script. It can be used to build KSI tool"
	echo "  (packages rpm or deb) with libksi statically or dynamically."
	echo ""
	echo ""

	echo "Options:"
	echo "  --libksi-static | -s"
	echo "       - Link libksi statically. Note that only libksi is linked statically."
	echo ""
	echo "  --build-rpm | -r"
	echo "       - Build RPM package."
	echo ""
	echo "  --build-deb | -d"
	echo "       - Build Deb package."
	echo ""
	echo "  --libksi-path | -p"
	echo "       - The path to libksi library and include files. Directory pointed by"
	echo "         this, must contain directories 'lib' and 'include/ksi'. To be more"
	echo "         precise see --libksi-lib-dir and --libksi-inc-dir. Note that the final"
	echo "         value used depends on the order of -p, -l and -i on command-line!"
	echo ""
	echo "  --libksi-lib-dir | -l"
	echo "       - Path to directory containing libksi library objects. Note that the"
	echo "         final value used depends on the order of -p, -l and -i on command-line!"
	echo ""
	echo "  --libksi-inc-dir | -i"
	echo "       - Path to directory containing directory 'ksi' that contains the actual"
	echo "         include files. Note that the final value used depends on the order of"
	echo "         -p, -l and -i on command-line!"
	echo ""
	echo "  --configure-flags | -c"
	echo "       - Extra flags for configure script. Note that -s, -l and -i will already"
	echo "         add something to configure options."
	echo ""
	echo "  --make-flags | -m"
	echo "       - Extra flags for make file."
	echo ""
	echo "  --linker-flags | -L"
	echo "       - Extra flags that are set to temporary environment variable LDFLAGS."
	echo "         Note that -p and -l add '-L' with libksi library to it."
	echo ""
	echo "  --compiler-flags | -C"
	echo "       - Extra flags that are set to temporary environment variable CPPFLAGS."
	echo "         Note that -p and -i add '-I' with libksi includes to it."
	echo ""
	echo "  -v"
	echo "       - Verbose output."
	echo ""
	echo "  --help | -h"
	echo "       - You are reading it right now."
	echo ""
	echo ""

	echo "Examples:"
	echo ""

	echo "  1) Link KSI tool with libksi (e.g. cloned from github) from not"
	echo "  default location statically."
	echo ""
	echo "    ./rebuild.sh -s -i /usr/tmp/libksi/src/ -l /usr/tmp/libksi/src/ksi/.libs/"
	echo ""

	echo "  2) Force KSI tool to link with static libksi library when --libksi-static"
	echo "  fails or does not perform static linking."
	echo ""
	echo "    ./rebuild.sh -i /usr/src/ksi/ -c '--without-libksi --disable-silent-rules"
	echo "     LIBS=/usr/lib/libksi.a'"
	echo ""
}

conf_args=""
make_args=""
libksi_include_dir=""
libksi_lib_dir=""
extra_linker_flags=""
extra_compiler_flags=""

is_installed_libksi=true
is_inc_dir_set=false
is_lib_dir_set=false
is_libksi_static=false
is_extra_l_or_c_flags=false
is_verbose=false
do_build_rpm=false
do_build_deb=false
show_help=false


# Simple command-line parameter parser.
while [ "$1" != "" ]; do
	case $1 in
		--libksi-static | -s )	 echo "Linking libksi statically."
								 is_libksi_static=true
								 ;;
		--build-rpm | -r )		 echo "Building rpm."
								 do_build_rpm=true
								 ;;
		--build-deb | -d )		 echo "Building deb."
								 do_build_deb=true
								 ;;
		--libksi-path | -p )	 shift
								 echo "Using libksi includes and lib from directory '$1'."
								 libksi_include_dir="$1/include"
								 libksi_lib_dir="$1/lib"
								 is_installed_libksi=false
								 is_lib_dir_set=true
								 is_inc_dir_set=true
								 ;;
		--libksi-lib-dir | -l )	 shift
								 echo "Using libksi library located in directory '$1'."
								 libksi_lib_dir=$1
								 is_installed_libksi=false
								 is_lib_dir_set=true
								 ;;
		--libksi-inc-dir | -i )	 shift
								 echo "Using libksi includes located in directory '$1'."
								 libksi_include_dir=$1
								 is_installed_libksi=false
								 is_inc_dir_set=true
								 ;;
		--configure-flags | -c ) shift
								 echo "Using extra configure flags '$1'."
								 conf_args="$conf_args $1"
								 ;;
		--make-flags | -m )		 shift
								 echo "Using extra make flags '$1'."
								 make_args="$make_args $1"
								 ;;
		--linker-flags | -L )	 shift
								 extra_linker_flags="$extra_linker_flags $1"
								 is_extra_l_or_c_flags=true
								 ;;
		--compiler-flags | -C )	 shift
								 extra_compiler_flags="$extra_compiler_flags $1"
								 is_extra_l_or_c_flags=true
								 ;;
		-v )					 is_verbose=true
								 ;;
		--help | -h )			 show_help=true
								 ;;
		* )						 echo "Unknown token '$1' from command-line."
								 show_help=true
	esac
	shift
done

if $show_help ; then
	help_txt
	exit 0
fi

if $is_extra_l_or_c_flags ; then
	export CPPFLAGS="$CPPFLAGS $extra_compiler_flags"
	export LDFLAGS="$LDFLAGS $extra_linker_flags"
fi

if $is_installed_libksi ; then
	echo "Using installed libksi."
else
	if $is_inc_dir_set ; then
		export CPPFLAGS="$CPPFLAGS -I$libksi_include_dir"
	fi

	if $is_lib_dir_set ; then
		export LDFLAGS="$LDFLAGS -L$libksi_lib_dir"
		export LD_LIBRARY_PATH="$LD_LIBRARY_PATH $libksi_lib_dir"
	fi
fi


if $is_libksi_static ; then
	conf_args="$conf_args --enable-static-build"
else
	echo "Linking with libksi dynamically."
fi


# Error handling.
if $do_build_rpm && $do_build_deb; then
	>&2 echo  "Error: It is not possible to build both deb and rpm packages!"
	exit 1
fi


# Simple configure and make with extra options.
if $is_verbose ; then
	echo "Using extra configure flags: '$conf_args'"
	echo "Using extra make flags: '$make_args'"
	echo "CPPFLAGS = $CPPFLAGS"
	echo "LDFLAGS  = $LDFLAGS"
fi

echo ""

autoreconf -if
./configure $conf_args
make $make_args clean

# Package the software.
if $do_build_rpm || $do_build_deb; then
	echo "Making dist."
	make dist
	version=$(tr -d [:space:] < VERSION)

	if $do_build_rpm ; then
		echo "Making rpm."
		BUILD_DIR=~/rpmbuild
		mkdir -p $BUILD_DIR/{BUILD,RPMS,SOURCES,SPECS,SRPMS,tmp} && \
		cp packaging/redhat/ksi.spec $BUILD_DIR/SPECS/ && \
		cp ksi-tools-*.tar.gz $BUILD_DIR/SOURCES/ && \
		rpmbuild -ba $BUILD_DIR/SPECS/ksi.spec && \
		cp $BUILD_DIR/RPMS/*/ksi-tools-*$version*.rpm . && \
		cp $BUILD_DIR/SRPMS/ksi-tools-*$version*.rpm . && \
		chmod -v 644 *.rpm
	elif $do_build_deb ; then
		ARCH=$(dpkg --print-architecture)
		RELEASE_VERSION="$(lsb_release -is)$(lsb_release -rs | grep -Po "[0-9]{1,3}" | head -1)"
		PKG_VERSION=1
		DEB_DIR=packaging/deb


		# Rebuild debian changelog.
		if command  -v dch > /dev/null; then
		  echo "Generating debian changelog..."
		  $DEB_DIR/rebuild_changelog.sh doc/ChangeLog $DEB_DIR/control ksi-tools $DEB_DIR/changelog "1.0.0:unstable "
		else
		  >&2 echo "Error: Unable to generate Debian changelog file as dch is not installed!"
		  >&2 echo "Install devscripts 'apt-get install devscripts'"
		  exit 1
		fi

		tar xvfz ksi-tools-$version.tar.gz
		mv ksi-tools-$version.tar.gz ksi-tools-$version.orig.tar.gz
		mkdir ksi-tools-$version/debian
		cp $DEB_DIR/control $DEB_DIR/changelog $DEB_DIR/rules $DEB_DIR/copyright ksi-tools-$version/debian
		chmod +x ksi-tools-$version/debian/rules
		cd ksi-tools-$version
		debuild -us -uc
		cd ..

		suffix=${version}-${PKG_VERSION}.${RELEASE_VERSION}_${ARCH}
		mv ksi-tools_${version}_${ARCH}.changes ksi-tools_$suffix.changes
		mv ksi-tools_${version}_${ARCH}.deb ksi-tools_$suffix.deb

		rm -rf ksi-tools-$version
	else
		>&2 echo  "Error: Undefined behaviour!"
		exit 1
	fi
else
	make $make_args
fi
