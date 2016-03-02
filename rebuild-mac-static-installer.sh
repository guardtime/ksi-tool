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


KSITOOL_VER=$(<VERSION)

MAC_KSITOOL_INSTALLER_VERSION="ksitool-mac-installer-$KSITOOL_VER.$MACHTYPE.pkg"

OLD_STRING='</installer-gui-script>'
ADD_README_BACKGROUND="<background file='background.png' mime-type='image/png' /> \
                       <license file='license.txt' />\
                       </installer-gui-script>"


PRF=ksitool-$(tr -d [:space:] < VERSION)

rm -f ${PRF}*.tar.gz && \
mkdir -p config m4 && \
echo Running autoreconf... && \
autoreconf -if && \
echo Running configure script... && \
./configure --enable-static-build && \
echo Running make... && \
make clean && \
make && \
mkdir -p mac_static_content/Source && \
cp src/ksitool mac_static_content/Source/ksitool && \
#create content needed for installer package
pkgbuild --root mac_static_content/Source/ --identifier KsitoolInstaller --install-location /usr/local/bin mac_static_content/Source.pkg && \
productbuild --synthesize --package mac_static_content/Source.pkg mac_static_content/Distribution.xml && \
#configure xml file must be modified so that license and background picture will be included in the installer package
sed -i '' "s:${OLD_STRING}:${ADD_README_BACKGROUND}:g" mac_static_content/Distribution.xml && \
productbuild --distribution mac_static_content/Distribution.xml --resources mac_static_content/Resources --package-path mac_static_content mac_static_content/$MAC_KSITOOL_INSTALLER_VERSION && \
rm mac_static_content/Source.pkg && \
rm mac_static_content/Distribution.xml
