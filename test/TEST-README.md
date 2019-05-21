# KSI Command-line Tool TEST-README

This document will describe how to properly configure and run the KSI command-line tool tests. Also the dependencies and brief overview of files related to the tests will be described.

Tests can be run with KSI built within the project or with KSI installed on the machine. On Unix platforms KSI binary file is located in `src` and on Windows in `bin` directory. If the platform specific binary exists the tests are run with the corresponding binary, otherwise the installed binary is used.


## DEPENDENCIES

* [shelltestrunner](http://joyful.com/shelltestrunner/) - Mandatory for every test.
* [valgrind](http://valgrind.org/) - For memory tests only, Unix.
* [drmemory](https://github.com/DynamoRIO/drmemory) - For memory tests only, Windows.
* `sed` - For memory tests on Windows (e.g. [Cygwin](https://www.cygwin.com/)).
* [gttlvutil](https://github.com/guardtime/gttlvutil) - When available extra tests for metadata and masking are performed.


## TEST RELATED FILES

All files related to the tests can be found from directory `test` that is located in KSI comman-line tool root directory.

```
 resource         - directory containing all test resource files
                    (e.g. signatures, files to be signed, server responses);
 test_suites      - directory containing all test suites;
 test.cfg.sample  - sample of the configurations file you must created to run tests;
 TEST-README      - the document you are reading right now;
 convert-to-memory-test.sh
                  - helper scrip that converts regular test to valgrind
                    memory test (Unix only). Should not be called by the user as it is used by memory-test.sh internally.
 convert-to-memory-test.bat
                  - helper scrip that converts regular test to drmemory
                    memory test (Windows only). Should not be called by the user as it is used by memory-test.bat internally.
 windows-test.bat - use to run tests on Windows platform.
 test.sh          - use to run tests on Unix platform.
 memory-test.sh   - use to run valgrind memory tests.
 memory-test.bat  - use to run drmemory memory tests.
```

## CONFIGURING TESTS

To configure tests a configuration file must be specified with valid publications file, aggregator and extender URL's with corresponding access credentials. See `test.cfg.sample` and read `ksi conf -h` or `man ksi-conf` for instructions. The file must be located in `test` directory.


## RUNNING TESTS

Tests must be run from KSI comman-line tool root directory and the output is generated to `test/out`. Tests must be run by corresponding test script found from test folder to ensure that test environment is configured properly. The exit code is `0` on success and `1` on failure.

To run tests on Windows, call:
```
test\windows-test.bat
test\memory-test.bat
```

To run tests on RHEL/CentOS:
```
test/test.sh
test/memory-test.sh
```
