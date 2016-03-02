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
ADD_LICENSE_BACKGROUND="<background file='background.png' mime-type='image/png' /> \
                       <license file='license.txt' />\
                       </installer-gui-script>"

MAC_STATIC_DIR="packaging/MacOS/mac_static_content"


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
mkdir -p $MAC_STATIC_DIR/Source && \
cp license.txt $MAC_STATIC_DIR/Resources/license.txt && \
cp src/ksitool $MAC_STATIC_DIR/Source/ksitool && \
#create content needed for installer package
pkgbuild --root $MAC_STATIC_DIR/Source/ --identifier KsitoolInstaller --install-location /usr/local/bin $MAC_STATIC_DIR/Source.pkg && \
productbuild --synthesize --package $MAC_STATIC_DIR/Source.pkg $MAC_STATIC_DIR/Distribution.xml && \
#configure xml file must be modified so that license and background picture will be included in the installer package
sed -i '' "s:${OLD_STRING}:${ADD_LICENSE_BACKGROUND}:g" $MAC_STATIC_DIR/Distribution.xml && \
productbuild --distribution $MAC_STATIC_DIR/Distribution.xml --resources $MAC_STATIC_DIR/Resources --package-path $MAC_STATIC_DIR $MAC_STATIC_DIR/$MAC_KSITOOL_INSTALLER_VERSION && \
rm $MAC_STATIC_DIR/Source.pkg && \
rm $MAC_STATIC_DIR/Distribution.xml
