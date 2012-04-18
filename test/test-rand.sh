#! /bin/sh

if [ $# -ne 1 ]; then
    echo "usage: `basename $0` <hostfile>"
    exit 1
fi

set -e
set -x

while true; do
    ./abtest $1 $(dcmpi_rand 1 20) $(dcmpi_rand 1 20)
    ./bwtest $1 $(dcmpi_rand 1 10000) $(dcmpi_rand 1 10000)
    ./memtest $1 $(dcmpi_rand 1 10000) $(dcmpi_rand 1 10000)
    ./comptest $1 $(dcmpi_rand 1 5) $(dcmpi_rand 1 5) $(dcmpi_rand 1 5)
    ./ringtest $1 $(dcmpi_rand 1 100)
    $(which mpiexec >/dev/null 2>&1) || ./multilayouttest $1 $(dcmpi_rand 1 10000) $(dcmpi_rand 1 10000)
done
