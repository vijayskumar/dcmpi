#!/bin/sh
# set -x

xform () {
    sed "$1" < $2 > $2.tmp && mv $2.tmp $2
}

EchoNoNewline() {
    if [ "`echo -n`" = "-n" ]; then
	echo "$@\c"
    else
	echo -n "$@"
    fi
}

fromdir=`dirname $0`

echo "welcome to dcmpi binary installer"
echo ""
EchoNoNewline "please enter installation directory (default .): "
read instdir
if [ x$instdir = "x" ]; then
    instdir=.
fi
echo ""
if [ -f $fromdir/lib/libdcmpijni.so ]; then 
    javahome=$JAVA_HOME
    if [ x$javahome = "x" ]; then
        javahome=`which java 2>/dev/null`    
	if [ x$javahome != "x" ]; then
            javahome=`dirname $javahome`
            javahome=`dirname $javahome`
	fi
    fi
    msg=""
    if [ x$javahome != "x" ]; then
        msg=" (default $javahome)"
    fi
    EchoNoNewline "please enter java home${msg}: "
    read jdir
    if [ x$jdir = "x" ]; then
        jdir=$javahome
    else
        echo "ERROR: please enter java home"
	exit 1
    fi
    if [ ! -d $jdir ]; then
        echo "ERROR: couldn't find $jdir"
        exit 1
    fi
    xform 's@^java_home.*@java_home = '$jdir'@' $fromdir/lib/dcmpideps.txt
fi

for fn in dcmpiconfig dcmpiremotestartup dcmpiruntime-socket dcmpiruntime; do
    fn2=$fromdir/bin/$fn
    if [ -f $fn2 ]; then
	$fromdir/misc/chrpath -d $fn2
    fi
done

if [ $fromdir != $instdir ]; then
    test -d $instdir || mkdir -p $instdir
    TD=`mktemp -d /tmp/XXXXXX`
    
    cp -r $fromdir/bin $TD
    cp -r $fromdir/lib $TD
    cp -r $fromdir/etc $TD
    cp -r $fromdir/include $TD

    cp -r $TD/bin $instdir
    cp -r $TD/lib $instdir
    cp -r $TD/etc $instdir
    cp -r $TD/include $instdir

    rm -fr $TD
fi

echo "installer exiting."
echo ""
echo "If you've just installed as a normal user, you'll now want to put"
echo "something like the following in your shell startup files:"
echo
echo "export LD_LIBRARY_PATH=`$fromdir/bin/dcmpideps --libdirs |sed 's/ /:/g'`:"'$LD_LIBRARY_PATH'
echo 
echo for bourne shells, or
echo
echo "setenv LD_LIBRARY_PATH `$fromdir/bin/dcmpideps --libdirs |sed 's/ /:/g'`:"'$LD_LIBRARY_PATH'
echo
echo for C shells
echo
echo "If you've installed as root, and you didn't install to a directory"
echo "in /etc/ld.so.conf, you can put these directories in /etc/ld.so.conf"
echo "and re-run ldconfig."

