# Replaces stderr checks with dr Memory.
# />>>=/i >>>2 \/(.*0.*)(.*0.*)(.*invalid heap arguments.*)\n(.*0.*)(.*0.*)(.*warnings.*)\n(.*0.*)(.*0.*)(.*0.*)(.*leak.*)\n(.*0.*)(.*0.*)(.*0.*)(.*possible leak.*)\/
/>>>=/{
/#.*>>>=/!{
	/>>>=/i >>>2 \/(.*NO ERRORS FOUND.*)\/
}};


