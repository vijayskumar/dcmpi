#! /bin/sh

if [ $# -gt 1 ]; then
    echo "usage: `basename $0` [PREFIX]"
    exit 1
fi

F=scons-0.96.1.tar.gz
D=scons-0.96.1

set -e
wget http://bmi.osu.edu/~rutt/pythoninst/scons-0.96.1.tar.gz
gunzip -qc $F | tar xvf -
cd $D
if [ $# -eq 0 ]; then
    python setup.py install
else
    python setup.py install --prefix=$1
fi
echo "`basename $0`: scons installed"
cd ..
rm -fr $F $D

