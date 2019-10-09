# Replaces stderr checks with valgrind check.

/>>>=/{
/#.*>>>=/!{
	/>>>=/i >>>2 \/((LEAK SUMMARY.*)\n(.*definitely lost.*)(.* 0 .*)(.* 0 .*)\n(.*indirectly lost.*)(.* 0 .*)(.* 0 .*)\n(.*possibly lost.*)(.* 0 .*)(.* 0 .*))|(.*All heap blocks were freed.*no leaks are possible.*)\/
}};


