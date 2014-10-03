cd ../src

# test configuration
WAIT=5

SIG_SERV_IP=http://192.168.100.36:3333/
PUB_SERV_IP=http://verify.guardtime.com/ksi-publications.bin
VER_SERV_IP=http://192.168.100.36:8081/gt-extendingservice
SERVICES="-S $SIG_SERV_IP  -X $VER_SERV_IP  -P $PUB_SERV_IP -C 25 -c 25"
echo $SERVICES

# input files
TEST_FILE=../test/testFile
TEST_OLD_SIG=../test/ok-sig-2014-04-30.1.ksig
SH256_DATA_FILE=../test/data_sh256.txt
INVALID_PUBFILE=../test/nok_pubfile

# input hash
SH1_HASH=bf9661defa3daecacfde5bde0214c4a439351d4d
SH256_HASH=c8ef6d57ac28d1b4e95a513959f5fcdd0688380a43d601a5ace1d2e96884690a
RIPMED160_HASH=0a89292560ae692d3d2f09a3676037e69630d022

# output files
TEST_EXTENDED_SIG=../test/out/extended.ksig
SH1_file=../test/out/sh1.ksig
SH256_file=../test/out/SH256.ksig
RIPMED160_file=../test/out/RIPMED160.ksig
TEST_FILE_OUT=../test/out/testFile
PUBFILE=../test/out/pubfile

CERTS=""

GLOBAL="$CERTS $SERVICES"
SIGN_FLAGS="-n -r -d -t"
VERIFY_FLAGS="-n -r -d -t" 
EXTEND_FLAGS="-t"


rm -rf ../test/out
mkdir -p ../test/out

echo "****************** Download publications file ******************"
./gtime -p -t $GLOBAL -o "$PUBFILE" -d 
echo $?
./gtime -v -t $GLOBAL -b "$PUBFILE"
echo $?

echo "****************** Get Publication string ******************" 
./gtime -p -t $GLOBAL -T 1410848909
echo $?


echo "****************** Sign data [-n] ******************"
./gtime -s $GLOBAL $SIGN_FLAGS -f "$TEST_FILE" -o "$TEST_FILE_OUT".ksig -b $PUBFILE 
echo $?
sleep $WAIT 

echo "****************** Verify online ******************"
./gtime -v -x $GLOBAL $VERIFY_FLAGS -i "$TEST_FILE_OUT".ksig
echo $?

echo "****************** Verify using publications file ******************"
./gtime -v $GLOBAL $VERIFY_FLAGS -i "$TEST_FILE_OUT".ksig -b $PUBFILE
echo $?

echo "****************** Sign data with algorithm [-H SH-1] ******************" 
./gtime -s $GLOBAL $SIGN_FLAGS -f "$TEST_FILE" -o "$TEST_FILE_OUT"SH-1.ksig  -H SHA-1
echo $?
sleep $WAIT
./gtime -v -x $GLOBAL $VERIFY_FLAGS -i "$TEST_FILE_OUT"SH-1.ksig
echo $?

echo "****************** Verifying signature and file [-t]******************" 
./gtime -v -x $GLOBAL $VERIFY_FLAGS -i "$TEST_FILE_OUT".ksig -f "$TEST_FILE"
echo $?


echo "****************** Extend old signature [-t] ******************" 
./gtime -x $GLOBAL $EXTEND_FLAGS -i "$TEST_OLD_SIG" -o "$TEST_EXTENDED_SIG"
echo $?
sleep $WAIT
./gtime -v -x $GLOBAL $VERIFY_FLAGS -i "$TEST_EXTENDED_SIG"
echo $?

echo "****************** Sign raw hash with algorithm specified [-F SH1:<hash>] ******************" 
./gtime -s $GLOBAL $SIGN_FLAGS -o "$SH1_file"  -F SHA-1:"$SH1_HASH"
echo $?
sleep $WAIT
./gtime -v -x $GLOBAL $VERIFY_FLAGS -i "$SH1_file"
echo $?

echo "****************** Sign raw hash with algorithm specified [-F SH256:<hash>] ******************" 
./gtime -s $GLOBAL $SIGN_FLAGS -o "$SH256_file" -F SHA-256:"$SH256_HASH"
echo $?
sleep $WAIT
./gtime -v -x $GLOBAL $VERIFY_FLAGS -i "$SH256_file" -f "$SH256_DATA_FILE"
echo $?

echo "****************** Sign raw hash with algorithm specified [-F RIPMED160:<hash>] ******************" 
./gtime -s $GLOBAL $SIGN_FLAGS -o "$RIPMED160_file" -F RIPEMD-160:"$RIPMED160_HASH"
echo $?
sleep $WAIT
./gtime -v -x $GLOBAL $VERIFY_FLAGS -i "$RIPMED160_file"
echo $?

echo "****************** Test include. Must show ignored parameters and fail ******************" 
./gtime -inc ../test/conf1 -inc ../test/conf3
echo $?





echo "****************** Error extend no suitable publication ******************" 
./gtime -x $GLOBAL $EXTEND_FLAGS -i "$TEST_FILE_OUT".ksig -o "$TEST_EXTENDED_SIG"
echo $?

echo "****************** Error extend not suitable format ******************" 
./gtime -x $GLOBAL $EXTEND_FLAGS -i "$TEST_FILE" -o "$TEST_EXTENDED_SIG"
echo $?

echo "****************** Error verify not suitable format ******************" 
./gtime -v -x $GLOBAL $VERIFY_FLAGS -i "$TEST_FILE" 
echo $?

echo "****************** Error verifying signature and wrong file ******************" 
./gtime -v -x $GLOBAL $VERIFY_FLAGS -i "$TEST_FILE_OUT".ksig -f "$TEST_FILE_OUT".ksig
echo $?

echo "****************** Error signing with SH1 and wrong hash ******************" 
./gtime -s $GLOBAL $SIGN_FLAGS -o "$SH1_file"  -F SHA-1:"$SH1_HASH"ff
echo $?

echo "****************** Error signing with unknown algorithm and wrong hash ******************" 
./gtime -s $GLOBAL $SIGN_FLAGS -o "$TEST_FILE"  -F _UNKNOWN:"$SH1_HASH"
echo $?

echo "****************** Error bad network provider******************" 
./gtime -s $SIGN_FLAGS -o "$SH1_file"  -F SHA-1:"$SH1_HASH" -S plaplaplaplpalpalap
echo $?




echo "****************** Error Verify signature and missing file ******************" 
./gtime -v -x $GLOBAL $VERIFY_FLAGS -i "$TEST_FILE_OUT".ksig -f missing_file
echo $?

echo "****************** Error missing cert files ******************" 
./gtime -p -o $PUBFILE -V missing1 -V missing2 -V missing 3 
echo $?

echo "****************** Error Invalid publications file ******************"
./gtime -v -t $GLOBAL $VERIFY_FLAGS -i "$TEST_FILE_OUT".ksig -b "$INVALID_PUBFILE"
echo $?

echo "****************** Error Unable to Get Publication string ******************" 
./gtime -p $GLOBAL -T 969085709
echo $?

echo "****************** Error wrong E-mail******************"
./gtime -p -t $GLOBAL -o "$PUBFILE"  -E magic@email.null 
echo $?
