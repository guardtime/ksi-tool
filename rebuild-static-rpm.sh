#!/bin/sh

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


BUILD_DIR=~/rpmbuild
version=$(tr -d [:space:] < VERSION)

conf_args="--enable-static-build"

if [ "$#" -eq 1 ]; then
	libksi_path="$1"
	export CPPFLAGS=-I$libksi_path/include
	export LDFLAGS=-L$libksi_path/lib
	export LD_LIBRARY_PATH=$libksi_path/lib
else
	conf_args+=" --enable-use-installed-libksi"
fi

autoreconf -if && \
./configure $conf_args && \
make clean && \
make dist && \
mkdir -p $BUILD_DIR/{BUILD,RPMS,SOURCES,SPECS,SRPMS,tmp} && \
cp packaging/redhat/ksi.spec $BUILD_DIR/SPECS/ && \
cp ksi-tools-*.tar.gz $BUILD_DIR/SOURCES/ && \
rpmbuild -ba $BUILD_DIR/SPECS/ksi.spec && \
cp $BUILD_DIR/RPMS/*/ksi-tools-*$version*.rpm . && \
cp $BUILD_DIR/SRPMS/ksi-tools-*$version*.rpm .
