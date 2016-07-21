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
