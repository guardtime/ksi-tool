 if [ "$#" -eq 2 ]; then
 	test_file_in="$1"
 	memory_test_out="$2"
 else
 	echo "Usage $0 <test_file_in> <memory_test_out>"
	exit
 fi
 
memory_control="\/((LEAK SUMMARY.*)\n(.*definitely lost.*)(.* 0 .*)(.* 0 .*)\n(.*indirectly lost.*)(.* 0 .*)(.* 0 .*)\n(.*possibly lost.*)(.* 0 .*)(.* 0 .*))|(.*All heap blocks were freed.*no leaks are possible.*)\/"
echo $test_file_in
echo $memory_test_out

cp  $test_file_in $memory_test_out 
sed -i '/\(>>>=\|EXECUTABLE\)/!d'  $memory_test_out 
sed -i '/EXECUTABLE.*/a >>>2 '"${memory_control}" $memory_test_out 
sed -i '/>>>=/a \\n' $memory_test_out
sed -i 's/test\/out\/[^\/]*/test\/out\/memory/g' $memory_test_out 
#sed -i 's/test\/out\/.*\//test\/out\/memory\//' $memory_test_out 
#All heap blocks were freed -- no leaks are possible

exit $?