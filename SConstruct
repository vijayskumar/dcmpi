import os, os.path, commands, re
import sys, string, stat

srcdir = os.path.abspath('src')

def DumpEnv( env, key = None, header = None, footer = None ):
    import pprint
    pp = pprint.PrettyPrinter( indent = 2 )
    if key:
        dict = env.Dictionary( key )
    else:
        dict = env.Dictionary()
    if header:
        print header
    pp.pprint( dict )
    if footer:
        print footer

def which(filename):
    if not os.environ.has_key('PATH') or os.environ['PATH'] == '':
        p = os.defpath
    else:
        p = os.environ['PATH']
    pathlist = string.split(p, os.pathsep)
    for path in pathlist:
        f = os.path.join(path, filename)
        if os.access(f, os.X_OK) and not stat.S_ISDIR(os.stat(f)[stat.ST_MODE]):
            return f
    return None

opts = Options('config.py')
opts.AddOptions(
    ('debug', 'Set to 1 to build a debug build', 0),
    ('debug_flags', 'Set to use these flags during debug build', None),
    ('mpi_cxx_compiler', 'Set to force mpi C++ compiler (e.g. mpicxx)', 'search for it'),
    ('prefix', 'Set to install prefix', '/usr/local'),
    ('gprof', 'Set to 1 to build with gprof flags', 0),
    ('lfs', 'Set to 1 to build with large file support', 0),
    ('pcre_path', 'Set to the install prefix path containing pcre (optional)', None),
    ('use_java', 'Set to 1 to use java (in your JAVA_HOME or PATH)', 0),
    ('use_python', 'Set to 1 to use python filters (uses python in your PATH)', 0),
    ('use_swig', 'Set to 1 to use swig to build layout API for java, python', 0),
    ('use_origin', 'Set to 1 to use the $ORIGIN/../lib mechanism for locating shared libs', 0),
    ('build_appconfig', 'Set to 1 to build app config layer', 0),
    ('extraincludes', 'List any additional include paths here', None),
    ('extralibs', 'List any additional link libraries here', None),
    ('extralibpath', 'List any additional link paths here', None),
    ('extradefines', 'List any additional preprocessor defines here', None),
    ('extrarpath', 'List any additional RPATH link directories here', None),
    ('build_testing', 'Build test programs', 1),
    ('CXX', 'Forces C++ compiler', None),
    ('CXXFLAGS', 'Forces C++ compiler flags', None),
    ('LINKFLAGS', 'Forces linker flags', None),
    )

env = Environment(ENV=os.environ, options=opts)
env.SConsignFile()

mpi_cxx_compiler = env.get('mpi_cxx_compiler')
debug_mode = int(env.get('debug'))
debug_flags = env.get('debug_flags')
gprof_mode = int(env.get('gprof'))
defines = []
libs = []
large_file_support = int(env.get('lfs'))
pcre_path = env.get('pcre_path')
use_java = int(env.get('use_java'))
env['use_java'] = use_java
use_python = int(env.get('use_python'))
use_swig = int(env.get('use_swig'))
env['use_swig'] = use_swig
use_origin = int(env.get('use_origin'))
install_prefix = env.get('prefix')
extraincludes = env.get('extraincludes')
extralibs = env.get('extralibs')
extralibpath = env.get('extralibpath')
extradefines = env.get('extradefines')
extrarpath = env.get('extrarpath')
build_testing = env.get('build_testing')
build_appconfig = int(env.get('build_appconfig'))

if use_origin:
    env.Append(LINKFLAGS = Split('-z origin') )
    env.Append(RPATH=[env.Literal(os.path.join('\\$$ORIGIN', os.pardir, 'lib'))])
    env.Append(RPATH=[env.Literal('\\$$ORIGIN')])

if extraincludes:
    env.Append(CPPPATH=extraincludes.split(':'))
if extralibs:
    added = extralibs.split(' ')
    libs.extend(added)
if extralibpath:
    env.Append(LIBPATH=extralibpath.split(':'))
if extradefines:
    defines.extend(extradefines.split(' '))
