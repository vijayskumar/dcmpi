#! /bin/sh

if [ $# -ne 1 ]; then
    echo "usage: `basename $0` <PREFIX>"
    exit 1
fi

set -e
wget http://www.python.org/ftp/python/2.4/Python-2.4.tgz
gunzip -qc Python-2.4.tgz | tar xvf -
cd Python-2.4
./configure --disable-ipv6 --prefix=$1 && make && make install
echo "`basename $0`: Python installed under $1, you probably want to:"
echo "  -add $1/bin to your PATH"
echo "  -add $1/man to your MANPATH"
echo "in your environment."
rm -fr Python-2.4.tgz Python-2.4

