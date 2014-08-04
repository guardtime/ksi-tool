cd ../src

# test configuration
WAIT=5

SIG_SERV_IP=http://192.168.1.36:3333/
# VER_SERV_IP=http://192.168.1.29:1111/gt-extendingservice
PUB_SERV_IP=http://172.20.20.7/publications.tlv
# SIG_SERV_IP=http://172.20.20.4:3333/
 VER_SERV_IP=http://192.168.1.36:8081/gt-extendingservice
SERVICES="-S $SIG_SERV_IP  -X $VER_SERV_IP  -P $PUB_SERV_IP"
echo $SERVICES

# input files
TEST_FILE=../test/testFile
TEST_OLD_SIG=../test/ok-sig-2014-04-30.1.ksig
SH256_DATA_FILE=../test/data_sh256.txt
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

rm -rf ../test/out
mkdir -p ../test/out

echo "****************** Download publications file ******************"
./gtime -p $SERVICES -t -o "$PUBFILE" 
./gtime -v $SERVICES -t -b "$PUBFILE" 

echo "****************** Sign data [-n] ******************"
./gtime -s $SERVICES  -f "$TEST_FILE"  -o "$TEST_FILE_OUT".ksig  -n 
sleep $WAIT 
./gtime -v -t $SERVICES  -x -i "$TEST_FILE_OUT".ksig -n

echo "****************** Sign data with algorithm [-H SH-1] ******************" 
./gtime -s $SERVICES -f "$TEST_FILE" -o "$TEST_FILE_OUT"SH-1.ksig  -H SHA-1
echo $?
sleep $WAIT
./gtime -v $SERVICES -x -i "$TEST_FILE_OUT"SH-1.ksig
echo $?

echo "****************** Verifying signature and file [-t]******************" 
./gtime -v -t $SERVICES -x -i "$TEST_FILE_OUT".ksig -f "$TEST_FILE"
echo $?

echo "****************** Verify signature and missing file [-t] ******************" 
./gtime -v -t $SERVICES -x -i "$TEST_FILE_OUT".ksig -f missing_file
echo $?

echo "****************** Extend old signature [-t] ******************" 
./gtime -x -t $SERVICES -i "$TEST_OLD_SIG" -o "$TEST_EXTENDED_SIG"
echo $?
sleep $WAIT
./gtime -v $SERVICES -x -i "$TEST_EXTENDED_SIG"
echo $?

echo "****************** Sign raw hash with algorithm specified [-F SH1:<hash>] ******************" 
./gtime -s $SERVICES -o "$SH1_file"  -F SHA-1:"$SH1_HASH"
echo $?
sleep $WAIT
./gtime -v $SERVICES -x -i "$SH1_file"
echo $?

echo "****************** Sign raw hash with algorithm specified [-F SH256:<hash>] ******************" 
./gtime -s $SERVICES -o "$SH256_file" -F SHA-256:"$SH256_HASH"
echo $?
sleep $WAIT
./gtime -v $SERVICES -x -i "$SH256_file" -f "$SH256_DATA_FILE"
echo $?

echo "****************** Sign raw hash with algorithm specified [-F RIPMED160:<hash>] ******************" 
./gtime -s $SERVICES -o "$RIPMED160_file" -F RIPEMD-160:"$RIPMED160_HASH"
echo $?
sleep $WAIT
./gtime -v $SERVICES -x -i "$RIPMED160_file"
echo $?

echo "****************** Error extend no suitable publication ******************" 
./gtime -x -t $SERVICES -i "$TEST_FILE_OUT".ksig -o "$TEST_EXTENDED_SIG"
echo $?

echo "****************** Error extend not suitable format ******************" 
./gtime -x -t $SERVICES -i "$TEST_FILE" -o "$TEST_EXTENDED_SIG"
echo $?

echo "****************** Error verify not suitable format ******************" 
./gtime -v -x -t $SERVICES -i "$TEST_FILE" 
echo $?

echo "****************** Error verifying signature and wrong file ******************" 
./gtime -v -t $SERVICES -x -i "$TEST_FILE_OUT".ksig -f "$TEST_FILE_OUT".ksig
echo $?

echo "****************** Error signing with SH1 and wrong hash ******************" 
./gtime -s $SERVICES -o "$SH1_file"  -F SHA-1:"$SH1_HASH"ff
echo $?

echo "****************** Error signing with unknown algorithm and wrong hash ******************" 
./gtime -s $SERVICES -o "$TEST_FILE"  -F _UNKNOWN:"$SH1_HASH"
echo $?

echo "****************** Error bad network provider******************" 
./gtime -s -o "$SH1_file"  -F SHA-1:"$SH1_HASH" -S plaplaplaplpalpalap
echo $?

echo "****************** Error no references -r ******************"
./gtime -v -t $SERVICES  -x -i "$TEST_FILE_OUT".ksig -f "$TEST_FILE" -r
echo $?
