#!/bin/sh

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
