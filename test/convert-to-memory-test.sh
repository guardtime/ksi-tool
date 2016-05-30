#!/bin/bash

#
# GUARDTIME CONFIDENTIAL
#
# Copyright (C) [2016] Guardtime, Inc
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

 if [ "$#" -eq 2 ]; then
 	test_file_in="$1"
 	memory_test_out="$2"
 else
 	echo "Usage $0 <test_file_in> <memory_test_out>"
	exit
 fi

# Delete a text between two markers. First marker is deleted
# the second one is not.
# arg1		- first marker.
# arg2		- last marker.
# arg3		- file to modify.
function delete_from_to_1() {
	sed -i '/.*'$1'.*/{
	:loop
N
s/.*\n//g
/.*'$2'.*/!{
b loop
}
}' $3
}


# Replace the stderr regex in test file. Avoid replacment
# If there is a comment mark at the beginning of the line.
# arg1		- regex for memory test.
# arg2		- file to modify.
function insert_memory_control() {
sed -i '/>>>=/{
/#.*>>>=/!{
	/>>>=/i >>>2 '"$1"'
}
}' $2
}

echo Generating memory test from $test_file_in

cp  $test_file_in $memory_test_out 
delete_from_to_1 '>>>2' '>>>=' $memory_test_out
insert_memory_control "\/((LEAK SUMMARY.*)\n(.*definitely lost.*)(.* 0 .*)(.* 0 .*)\n(.*indirectly lost.*)(.* 0 .*)(.* 0 .*)\n(.*possibly lost.*)(.* 0 .*)(.* 0 .*))|(.*All heap blocks were freed.*no leaks are possible.*)\/" $memory_test_out
sed -i 's/test\/out\/[^\/]*/test\/out\/memory/g' $memory_test_out 



exit $?