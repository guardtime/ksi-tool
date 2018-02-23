#!/bin/sh

#
# Copyright 2013-2018 Guardtime, Inc.
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

set -e

libksi_git="https://github.com/guardtime/libksi.git"
libparamset_git="https://github.com/guardtime/libparamset.git"

tmp_build_dir_name="tmp_dep_build"
lib_out_dir="dependencies"
libksi_dir_name="libksi"
libparamset_dir_name="libparamset"


mkdir -p $tmp_build_dir_name
rm -rf $lib_out_dir

cd $tmp_build_dir_name
  git clone $libksi_git $libksi_dir_name 
  git clone $libparamset_git $libparamset_dir_name

  cd $libksi_dir_name
    git checkout v3.17.2693
    ./rebuild.sh
  cd ..

  cd $libparamset_dir_name
    git checkout v1.0.212
    ./rebuild.sh
  cd ..
cd ..


mkdir -p $lib_out_dir/include/ksi
mkdir -p $lib_out_dir/include/param_set
mkdir -p $lib_out_dir/lib

cp $tmp_build_dir_name/$libksi_dir_name/src/ksi/*.h $lib_out_dir/include/ksi/
cp $tmp_build_dir_name/$libksi_dir_name/src/ksi/.libs/libksi.* $lib_out_dir/lib/
cp $tmp_build_dir_name/$libparamset_dir_name/src/param_set/*.h $lib_out_dir/include/param_set/
cp $tmp_build_dir_name/$libparamset_dir_name/src/param_set/.libs/libparamset.* $lib_out_dir/lib/

rm -rf $tmp_build_dir_name
