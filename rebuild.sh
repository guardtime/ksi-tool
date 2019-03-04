#!/bin/sh

#
# Copyright 2013-2019 Guardtime, Inc.
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
	echo "  $0 [-s] [-d|-r] [Options]"
	echo "  $0 --get-dep-online [-s] [-d|-r --no-dep-check] [Options]"
	echo "  $0 -l path -i path [-s] [-d|-r --no-dep-check] [Options]"
	echo ""

	echo "Description:"
	echo "  This is KSI tool general build script. It can be used to build KSI tool"
	echo "  (packages rpm or deb) with libksi and libparamset statically or dynamically."
	echo "  See sections 'Examples' and 'Problems that can be resolved with this script'"
	echo "  to see why and how this script is used."
	echo ""
	echo ""

	echo "Options:"
	echo "  --link-static | -s"
	echo "       - Link libksi and libparamset statically. Note that ONLY libksi and"
	echo "         libparamset are linked statically. On some platforms this may not work."
	echo "         See example 2, 4 and ./configure -h to alter linking process."
	echo ""
	echo "  --build-rpm | -r"
	echo "       - Build RPM package."
	echo ""
	echo "  --build-deb | -d"
	echo "       - Build Deb package."
	echo ""
	echo "  --lib-dir | -l"
	echo "       - Path to directory containing library objects. Can have multiple"
	echo "         values. Note that full path is required!"
	echo ""
	echo "  --inc-dir | -i"
	echo "       - Path to directory containing include directories. Can have."
	echo "         multiple values. Note that full path is required!"
	echo ""
	echo "  --lib | -b"
	echo "       - Alter environment variable LIBS (see configure -h). Can be used to"
	echo "         specify library file explicitly. Note that full path is required!"
	echo "         See examples 2 and 4."
	echo ""
	echo "  --configure-flags | -c"
	echo "       - Extra flags for configure script. Note that -s will already add"
	echo "         something to configure options."
	echo ""
	echo "  --make-flags | -m"
	echo "       - Extra flags for make file."
	echo ""
	echo "  --linker-flags | -L"
	echo "       - Extra flags that are set to temporary environment variable LDFLAGS."
	echo "         Note that -l and --get-dep-online will affect that."
	echo ""
	echo "  --compiler-flags | -C"
	echo "       - Extra flags that are set to temporary environment variable CPPFLAGS."
	echo "         Note that -i and --get-dep-online will affect that."
	echo ""
	echo "  --get-dep-online"
	echo "       - When this flag is set, libksi and libparamset are downloaded from"
	echo "         github and built. Result is dumped in directory 'dependencies' that"
	echo "         will contain 'include' and 'lib' directory. Libraries built should"
	echo "         be available automatically. If not, see -l and -i."
	echo ""
	echo "  --no-dep-check"
	echo "       - No dependency check is performed when building rpm or deb package. Note"
	echo "         that it doesn't remove required dependencies from constructed packages!"
	echo "         It is useful when building packages with dependencies that are not"
	echo "         installed by package manager."
	echo ""
	echo "  --ign-online-dep-test"
	echo "       - This option can be combined with --get-dep-online to ignore failing"
	echo "         tests of the dependencies built. Note that actually it ignores the exit"
	echo "         code of the rebuild script and continues in case of error. Make sure"
	echo "         that You really know what You are doing when using this option!"
	echo ""
	echo "  -v"
	echo "       - Verbose output."
	echo ""
	echo "  --help | -h"
	echo "       - You are reading it right now."
	echo ""
	echo ""

	echo "Problems that can be resolved with this script:"
	echo "  1) When all required dependencies are installed, just run ./rebuild.sh."
	echo "  2) When there are wrong dependencies installed or it is prohibited to install"
	echo "     extra packages, specify include and library paths for libksi and/or"
	echo "     libparamset with -i and -l. If there are no library and include files"
	echo "     present, consider using --get-dep-online. Use --link-static to make binary"
	echo "     work without depending on installed libksi or libparamset. See examples 1."
	echo "  3) When packaging is performed in 'sterile' environment and it is prohibited"
	echo "     to install any packages, use advice given in point 2 with --no-dep-check."
	echo "     This option skips checking if libksi or libparamset packages are installed"
	echo "     during the build. Constructed packages are correct and dependency check is"
	echo "     performed during the install. See example 3."
	echo "  4) When option --link-static just does not work (OSX) and binaries built are"
	echo "     linked dynamically see examples 2 and 4 for forced static linking."
	echo ""
	echo ""

	echo "Examples:"
	echo ""
	echo "  1) Link KSI tool with libksi (e.g. cloned from github), from not default"
	echo "  location, statically. Useful when libksi is not installed or installed version"
	echo "  do not match. Installed version of libparamset is used."
	echo ""
	echo "    ./rebuild.sh -s -i /usr/tmp/libksi/src/ -l /usr/tmp/libksi/src/ksi/.libs/"
	echo ""
	echo "  2) Force KSI tool to link with static libksi library when --link-static"
	echo "  fails or does not perform static linking (see ./configure -h)."
	echo ""
	echo "    ./rebuild.sh -i /usr/src/ksi/ -c '--without-libksi --disable-silent-rules' \\"
	echo "     --lib /usr/lib/libksi.a'"
	echo ""
	echo "  3) Build KSI tool rpm packages with libksi and libparamset from github. Libksi"
	echo "  and libparamset are linked and packaged statically."
	echo ""
	echo "    ./rebuild.sh --get-dep-online --no-dep-check --link-static --build-rpm"
	echo ""
	echo "  4) Build KSI tool deb packages with libksi and libparamset from github. Force"
	echo "  libksi and libparamset to be linked and packaged statically (see"
	echo "  ./configure -h)."
	echo ""
	echo "    ./rebuild.sh --get-dep-online --no-dep-check --build-deb \\"
	echo "    -c '--without-libksi --without-libparamset' \\"
	echo "    --lib \`pwd\`/dependencies/lib/libksi.a \\"
	echo "    --lib \`pwd\`/dependencies/lib/libparamset.a"
	echo ""

}

