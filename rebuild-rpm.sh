#!/bin/sh

BUILD_DIR=~/rpmbuild

autoreconf -if && \
./configure $* && \
make clean && \
make dist && \
mkdir -p $BUILD_DIR/{BUILD,RPMS,SOURCES,SPECS,SRPMS,tmp} && \
cp redhat/ksitool.spec $BUILD_DIR/SPECS/ && \
cp ksitool-*.tar.gz $BUILD_DIR/SOURCES/ && \
rpmbuild -ba $BUILD_DIR/SPECS/ksitool.spec && \
cp $BUILD_DIR/RPMS/*/ksitool-*.rpm . && \
cp $BUILD_DIR/SRPMS/ksitool-*.rpm .
