setlocal

IF "%2"=="" (
echo "Usage: $0 <test_file_in> <memory_test_out>"
)
 
 set test_file_in="%1"
 set memory_test_out="%2"
 

 copy  %test_file_in% %memory_test_out%
 
 sed -i -f test\delete-stderr-check.sed %memory_test_out%
 sed -i -f test\replace-with-dr-memory.sed %memory_test_out%
 sed -i -f test\rename-output.sed %memory_test_out%
 sed -i  "s/^[^#]*>>>=.*/>>>= 0/" %memory_test_out%
 
 set exit_code=%errorlevel%
 
 
endlocal
rem exit /B %exit_code%
