#! /bin/sh

if [ $# -ne 1 ]; then
    echo "usage: `basename $0` <hostfile>"
    exit 1
fi

set -e
set -x

./abtest $1 1 1
./abtest $1 2 2
./abtest $1 $(dcmpi_rand 1 10) $(dcmpi_rand 1 10)
./memtest $1 512k 10
./bwtest $1 1 1000
./bwtest $1 512k 10
./comptest $1
# ./comptest $1 2 2 2
./consoletest $1
./overwritetest $1
for n in 1 2 3 4 5 6 10 30; do
    ./ringtest $1 $n
done
# ./portpolicytest $1
./tickettest $1
./labeltest $1
./multilayouttest $1 1 100
./selfwritetest $1
# ./rcmd $1 hostname # doesn't work on infiniband VAPI
./ddtest $1
./emptybuffertest $1
set +e
./pytest1 $1
./java_test1 $1
./java_test2 $1
./java_test3 $1
./java_test4 $1