if extrarpath and sys.platform != 'darwin':
    env.Append(RPATH=extrarpath.split(':'))
elif extrarpath:
    sys.stderr.write("WARNING: RPATH not supported on Mac OS X\n")
    
print 'C++ compiler set to', env['CXX']

if mpi_cxx_compiler != 'search for it':
    pass
else:
    mpi_cxx_compiler = None
    # search for known good MPI C++ compilers
    options = ['mpicxx','mpiCC','mpic++']
    for o in options:
        if WhereIs(o) != None:
            mpi_cxx_compiler = WhereIs(o)
            break

if mpi_cxx_compiler == 'None':
    mpi_cxx_compiler = None

if mpi_cxx_compiler and (mpi_cxx_compiler.find('/') == -1 and \
                         which(mpi_cxx_compiler) == None):
    sys.stderr.write("WARNING: MPI C++ compiler %s not found in PATH\n" % (mpi_cxx_compiler))
    mpi_cxx_compiler = None

if not mpi_cxx_compiler:
    print "MPI C++ compiler not found.  Proceeding to build without MPI."
else:
    sys.stdout.write('MPI C++ compiler set to %s ' % (mpi_cxx_compiler))
    if mpi_cxx_compiler.find('/') == -1:
        print '(%s)' % (which(mpi_cxx_compiler))
    else:
        print
    defines.append('DCMPI_HAVE_MPI')
        
if debug_mode:
    print 'building with debug'
    env['CXXFLAGS'] = env['CXXFLAGS'] + ' -g'
    defines.append('DEBUG')
    if debug_flags:
        print 'building with debug flags of', debug_flags
        env['CXXFLAGS'] = env['CXXFLAGS'] + ' ' + debug_flags
else:
    print 'building optimized version'
    env['CXXFLAGS'] = env['CXXFLAGS'] + ' -O3'
    env['CXXFLAGS'] = env['CXXFLAGS'] + ' -DNDEBUG'

if gprof_mode:
    env['CXXFLAGS'] = env['CXXFLAGS'] + ' -pg'
    env['LINKFLAGS'] = env['LINKFLAGS'] + ' -pg'

if debug_mode:
    env['CXXCOM'] = string.replace(env['CXXCOM'],'$SOURCES','$SOURCES.abspath')
    env['SHCXXCOM'] = string.replace(env['SHCXXCOM'],'$SOURCES','$SOURCES.abspath')

if pcre_path:
    libpath = pcre_path + '/lib'
    incpath = pcre_path + '/include'
    env.Append(RPATH=[libpath])
    env.Append(CPPPATH=[incpath])
    env.Append(LIBPATH=[libpath])
    env.Append(LIBS=['pcre'])

java_depstate = {}
if use_java:
    if os.getenv('JAVA_HOME'):
        java_exe = os.path.join(os.getenv('JAVA_HOME'), 'bin','java')
    else:
        java_exe = which('java')
    print 'building with support for java filters (using java of %s)' % (java_exe)
    env.Append(CPPDEFINES=['DCMPI_HAVE_JAVA'])
    java_container = os.path.dirname(os.path.dirname(java_exe))
    java_depstate['home'] = java_container
    bindir = os.path.join(java_container, 'bin')
    env['JAVAC'] = os.path.join(bindir, 'javac')
    env['JAVAH'] = os.path.join(bindir, 'javah')
    if os.uname()[0] == 'Linux':
        d = ['include',os.path.join('include','linux')]
        java_depstate['include_extends'] = d
        env.Append(CPPPATH=[os.path.join(java_container, x) for x in d])
    elif os.uname()[0] == 'Solaris':
        d = ['include',os.path.join('include','solaris')]
        java_depstate['include_extends'] = d
        env.Append(CPPPATH=[os.path.join(java_container, x) for x in d])
    elif os.uname()[0] == 'Darwin':
        d = ['/System/Library/Frameworks/JavaVM.framework/Headers']
        env.Append(CPPPATH=d)
    else:
        sys.stderr.write("ERROR (jni): unknown arch %s" % (os.uname()[0]))
        sys.exit(1)

    libpaths = []
    if os.uname()[0] == 'Darwin':
        d = ['/System/Library/Frameworks/JavaVM.framework/Libraries']
        env.Append(LIBPATH=d)
    else:
        jredir = os.path.join(java_container, 'jre')
        if os.path.exists(jredir + '/lib/i386'):
            d = [os.path.join('jre','lib','i386'),
                 os.path.join('jre','lib','i386','server')]
            java_depstate['library_extends'] = d
            env.Append(LIBPATH=[os.path.join(java_container, x) for x in d])
            env.Append(RPATH=[os.path.join(java_container, x) for x in d])
        elif os.path.exists(jredir + '/lib/amd64'):
            d = [os.path.join('jre','lib','amd64'),
                 os.path.join('jre','lib','amd64','server')]
            java_depstate['library_extends'] = d
            env.Append(LIBPATH=[os.path.join(java_container, x) for x in d])
            env.Append(RPATH=[os.path.join(java_container, x) for x in d])
        else:
            sys.stderr.write("ERROR: unknown architecture, cannot find %s\n" % \
                             (jredir + '/lib/XXX'))
            sys.exit(1)
    env.Append(LIBS=['jvm'])

