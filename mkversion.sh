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

if [ ! -f VERSION.input ]; then
    if [ ! -f VERSION ]; then
		echo "Error: File 'VERSION.input' nor 'VERSION' exists!" 1>&2
		echo "Define 'VERSION.input' as <minor>.<major> to generate file 'VERSION' with git as <major>.<minor>.<build>." 1>&2
		echo "Define 'VERSION' as <minor>.<major>.<build>." 1>&2
		exit 1
	fi
else
	VER=$(tr -d [:space:] < VERSION.input)
	if hash git 2>/dev/null; then
		BUILD=$(git rev-list HEAD | wc -l)
	else
		echo "Error: Git is not installed. Unable to generate build number. Build number is set to 0." 1>&2
		BUILD="0"
	fi

	echo ${VER}.${BUILD} > VERSION
fi
