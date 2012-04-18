#!/usr/bin/env python
import os, os.path, sys, re, commands, pickle, tempfile, getopt
import socket, string, random, time, traceback, struct

from dcmpi import *

class A(DCFilter):
    def init(self):
        print 'A.init() called!'
    def process(self):
        print 'A.process() called!'
        b = DCBuffer()
        for i in range(100):
            b.pack("idfia", i, float(i), float(i), 3, '\x00\x01\x02')
            print 'used size: ', b.get_used_size()
        self.write(b, "0")
        print 'A.process() finished!'
    def fini(self):
        print 'A.fini() called!'

class Z(DCFilter):
    def process(self):
        print 'Z.process() called!'
        pkts = 0
        b = self.read("0")
        for i in range(100):
            i, f1, f2, rawsz = b.unpack("idfi")
            raw = b.get_raw_extract()
            print 'extracted', i, f1, f2, rawsz
            for x in range(rawsz):
                v = struct.unpack("B", raw[x:x+1])
                print v[0],
            b.increment_extract_ptr(rawsz)
            print
        print 'Z.process() finished!'

class bwfilter(DCFilter):
    def __init__(self):
        self.packet_size = None
        self.num_packets = None
    def init(self):
        self.packet_size = int(self.get_param('packet_size'))
        self.num_packets = int(self.get_param('num_packets'))

class bw1(bwfilter):
    def process(self):
        before = time.time()
        for n in range(self.num_packets):
            b = DCBuffer(self.packet_size)
            b.set_used_size(self.packet_size)
            self.write(b, '0')
        self.read('1')
        after = time.time()
        elapsed = after - before
        bytes = self.packet_size*self.num_packets
        print "MB/sec %4.2f bytes %d bufsize %d reps %d seconds %2.2f" % \
              ((bytes/2**20)/elapsed, bytes,
               self.packet_size, self.num_packets, elapsed)

class bw2(bwfilter):
    def process(self):
        for n in range(self.num_packets):
            b = self.read('0')
        done = DCBuffer()
        done.pack("i", 0)
        self.write(done, "1")

class tocon(DCFilter):
    def init(self):
        print 'hi, inited on', socket.gethostname()
    def process(self):
        out = DCBuffer()
        for i in range(5):
            out.pack('s',`i`)
            out.pack('i',i)
            out.pack('f',i+0.1)
            out.pack('d',i+0.1)
        self.write(out, '0')