conf_args=""
make_args=""
include_dir=""
lib_dir=""
lib_path=""
lib_extra=""
extra_linker_flags=""
extra_compiler_flags=""
rpmbuild_flags=""
debuild_flags=""

is_inc_dir_set=false
is_lib_dir_set=false
is_lib_extra=false
is_liblink_static=false
is_extra_l_or_c_flags=false
is_verbose=false
do_build_rpm=false
do_build_deb=false
do_build_dependecies=false
show_help=false
rebuild_tool_dependencies_flags=""

# Simple command-line parameter parser.
while [ "$1" != "" ]; do
	case $1 in
		--link-static | -s )	 echo "Linking libksi and libparamset statically."
								 is_liblink_static=true
								 ;;
		--build-rpm | -r )		 echo "Building rpm."
								 do_build_rpm=true
								 ;;
		--build-deb | -d )		 echo "Building deb."
								 do_build_deb=true
								 ;;
		--lib-dir | -l )	 	 shift
								 echo "Library search path added: '$1'."
								 lib_dir="$lib_dir -L$1"
								 lib_path="$lib_path $1:"
								 is_lib_dir_set=true
								 ;;
		--inc-dir | -i )	 	 shift
								 echo "Include file path added: '$1'."
								 include_dir="$include_dir -I$1"
								 is_inc_dir_set=true
								 ;;
		--lib | -b )			 shift
								 echo "Library added to LIBS (configure env): '$1'."
								 lib_extra="$lib_extra $1"
								 is_lib_extra=true
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
		--get-dep-online )	     echo "Download and build libksi and libparamset."
								 do_build_dependecies=true
								 ;;
		--no-dep-check )	     echo "Ignoring 'build depends on' when building a package."
								 rpmbuild_flags="--nodeps"
								 debuild_flags="-d"
								 ;;
		--ign-online-dep-test )	 echo "Ignoring test results while building online dependencies"
								 rebuild_tool_dependencies_flags="--ignore-test-results"
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

if $do_build_dependecies ; then
	is_inc_dir_set=true
	is_lib_dir_set=true
	./rebuild-tool-dependencies.sh $rebuild_tool_dependencies_flags
	include_dir="$include_dir -I$(pwd)/dependencies/include"
	lib_dir="$lib_dir -L$(pwd)/dependencies/lib"
	lib_path="$lib_path $(pwd)/dependencies/lib:"
fi

if $is_extra_l_or_c_flags ; then
	export CPPFLAGS="$CPPFLAGS $extra_compiler_flags"
	export LDFLAGS="$LDFLAGS $extra_linker_flags"
fi

if $is_lib_extra ; then
	export LIBS="$lib_extra"
fi

if $is_inc_dir_set ; then
	export CPPFLAGS="$CPPFLAGS $include_dir"
fi

if $is_lib_dir_set ; then
	export LDFLAGS="$LDFLAGS $lib_dir"
	export LD_LIBRARY_PATH="$LD_LIBRARY_PATH $lib_path"
fi



if $is_liblink_static ; then
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
	conf_args="$conf_args --disable-silent-rules"
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
		rpmbuild -ba $rpmbuild_flags $BUILD_DIR/SPECS/ksi.spec && \
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
		# debuild cleans some environment variables, to keep LIBS -e is used.
		debuild -e LIBS -us -uc $debuild_flags
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