if use_python:    
    print 'building with support for python filters (using python of %s)' % \
          (which('python'))
    pyver = commands.getoutput("""python -c 'import sys; print "%d.%d" % (sys.version_info[0], sys.version_info[1])'""")
    print '--> python version is', pyver

    # include Python.h directory
    cmd = """python -c 'import distutils.sysconfig; print "-I" + distutils.sysconfig.get_config_var("INCLUDEPY")'"""
    print '--> python include info is', commands.getoutput(cmd)
    env.ParseConfig(cmd)

    # link to python itself
    env.Append(LIBS='python' + pyver)
    cmd = """python -c 'import distutils.sysconfig; print "-L%s" % distutils.sysconfig.get_config_var("LIBPL")'"""
    print '--> python link dir is', commands.getoutput(cmd)
    env.ParseConfig(cmd)

    # link to python's deps
    cmd = """python -c 'import distutils.sysconfig; print distutils.sysconfig.get_config_var("LOCALMODLIBS"), distutils.sysconfig.get_config_var("BASEMODLIBS"), distutils.sysconfig.get_config_var("LIBS")'"""
    print '--> python external link info is', commands.getoutput(cmd)
    env.ParseConfig(cmd)

    env.Append(CPPDEFINES=['DCMPI_HAVE_PYTHON'])

context = env.Configure()

# check for pthread
pthread_body = """
#include <pthread.h>
int main() {
    pthread_t thr;
    return 0;
}
"""
has_pthread = context.TryLink(pthread_body, '.cpp')
if not has_pthread:
    raise Exception("cannot find pthread, or cannot compile the file:\n" +
                pthread_body + "...aborting (check config.log for details).")
    sys.exit(1)

# check for sstream and generate typedefs, creating dcmpi_typedefs.h
typedef_text = "/* NOTE:  machine generated file, do not edit! */\n"
typedef_text = typedef_text + "#ifndef DCMPI_TYPEDEFS_H\n#define DCMPI_TYPEDEFS_H\n\n"
has_newer_sstream = context.TryLink("""
#include <sstream>
using namespace std;
int main() {
    char buf[1024];
    istringstream iss(buf);
    ostringstream oss;
    return 0;
}
""", '.cpp')
if has_newer_sstream:
   typedef_text = typedef_text + """#include <sstream>
typedef std::ostringstream ostr;
typedef std::istringstream istr;
"""
else:
   typedef_text = typedef_text + """#include <strstream.h>
typedef ostrstream ostr;
typedef istrstream istr;
"""

