#!/bin/bash

 if [ "$#" -eq 6 ]; then
 	input_conf="$1"
	S_embed_user="$2"
	S_embed_key="$3"
	X_embed_user="$4"
	X_embed_key="$5"
	new_scheme="$6"
 else
 	echo "Usage $0 <conf in> <aggr-user> <aggr-key> <ext-user> <ext-key> <new_scheme>"
	echo ""
	echo "  This script is used to generate KSI tool configuration file that has embedded"
	echo "  user info inside aggregator (-S) and extender (-X) URL. Requirements for"
	echo "  original configuration file are:"
	echo "    o URLs have scheme http or https."
	echo "    o URLs contain no embedded user info."
	echo "    o User info is specified explicitly."
	echo ""
	echo " Inputs:"
	echo " <conf in>    - KSI tool configuration file."
	echo " <aggr-user>  - Value of --aggr-user, if is empty string value is extracted from"
	echo "                configuration file."
	echo " <aggr-key>   - Value of --aggr-key, if is empty string value is extracted from"
	echo "                configuration file."
	echo " <ext-user>   - Value of --ext-user, if is empty string value is extracted from"
	echo "                configuration file."
	echo " <ext-key>    - Value of --ext-key, if is empty string value is extracted from"
	echo "                configuration file."
	echo " <new_scheme> - New scheme used for constructed urls (e.g. 'ksi+http://')."
	echo ""
	exit 1
 fi

# Function to extract option value from configuration file.
# fname (e.g test/test.cfg)
# option (e.g. -S)
# return value of option
function extract_option() {
	cat $1 | grep -v "#" | grep -Po "($2) \K.*"
}

# Function to extract URL scheme from strin.
# url (e.g. http://plahh)
# return scheme (e.g. http://)
function extract_scheme() {
	echo $1 | grep -Po ".*//"
}

# Function to extract URL without scheme.
# url (e.g. http://plahh)
# scheme (e.g. http://)
# return URL part after scheme
function extract_host_and_stuff() {
	scheme=$(sed 's/+/./g' <<<"$2")
	echo $1 | grep -Po "($scheme)\K.*"

}

# Extract entire URL.
S_URL=$(extract_option $input_conf "-S")
X_URL=$(extract_option $input_conf "-X")
old_conf_usable_part=$(cat $1 | grep -Ev "(\-X)|(\-S)|(#)|(--aggr-user)|(--aggr-key)|(--ext-user)|(--ext-key)")

# Extract scheme and another part of URL.
S_scheme=$(extract_scheme $S_URL)
S_host_and_stuff=$(extract_host_and_stuff $S_URL $S_scheme)
X_scheme=$(extract_scheme  $X_URL)
X_host_and_stuff=$(extract_host_and_stuff $X_URL $X_scheme)

S_key=$(extract_option $input_conf "--aggr-key")
S_user=$(extract_option $input_conf "--aggr-user")
X_key=$(extract_option $input_conf "--ext-key")
X_user=$(extract_option $input_conf "--ext-user")

# In case of empty string embedded user info is extracted
# from configuration file.

if [ "$S_embed_user" == "" ]; then
  S_embed_user="$S_user"
fi

if [ "$S_embed_key" == "" ]; then
  S_embed_key="$S_key"
fi

if [ "$X_embed_user" == "" ]; then
  X_embed_user="$X_user"
fi

if [ "$X_embed_key" == "" ]; then
  X_embed_key="$X_key"
fi

# Create new URLs.
S_NEW_URL=$(printf '%s%s:%s@%s' $new_scheme $S_embed_user $S_embed_key $S_host_and_stuff)
X_NEW_URL=$(printf '%s%s:%s@%s' $new_scheme $X_embed_user $X_embed_key $X_host_and_stuff)


printf "%s\n -S %s\n -X %s\n" "$old_conf_usable_part" $S_NEW_URL $X_NEW_URL
exit 0
