#!/bin/sh

BUILD_DIR=~/rpmbuild

autoreconf -i && \
./configure $* && \
make clean && \
make dist && \
mkdir -p $BUILD_DIR/{BUILD,RPMS,SOURCES,SPECS,SRPMS,tmp} && \
cp redhat/gtime.spec $BUILD_DIR/SPECS/ && \
cp gtime-*.tar.gz $BUILD_DIR/SOURCES/ && \
rpmbuild -ba $BUILD_DIR/SPECS/gtime.spec && \
cp $BUILD_DIR/RPMS/*/gtime-*.rpm . && \
cp $BUILD_DIR/SRPMS/gtime-*.rpm .
