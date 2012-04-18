#!/usr/bin/env python
import os, os.path, sys, re, commands, pickle, tempfile, getopt
import socket, string, random, time, traceback, inspect, types

# import dcmpi

def load_filter(classname, modfile):
    out = None
    try:
#         print 'load_filter(%s,%s)' % (classname, modfile)
        loaddir = os.path.dirname(modfile)
#         print 'inserting', loaddir
        if loaddir not in sys.path:
            sys.path.insert(0, loaddir)
#             print 'sys.path now', sys.path
        modname = os.path.basename(modfile).split('.py')[0]
        txt = 'import ' + modname
#         print txt
        exec(txt)
        out = eval(modname + "." + classname + '()', globals(), locals())
    except Exception:
        traceback.print_exc()        
    return out

def getparentclasses(cls):
    """Return list of immediate parent classes for input class object
    cls.  Returns empty list if there are none."""
    if not isinstance(cls, types.ClassType):
        raise TypeError
    return list(cls.__bases__)

def getrootclasses(cls):
    """Return list of root ancestor classes for input class object cls."""
    if not isinstance(cls, types.ClassType):
        print cls
        raise TypeError
    parents = getparentclasses(cls)
    if parents == []:
        return [cls]
    else:
        out = []
        for par in parents:
            out.extend(getrootclasses(par))
        return out

def scan_library_for_filters(modfile):
    assert modfile[0] == '/'
    loaddir = os.path.dirname(modfile)
    if loaddir not in sys.path:
        sys.path.insert(0, loaddir)
    modname = os.path.basename(modfile).split('.py')[0]
    txt = 'import ' + modname
    exec(txt)
    mod = eval(modname, globals(), locals())
    classes = [x[1] for x in inspect.getmembers(mod, inspect.isclass)]
    out = []
    for cl in classes:
        if cl.__name__ == 'DCFilter':
            pass
        else:
            roots = getrootclasses(cl)
            for root in roots:
                if root.__name__ == 'DCFilter':
                    out.append(cl.__name__)
    return out
