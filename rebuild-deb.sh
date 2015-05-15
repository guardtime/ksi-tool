#!/bin/bash

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

set -e

deb_dir=packaging/deb


#Rebuild ksitool
./rebuild.sh


#Make directories
mkdir $deb_dir/tmp

mkdir -p $deb_dir/ksitool/usr
mkdir -p $deb_dir/ksitool/usr/bin
mkdir -p $deb_dir/ksitool/usr/share/doc
mkdir -p $deb_dir/ksitool/usr/share/doc/ksitool
mkdir -p $deb_dir/ksitool/usr/share/man/man1

#Copy content for installion
cp -f src/ksitool $deb_dir/ksitool/usr/bin/
gzip -c doc/ksitool.1 > $deb_dir/ksitool/usr/share/man/man1/ksitool.1.gz
cp -f license.txt $deb_dir/ksitool/usr/share/doc/ksitool/license.txt

#Get Version and architecture
VER=$(tr -d [:space:] < VERSION)
ARCH=$(dpkg --print-architecture)

echo build ver: $VER arch: $ARCH

#Copy into tmp and set VER and ARCH in control file
cp -r $deb_dir/ksitool $deb_dir/tmp/
sed -i s/@VER@/$VER/g $deb_dir/tmp/ksitool/DEBIAN/control 
sed -i s/@ARCH@/$ARCH/g $deb_dir/tmp/ksitool/DEBIAN/control 

#Build DEB
dpkg-deb --build $deb_dir/tmp/ksitool
mv $deb_dir/tmp/ksitool.deb ksitool_${VER}_${ARCH}.deb 


#Clean
rm -rf $deb_dir/tmp
rm -rf $deb_dir/ksitool/usr
