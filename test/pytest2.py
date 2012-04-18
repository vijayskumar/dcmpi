#!/usr/bin/env python
import os, os.path, sys, commands

for path in commands.getoutput("dcmpideps --pythonpath").split(':'):
    sys.path.insert(0, path)

from dcmpilayout import *

def usage():
    s = 'usage: ' + os.path.basename(sys.argv[0]) + ' <hostfile>\n'
    sys.stderr.write(s)
    sys.exit(1)

if ((len(sys.argv)-1) != 1):
    usage()

layout = DCLayout()
layout.use_filter_library('pyfilters.py')
A = DCFilterInstance("A", "A1");
Z = DCFilterInstance("Z", "Z1");
layout.add(A);
layout.add(Z);
layout.add_port(A, "0", Z, "0");
layout.set_exec_host_pool_file(sys.argv[1]);
layout.execute()