(ok, integral_type_text) = context.TryRun("""
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define SIGNED   1
#define UNSIGNED 0

const char * getMatchingType(int bytes, int signed_type)
{
    if (signed_type) {
        if (sizeof(signed char) == bytes)
            return "signed char";
        else if (sizeof(signed short) == bytes)
            return "signed short";
        else if (sizeof(signed int) == bytes)
            return "signed int";
        else if (sizeof(signed long) == bytes)
            return "signed long";
        else if (sizeof(signed long long) == bytes)
            return "signed long long";
        else {
            exit(EXIT_FAILURE);
        }
    }
    else {
        if (sizeof(unsigned char) == bytes)
            return "unsigned char";
        else if (sizeof(unsigned short) == bytes)
            return "unsigned short";
        else if (sizeof(unsigned int) == bytes)
            return "unsigned int";
        else if (sizeof(unsigned long) == bytes)
            return "unsigned long";
        else if (sizeof(unsigned long long) == bytes)
            return "unsigned long long";
        else {
            exit(EXIT_FAILURE);            
        }
    }
    return NULL;
}

int main(int argc, char * argv[])
{
    printf("typedef %20s   int1;\\n",  getMatchingType(1, SIGNED));
    printf("typedef %20s   uint1;\\n", getMatchingType(1, UNSIGNED));
    printf("typedef %20s   int2;\\n",  getMatchingType(2, SIGNED));
    printf("typedef %20s   uint2;\\n", getMatchingType(2, UNSIGNED));
    printf("typedef %20s   int4;\\n",  getMatchingType(4, SIGNED));
    printf("typedef %20s   uint4;\\n", getMatchingType(4, UNSIGNED));
    printf("typedef %20s   int8;\\n",  getMatchingType(8, SIGNED));
    printf("typedef %20s   uint8;\\n", getMatchingType(8, UNSIGNED));
    printf("\\n");
    printf("typedef unsigned int           uint;\\n");
    return 0;
}
""", '.cpp')
if not ok:
    raise Exception('could not create dcmpi_typedefs.h, aborting!')
    sys.exit(1)
typedef_text = typedef_text + "\n" + integral_type_text

# check for endian
(ok, output) = context.TryRun("""
#include <stdio.h>
int main(void)
{
    unsigned int w = 1;
    if (((unsigned char*)&w)[0] == (unsigned char)0) {
        printf("big");
    }
    else {
        printf("little");
    }
    return 0;
}
""", '.cpp')
if output == "big":
    typedef_text = typedef_text + '#define DCMPI_BIG_ENDIAN\n'
else:
    typedef_text = typedef_text + '#define DCMPI_LITTLE_ENDIAN\n'

sizeof_ptr_body = """
#include <stdio.h>

int main()
{
    printf("%d\\n", (int)sizeof(void*));
    return 0;
}
"""
(ok, output) = context.TryRun(sizeof_ptr_body, '.cpp')
typedef_text = typedef_text + "#define DCMPI_SIZE_OF_PTR " + output + "\n"

# check for sysconf of phys memory
has_phys_mem_introspect = context.TryLink(
    """
#include <unistd.h>
int main()
{
    long r = sysconf(_SC_PHYS_PAGES) *
             sysconf(_SC_PAGE_SIZE);
    return 0;
}
""", '.cpp')
if not has_phys_mem_introspect:
    typedef_text = typedef_text + \
        '#define DCMPI_CANNOT_DISCOVER_PHYSICAL_MEMORY\n'

# check for sched_yield()
has_yield = context.TryLink("""
#include <sched.h>
int main() {
    sched_yield();
    return 0;
}
""", '.cpp')
if not has_yield:
    typedef_text = typedef_text + '#define DCMPI_NO_YIELD\n'


has_gethostbyname_r = context.TryLink("""
#include <netdb.h>

int main() {
    struct hostent ret;
    char buf[1024];
    size_t buflen = sizeof(buf);
    struct hostent * result;
    int err;
    gethostbyname_r("foo", &ret, buf, buflen,
                    &result, &err);
    return 0;
}
""", '.cpp')
if has_gethostbyname_r:
    typedef_text = typedef_text + '#define DCMPI_HAVE_GETHOSTBYNAME_R\n'
    

# for large files support
if large_file_support:
    # see http://www.suse.de/~aj/linux_lfs.html
    defines.append('_FILE_OFFSET_BITS=64')
    defines.append('_LARGEFILE_SOURCE')

