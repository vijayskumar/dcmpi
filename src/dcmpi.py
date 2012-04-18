#!/usr/bin/env python
from __future__ import generators

import os, os.path, sys, re, commands, pickle, tempfile, getopt
import socket, string, random, time, traceback

import dcmpi_pynativedefs as native


class DCBuffer:
    def _handoff(self, peer):
        self.peer = peer
    def __init__(self, initial_size=None):
        self.peer = None
        if initial_size == 'internal-disable-peer':
            pass
        else:
            if not initial_size:
                initial_size = -1
            self.peer = native.DCBuffer_new(initial_size)
        self.pack_map = \
                      {'i' : (native.DCBuffer_appendint, int),
                       's' : (native.DCBuffer_appendstr, str),
                       'f' : (native.DCBuffer_appendfloat, float),
                       'd' : (native.DCBuffer_appenddouble, float),
                       'a' : (native.DCBuffer_appendbytearray, str)}
        self.unpack_map = \
                        {'i' : native.DCBuffer_extractint,
                         's' : native.DCBuffer_extractstr,
                         'f' : native.DCBuffer_extractfloat,
                         'd' : native.DCBuffer_extractdouble}
    def pack(self, format, *values):
        idx = 0
        if len(format) != len(values):
            raise Exception("ERROR: format string length != values length")
        for letter in format:
            if not self.pack_map.has_key(letter):
                raise Exception("ERROR: unsupported format letter: %s" % \
                                (letter))
            value = values[idx]
            func = self.pack_map[letter][0]
            forcetype = self.pack_map[letter][1]
            if not type(value) is forcetype:
                raise Exception('invalid type of value', type(value))
            func(self.peer, value)
            idx += 1
    def unpack(self, format):
        out = []
        for letter in format:
            if self.unpack_map.has_key(letter):
                out.append(self.unpack_map[letter](self.peer))
            else:
                raise Exception("ERROR: unsupported format letter: %s" %\
                                (letter))
        return tuple(out)
    def get_raw(self):
        return native.DCBuffer_getraw(self.peer)
    def get_raw_extract(self):
        return native.DCBuffer_getrawextract(self.peer)
    def set_used_size(self, length):
        return native.DCBuffer_setusedsize(self.peer, length)
    def get_used_size(self):
        return native.DCBuffer_getusedsize(self.peer)
    def increment_extract_ptr(self, length):
        return native.DCBuffer_increment_extract_ptr(self.peer, length)

class DCFilter:
    def __init__(self):
        self.peer = None
    def _handoff(self, peerptr):
        self.peer = peerptr

    # main 3 methods to be overridden in subclasses
    def init(self):
        pass
    def process(self):
        sys.stderr.write("WARNING: python filter named %s has no defined process method?\n" % self.__class__.__name__)
    def fini(self):
        pass

    def get_param(self, key):
        return native.DCFilter_get_param(self.peer, key)
    def read_nonblocking(self, port):
        buf_peer = native.DCFilter_read_nonblocking(self.peer, port)
        out = None
        if buf_peer:
            out = DCBuffer('internal-disable-peer')
            out._handoff(buf_peer)
        return out
    def read(self, port):
        buf_peer = native.DCFilter_read(self.peer, port)
        out = DCBuffer('internal-disable-peer')
        out._handoff(buf_peer)
        return out
    def readbufs(self, port): # python generator
        while 1:
            buf_peer = native.DCFilter_read_until_upstream_exit(self.peer, port)
            if not buf_peer:
                return
            else:
                out = DCBuffer('internal-disable-peer')
                out._handoff(buf_peer)
                yield out
    def write(self, buffer, port, label=""):
        native.DCFilter_write(self.peer, buffer.peer, port, label)

if __name__ == "__main__": # when run as a script
    b = DCBuffer()
    for i in range(100):
        b.pack("i", i)
        print b.unpack("i")
