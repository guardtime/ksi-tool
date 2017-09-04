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

#Get Version and architecture
VER=$(tr -d [:space:] < VERSION)
ARCH=$(dpkg --print-architecture)
PKG_VERSION=1


deb_dir=packaging/deb
tmp_dir=$deb_dir/tmp


#Rebuild ksitool
./rebuild-static.sh


bin_install_dir=usr/bin
bin_doc_install_dir=usr/share/doc/ksi-tools
bin_man_install_dir=usr/share/man/

#Make directories
mkdir -p $tmp_dir

mkdir -p $deb_dir/ksi-tools/$bin_install_dir
mkdir -p $deb_dir/ksi-tools/$bin_doc_install_dir
mkdir -p $deb_dir/ksi-tools/$bin_man_install_dir/man1
mkdir -p $deb_dir/ksi-tools/$bin_man_install_dir/man5

#Copy content for installion
cp -f src/ksi $deb_dir/ksi-tools/$bin_install_dir

gzip -c doc/ksi.1 > $deb_dir/ksi-tools/$bin_man_install_dir/man1/ksi.1.gz
gzip -c doc/ksi-sign.1 > $deb_dir/ksi-tools/$bin_man_install_dir/man1/ksi-sign.1.gz
gzip -c doc/ksi-extend.1 > $deb_dir/ksi-tools/$bin_man_install_dir/man1/ksi-extend.1.gz
gzip -c doc/ksi-verify.1 > $deb_dir/ksi-tools/$bin_man_install_dir/man1/ksi-verify.1.gz
gzip -c doc/ksi-pubfile.1 > $deb_dir/ksi-tools/$bin_man_install_dir/man1/ksi-pubfile.1.gz
gzip -c doc/ksi-conf.5 > $deb_dir/ksi-tools/$bin_man_install_dir/man5/ksi-conf.5.gz
cp -f LICENSE $deb_dir/ksi-tools/$bin_doc_install_dir
cp -f doc/ChangeLog $deb_dir/ksi-tools/$bin_doc_install_dir

#Copy into tmp and set VER and ARCH in control file
cp -r $deb_dir/ksi-tools $deb_dir/tmp/
sed -i s/@DPKG_VERSION_REPLACED_WITH_SED@/$ARCH/g $deb_dir/tmp/ksi-tools/DEBIAN/control

#Build DEB
dpkg-deb --build $deb_dir/tmp/ksi-tools
mv $deb_dir/tmp/ksi-tools.deb ksi-tools_${VER}-${PKG_VERSION}_${ARCH}.deb


#Clean
rm -rf $deb_dir/tmp
rm -rf $deb_dir/ksi-tools/usr
