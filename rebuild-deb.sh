#!/bin/bash

deb_dir=packaging/deb
mkdir $deb_dir/tmp

mkdir -p $deb_dir/ksitool/usr
mkdir -p $deb_dir/ksitool/usr/bin
mkdir -p $deb_dir/ksitool/usr/share/doc
mkdir -p $deb_dir/ksitool/usr/share/doc/ksitool
mkdir -p $deb_dir/ksitool/usr/share/man/man1

cp -f src/ksitool $deb_dir/ksitool/usr/bin/
gzip -c doc/ksitool.1 > $deb_dir/ksitool/usr/share/man/man1/ksitool.1.gz

VER=$(tr -d [:space:] < VERSION)
ARCH=$(dpkg --print-architecture)

echo build ver: $VER arch: $ARCH


cp -r $deb_dir/ksitool $deb_dir/tmp/
sed -i s/@VER@/$VER/g $deb_dir/tmp/ksitool/DEBIAN/control 
sed -i s/@ARCH@/$ARCH/g $deb_dir/tmp/ksitool/DEBIAN/control 
dpkg-deb --build $deb_dir/tmp/ksitool
mv $deb_dir/tmp/ksitool.deb ksitool_${VER}_${ARCH}.deb 

rm -rf $deb_dir/tmp
rm -rf $deb_dir/ksitool/usr