# check for pcre
if pcre_path:
    pcre_body = """#include <pcre.h>
int main(int argc, char * argv[])
{
    pcre *re;
    const char *error;
    int erroffset;
    re = pcre_compile(
           "^A.*Z",          /* the pattern */
           0,                /* default options */
           &error,           /* for error message */
           &erroffset,       /* for error offset */
           NULL);            /* use default character tables */

    int rc;
    int ovector[30];
    rc = pcre_exec(
        re,             /* result of pcre_compile() */
        NULL,           /* we didn't study the pattern */
        "AxxxxxZ",      /* the subject string */
        11,             /* the length of the subject string */
        0,              /* start at offset 0 in the subject */
        0,              /* default options */
        ovector,        /* vector of integers for substring information */
        30);            /* number of elements in the vector */
    if (rc == 1) { return 0; } else { return 1; }
}
"""
    (ok, throwaway) = context.TryRun(pcre_body,'.cpp')
    if not ok:
        sys.stderr.write("ERROR: could not validate your pcre installation at %s\n" % (pcre_path))
        sys.exit(1)
    typedef_text = typedef_text + "#define DCMPI_HAVE_PCRE\n"

# check for rand_r
rand_r_body = """#include <stdlib.h>
int main(int argc, char * argv[])
{
    unsigned int foo = 1;
    rand_r(&foo);
}
"""
has_rand_r = context.TryLink(rand_r_body,'.cpp')
if has_rand_r:
    typedef_text = typedef_text + "#define DCMPI_HAVE_RAND_R\n"

# check for liblzop
if context.CheckLibWithHeader('lzo2','lzo/lzo1x.h', 'C++'):
    print 'building with lzop compression library'
    typedef_text = typedef_text + "#define DCMPI_HAVE_LZOP\n"

# check for libz
# if context.CheckLibWithHeader('z','zlib.h', 'C++'):
#     typedef_text = typedef_text + "#define DCMPI_HAVE_GZIP\n"

if context.CheckCXXHeader('execinfo.h'):
    typedef_text = typedef_text + "#define DCMPI_HAVE_BACKTRACE\n"

if sys.platform=='darwin':
    typedef_text = typedef_text + "#define DCMPI_HAVE_MACOSX\n"

# return modified env from context
env = context.Finish()

# note which compiler was use to build dcmpi
typedef_text = typedef_text + \
               "\n/* NOTE:  this version of dcmpi was built with the " + \
               "compiler " + env["CXX"]
if mpi_cxx_compiler:
    typedef_text = typedef_text + " and " + mpi_cxx_compiler
typedef_text = typedef_text + " */\n"

typedef_text = typedef_text + "#endif\n"
typedef_file = open("src/dcmpi_typedefs.h","w")
typedef_file.write(typedef_text)
typedef_file.close()

incpaths = [srcdir]
libdirs = [srcdir]
if build_appconfig:
    sqlite_dir = os.path.realpath('extensions/sqlite-3.3.5')
    env['sqlite_dir'] = sqlite_dir
    SConscript(os.path.join(sqlite_dir, 'SConscript'))

env.Append(CPPDEFINES = defines)
env.Append(CPPPATH = incpaths)
env.Append(LIBPATH = libdirs)
libs.extend(['dl','pthread'])
env.Append(LIBS = libs)

env['install_prefix'] = install_prefix

# avoid ORIGIN:  this does not work across multiple MPI compiler
# wrappers which have different lossiness of quoting metacharacters

# make it work in build tree
# env.Append(RPATH = [srcdir])
# make it work in install tree
# env.Append(RPATH = [os.path.join(install_prefix, 'lib')])

Export("mpi_cxx_compiler")
Export("srcdir")
Export("env")
Export("java_depstate")

# mpi env is based on env, but differs in terms of C++ compiler
if mpi_cxx_compiler:
    mpi_env = env.Copy(CXX = mpi_cxx_compiler)
    Export("mpi_env")

SConscript('src/SConscript')

if build_testing:
    SConscript('test/SConscript')

Help(opts.GenerateHelpText(env))
