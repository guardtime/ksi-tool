#!/bin/bash

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

 if [ "$#" -eq 2 ]; then
 	test_file_in="$1"
 	memory_test_out="$2"
 else
 	echo "Usage $0 <test_file_in> <memory_test_out>"
	exit
 fi


echo Generating memory test from $test_file_in

cp  $test_file_in $memory_test_out 
sed -i -f test/delete-stderr-check.sed $memory_test_out
sed -i -f test/replace-with-valgrind.sed $memory_test_out
sed -i -f test/rename-output.sed $memory_test_out


exit $?