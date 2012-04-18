#!/usr/bin/env python
import os, os.path, sys, re, commands, pickle, tempfile, getopt
import socket, string, random, time, traceback, shutil

for path in commands.getoutput("dcmpideps --pythonpath").split(':'):
    sys.path.insert(0, path)

from dcmpilayout import *

# arg is a number, or a thing that looks like a number
def csnum(arg):
    float_regexp = "-?([0-9]+(\\.[0-9]*)?|\\.[0-9]+)"
    re_kb = '^' + float_regexp + '[kK]$'
    re_mb = '^' + float_regexp + '[mM]$'
    re_gb = '^' + float_regexp + '[gG]$'
    re_tb = '^' + float_regexp + '[tT]$'
    re_pb = '^' + float_regexp + '[pP]$'
    re_eb = '^' + float_regexp + '[eE]$'
    if (re.match(re_kb, arg)):
        return int(round(float(re.sub(re_kb, r'\1', arg)) * (2**10)))
    elif (re.match(re_mb, arg)):
        return int(round(float(re.sub(re_mb, r'\1', arg)) * (2**20)))
    elif (re.match(re_gb, arg)):
        return int(round(float(re.sub(re_gb, r'\1', arg)) * (2**30)))
    elif (re.match(re_tb, arg)):
        return int(round(float(re.sub(re_tb, r'\1', arg)) * (2**40)))
    elif (re.match(re_pb, arg)):
        return int(round(float(re.sub(re_pb, r'\1', arg)) * (2**50)))
    elif (re.match(re_eb, arg)):
        return int(round(float(re.sub(re_pb, r'\1', arg)) * (2**60)))
    elif (re.match('^[0-9]+', arg)):
        return int(arg)
    else:
        raise ValueError("ERROR:  invalid number " + arg)

def usage():
    s = 'usage: ' + os.path.basename(sys.argv[0]) + ' <hostfile> <packet_size> <num_packets>\n'
    sys.stderr.write(s)
    sys.exit(1)

if ((len(sys.argv)-1) != 3):
    usage()

layout = DCLayout()
layout.use_filter_library('libbwfilters.so')
layout.set_param_all("packet_size", `csnum(sys.argv[2])`)
layout.set_param_all("num_packets", `csnum(sys.argv[3])`)
bw1 = DCFilterInstance("bw1", "bw1")
bw2 = DCFilterInstance("bw2", "bw2")
layout.add(bw1)
layout.add(bw2)
layout.add_port(bw1, "0", bw2, "0")
layout.add_port(bw2, "1", bw1, "1")
layout.set_exec_host_pool_file(sys.argv[1])
layout.execute()
