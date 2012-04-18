#ifndef DCMPI_UTIL_H
#define DCMPI_UTIL_H

/***************************************************************************
    $Id: dcmpi_util.h,v 1.44 2006/09/29 13:27:18 rutt Exp $
    Copyright (C) 2004 by BMI Department, The Ohio State University.
    For license info, see LICENSE in root directory of source distribution.
 ***************************************************************************/

#include "dcmpi_clib.h"

#include "dcmpi.h"

#include "dcmpi_backtrace.h"

#include "DCMPIPacket.h"

#ifdef DCMPI_HAVE_PCRE
#include <pcre.h>
#endif

#ifdef DCMPI_HAVE_JAVA
#include <jni.h>
#endif

#ifdef DEBUG
#define checkrc(rc) if ((rc) != 0) { std::cerr << "ERROR: bad return code at " << __FILE__ << ":" << __LINE__ << " on host " << dcmpi_get_hostname() << std::endl << std::flush; assert(0); }
#else
#define checkrc(rc) if ((rc) != 0) { std::cerr << "ERROR: bad return code at " << __FILE__ << ":" << __LINE__ << " on host " << dcmpi_get_hostname() << std::endl << std::flush; exit(1); }
#endif

// string utils
#define tostr(s) dcmpi_to_string(s)

inline char *StrDup(const char *sb) {
    if (!sb) return NULL;
    char *sbTmp = new char[strlen(sb) + 1];
    strcpy(sbTmp, sb);
    return sbTmp;
}

inline int Atoi(const std::string & s) {
    return atoi(s.c_str());
}

inline double Atof(const std::string & s) {
    return strtod(s.c_str(), NULL);
//    return atof(s.c_str());
}

inline int8 Atoi8(std::string str)
{
    return strtoll(str.c_str(), NULL, 10);
}

// endian handling
inline void SwapByteArray_NC(void *p, int elemsz, int numelem)
{
    int i;
    for (i=0; i<numelem; ++i)
        dcmpi_endian_swap(((char *)p)+i*elemsz, elemsz);
}

inline void SwapByteArray(
    int nElem, int sElem, const void * pSrc, void * pDst = NULL)
{
    unsigned char *pBufSrc = (unsigned char *)pSrc;
    unsigned char *pBufDst = (unsigned char *)pDst;

    for (int i=0; i<nElem; i++) {
        for (int j=0; j<sElem; j++) {
            pBufDst[j] = pBufSrc[sElem - j - 1];
        }
        pBufSrc += sElem;
        pBufDst += sElem;
    }
}

inline std::string sql_escape(std::string s)
{
    dcmpi_string_replace(s, "'","''");
    return s;
}

// file routines
inline bool fileExists(const std::string & filename)
{
    /* stat returns 0 if the file exists */
    struct stat stat_out;
    return (stat(filename.c_str(), &stat_out) == 0);
}

inline bool fileIsDirectory(const std::string & filename)
{
    /* stat returns 0 if the file exists */
    struct stat stat_out;
    if (stat(filename.c_str(), &stat_out) != 0) {
        return false;
    }
    if (S_ISDIR(stat_out.st_mode)) {
        return true;
    }
    else {
        return false;
    }
}

inline off_t file_size(std::string filename)
{
    struct stat stat_out;
    if (stat(filename.c_str(), &stat_out) != 0) {
        std::cerr << "ERROR: file " << filename
                  << " does not exist"
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        exit(1);
    }
    off_t out = stat_out.st_size;
    return out;
}

inline void file_create(std::string filename)
{
    FILE * toucher = fopen(filename.c_str(), "w");
    if (!toucher) {
        std::cerr << "ERROR:  cannot touch file " << filename
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        exit(1);
    }
    fclose(toucher);
}

std::map<std::string, std::string> file_to_pairs(std::string filename);
std::map<std::string, std::string> file_to_pairs(FILE * f);
void pairs_to_file(std::map<std::string, std::string> pairs,
                   std::string filename);
inline std::string file_to_string(std::string filename)
{
    std::string out;
    off_t fs = file_size(filename);
    char * buf = new char[fs+1];
    buf[fs] = 0;
    FILE * f = fopen(filename.c_str(), "r");
    if (!f) {
        std::cerr << "ERROR: opening file " << filename
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        exit(1);
    }
    if (fread(buf, fs, 1, f) < 1) {
        std::cerr << "ERROR: in fread()"
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        exit(1);
    }
    out = buf;
    fclose(f);
    delete[] buf;
    return out;
}

std::string dcmpi_file_read_line(FILE * f); // returns non-stripped (has \n)

/* The following methods are like UNIX read() and write(), except you don't
 * worry that only a portion of the number of bytes were written or read.
 * They both return a nonzero error code (from errno.h) if an error occured in
 * reading or writing.  For DC_read_all(), if an EOF is detected, a nonzero
 * error code will returned and in addition the hitEOF out-parameter is will
 * be set to true (nonzero) if it is passed as non-NULL. */
int DC_read_all(int fd, void *buf, size_t count, int * hitEOF = NULL);
int DC_write_all(int fd, const void * buf, size_t count);
inline int DC_write_all_string(int fd, const std::string & s)
{
    return DC_write_all(fd, s.c_str(), s.size());
}

// misc
std::string shell_quote(const std::string & s);
std::string shell_unquote(const std::string & s);

inline bool resembles_localhost(const std::string & arg)
{
    static DCMutex m;
    static bool inited1 = false;
    static std::string localhost;
    static std::string localhost_shortname;

    m.acquire();
    if (!inited1) {
        localhost = dcmpi_get_hostname();
        localhost_shortname = dcmpi_get_hostname(true);
        inited1 = true;
    }
    m.release();

    if (arg == localhost) {
        return true;
    }
    uint i;
    static bool inited2 = false;
    static std::set<std::string> localhost_set;
    m.acquire();
    if (!inited2) {
        std::string localhost_name;
        std::vector<std::string> localhost_aliases;
        dcmpi_gethostbyname(localhost, localhost_name, localhost_aliases);
        std::string localhost_name_shortname = dcmpi_string_tokenize(localhost_name, ".")[0];
        localhost_set.insert(localhost);
        localhost_set.insert(localhost_shortname);
        localhost_set.insert(localhost_name);
        localhost_set.insert(localhost_name_shortname);
        for (i = 0; i < localhost_aliases.size(); i++) {
            localhost_set.insert(localhost_aliases[i]);
        }
        inited2 = true;
    }
    m.release();

    std::string arg_name;
    std::vector<std::string> arg_aliases;
    dcmpi_gethostbyname(arg, arg_name, arg_aliases);
    std::string arg_shortname = dcmpi_string_tokenize(arg, ".")[0];
    std::string arg_name_shortname = dcmpi_string_tokenize(arg_name, ".")[0];
    std::set<std::string> arg_set;
    arg_set.insert(arg);
    arg_set.insert(arg_shortname);
    arg_set.insert(arg_name);
    arg_set.insert(arg_name_shortname);
    for (i = 0; i < arg_aliases.size(); i++) {
        arg_set.insert(arg_aliases[i]);
    }

    std::set<std::string> tmp_set;
    std::set_intersection(localhost_set.begin(), localhost_set.end(),
                          arg_set.begin(), arg_set.end(),
                          std::inserter(tmp_set, tmp_set.begin()));
    return (tmp_set.empty() == false);
}

inline std::vector<std::string> dcmpi_get_ip_addrs(void)
{
    std::vector<std::string> out;
    struct ifaddrs *ifa = NULL;
    if (getifaddrs (&ifa) < 0)
    {
        perror ("getifaddrs");
        exit(1);;
    }

    for (; ifa; ifa = ifa->ifa_next)
    {
        char ip[ 200 ];
        socklen_t salen;

        if (ifa->ifa_addr->sa_family == AF_INET)
            salen = sizeof (struct sockaddr_in);
        else if (ifa->ifa_addr->sa_family == AF_INET6)
//            salen = sizeof (struct sockaddr_in6);
            continue;
        else
            continue;

        if (getnameinfo (ifa->ifa_addr, salen,
                         ip, sizeof (ip), NULL, 0, NI_NUMERICHOST) < 0)
        {
            perror ("getnameinfo");
            continue;
        }
        //printf ("%s\n", ip);
        out.push_back(ip);
    }
    freeifaddrs(ifa);
    out.erase(std::find(out.begin(),
                        out.end(),
                        "127.0.0.1"));
    return out;
}

std::string dcmpi_socket_read_line(int s); // returns stripped (no \n)
void dcmpi_socket_write_line(int s, std::string message); // message shouldn't contain \n

inline bool dcmpi_socket_data_ready_for_reading(int s)
{
//     pollfd pfd;
//     pfd.fd = s;
//     pfd.events = POLLIN;
//     pfd.revents = 0;
//     if (poll(&pfd, 1, 0)) {
//         return true;
//     }
//     else {
//         return false;
//     }
    while (1) {
        fd_set ins;
        int nfds = s + 1;
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 0;
        FD_ZERO(&ins);
        FD_SET(s, &ins);
        int rc = select(nfds, &ins, NULL, NULL, &timeout);
        if (rc == -1 && errno == EINTR) {
            dcmpi_doublesleep(0.01);
        }
        else if (rc == -1) {
            std::cerr << "ERROR: on host " << dcmpi_get_hostname()
                      << " calling select, errno=" << errno
                      << " at " << __FILE__ << ":" << __LINE__
                      << std::endl << std::flush;
            exit(1);            
        }
        else {
            if (FD_ISSET(s, &ins)) {
                return true;
            }
            else {
                return false;
            }
        }
    }
}

inline void dcmpi_socket_read_dcbuffer(int s, DCMPIPacket * p)
{
    DCBuffer * buf;
    int sz;
    sz = p->body_len;
    buf = new DCBuffer(sz);
    char * offset = buf->getPtr();
    if (DC_read_all(s, offset, p->body_len) != 0) {
        std::cerr << "ERROR: DC_read_all"
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        exit(1);
    }
    buf->setUsedSize(sz);
    p->body = buf;
}

inline void dcmpi_socket_write_dcbuffer(int s, DCMPIPacket * p)
{
    int rc;
    if ((rc = DC_write_all(s, p->body->getPtr(), p->body_len)) != 0) {
        std::cerr << "ERROR: writing on socket "
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        exit(1);
    }
}

void dcmpi_bash_select_emulator(
    int                              indentSpaces,
    const std::vector<std::string> & inChoices,
    const std::string &              description,
    const std::string &              exitDescription,
    bool                             allowMultipleChoices,
    bool                             allowDuplicates,
    std::vector<std::string> &       outChoices,
    std::vector<int> &               outIndexes);

inline std::vector<std::string> get_config_noncomment_nonempty_lines(
    std::string filename)
{
    std::vector<std::string> out;
    FILE * f;
    if ((f = fopen(filename.c_str(), "r")) == NULL) {
        std::cerr << "ERROR: opening file"
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        exit(1);
    }
    char buf[1024];
    while (fgets(buf, sizeof(buf), f)) {
        std::string s = buf;
        dcmpi_string_trim(s);
        if (s[0] != '#' && !s.empty()) {
            out.push_back(s);
        }
    }
    if (fclose(f) != 0) {
        std::cerr << "ERROR: calling fclose()"
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        exit(1);
    }
    return out;
}

inline std::string get_systemwide_config_root()
{
    std::string mpi_prog;
    if (!dcmpi_is_local_console) {
        mpi_prog = dcmpi_remote_argv_0;
    }
    else {
        mpi_prog = dcmpi_find_executable("dcmpiconfig", true);
    }
    std::string root = dcmpi_file_dirname(mpi_prog);
    std::string containing_dir = dcmpi_file_basename(root);
    if (containing_dir == "bin") {
        root = dcmpi_file_dirname(root);
        root = root + "/etc/dcmpi";
        return root;
    }
    else {
        return "";
    }
}

// return systemwide, user config file
inline std::vector<std::string> get_config_filenames(std::string config_name, bool ok_if_not_exist=false)
{
    std::vector<std::string> out;
    std::string mpi_prog;
    if (!dcmpi_is_local_console) {
        mpi_prog = dcmpi_remote_argv_0;
    }
    else {
        mpi_prog = dcmpi_find_executable("dcmpiconfig", true);
    }
    std::string system_root = get_systemwide_config_root();
    if (dcmpi_file_exists(system_root)) {
        std::string fn = (system_root + "/" + config_name);
        if (ok_if_not_exist || dcmpi_file_exists(fn)) {
            out.push_back(fn);
            if (dcmpi_verbose()) {
                std::cout << "get_config_filenames(): systemwide filename is "
                          << fn << std::endl;
            }
        }
    }
    if (out.size() == 0) {
        out.push_back("");
    }
    std::string user_fn = dcmpi_get_home() + "/" + config_name;
    
    if (ok_if_not_exist || dcmpi_file_exists(user_fn)) {
        if (dcmpi_verbose()) {
            std::cout << "get_config_filenames(): user filename is ";
            std::cout << user_fn << std::endl;
        }
        out.push_back(user_fn);
    }
    else {
        out.push_back("");
    }
    return out;
}

inline std::vector<std::string> get_dcmpi_filter_path(void)
{
    std::vector<std::string> out;

    if (getenv("DCMPI_FILTER_PATH")) {
        std::string fpath = getenv("DCMPI_FILTER_PATH");
        std::vector<std::string> paths = dcmpi_string_tokenize(fpath, ":");
        for (uint u = 0; u < paths.size(); u++) {
            std::string path = paths[u];
            if (dcmpi_file_exists(path)) {
                out.push_back(path);
            }
        }
    }

    std::vector<std::string> config_files =
        get_config_filenames("dcmpi_filter_path");

    for (int i = 1; i >= 0; i--) {
        if (config_files[i] != "") {
            std::vector<std::string> lines =
                get_config_noncomment_nonempty_lines(config_files[i]);
            for (uint u2 = 0; u2 < lines.size(); u2++) {
                std::string line = lines[u2];
                std::vector<std::string> toks = dcmpi_string_tokenize(line, ":");
                for (uint u3 = 0; u3 < toks.size(); u3++) {
                    std::string term = toks[u3];
                    if (dcmpi_file_exists(term)) {
                        out.push_back(term);
                    }
                }
            }
        }
    }

    if (dcmpi_verbose()) {
        for (uint u = 0; u < out.size(); u++) {
            std::cout << "dcmpi_filter_path entry on "
                      << dcmpi_get_hostname()
                      << ": " << out[u] << std::endl;
        }
    }
    
    return out;
}

#define DCMPI_DEFAULT_RUN_COMMAND "dcmpirun-socket -hostfile %h -size %n -exe %e"
#define DCMPI_DEFAULT_SOCKET_RSH_COMMAND "ssh -x -o StrictHostKeyChecking=no"
inline std::string get_raw_run_command()
{
    FILE * f;
    std::vector<std::string> input_files = get_config_filenames("runcommand");
    std::string input_file;
    if (input_files[0] != "") {
        input_file = input_files[0];
    }
    if (input_files[1] != "") {
        input_file = input_files[1];
    }
    if (input_file.empty()) {
        return DCMPI_DEFAULT_RUN_COMMAND;
    }
    if ((f = fopen(input_file.c_str(), "r"))==NULL) {
        std::cerr << "ERROR: " << input_file
                  << " is unreadable, aborting"
                  << std::endl << std::flush;
        exit(1);
    }
    if (dcmpi_verbose()) {
        std::cout << "using runcommand from " << input_file << std::endl;
    }
    
    char buf[512];
    while (1) {
        if (fgets(buf, sizeof(buf), f) == NULL) {
            std::cerr << "ERROR: no run command found in " << input_file
                      << std::endl << std::flush;
            exit(1);
        }
        else if (buf[0] != '#') {
            break;
        }
    }
    fclose(f);
    std::string out = buf;
    dcmpi_string_trim(out);
    return out;
}

inline std::string get_dcmpiruntime_runtime(const std::string & mpi_run_command)
{
    std::string runtime = "dcmpiruntime";
    if (dcmpi_string_starts_with(mpi_run_command, "dcmpirun-socket")) {
        runtime = "dcmpiruntime-socket";
    }
    return runtime;
}

inline std::string get_dcmpiruntime_suffix(const std::string & mpi_run_command)
{
    std::string suffix;
    if (dcmpi_string_starts_with(mpi_run_command, "dcmpirun-socket")) {
        std::vector<std::string> socket_config =
            get_config_filenames("socket");
        std::string rsh = DCMPI_DEFAULT_SOCKET_RSH_COMMAND;
        std::string wrapper;
        std::string wrapperhosts;
        if (socket_config.empty()) {
            ;
        }
        else {
            for (uint i = 0; i < socket_config.size(); i++) {
                if (socket_config[i].empty()==false) {
                    std::map<std::string, std::string> pairs =
                        file_to_pairs(socket_config[i]);
                    if (pairs.count("rsh")) {
                        rsh = pairs["rsh"];
                    }
                    if (pairs.count("wrapper")) {
                        wrapper = pairs["wrapper"];
                    }
                    if (pairs.count("wrapperhosts")) {
                        wrapperhosts = pairs["wrapperhosts"];
                    }
                }
            }
        }
        suffix = "-rsh " + shell_quote(rsh);
        if (wrapper.size()) {
            suffix += " -wrapper " + shell_quote(wrapper);
        }
        if (wrapperhosts.size()) {
            suffix += " -wrapperhosts " + shell_quote(wrapperhosts);
        }
    }
    return suffix;
}

inline void run_command_finalize(
    const std::string & executable,
    std::string exe_suffix,
    const std::vector<std::string> & hosts,
    std::map<std::string, std::string> & launch_params,
    std::string & run_command,
    std::vector<std::string> & mpi_run_temp_files)
{
    std::string executable_and_args = dcmpi_find_executable(executable);
    if (exe_suffix.empty()==false) {
        executable_and_args += " " + exe_suffix;
    }
    executable_and_args += " dcmpi_control_args_begin";
    std::map<std::string, std::string>::iterator it;
    for (it = launch_params.begin();
         it != launch_params.end();
         it++) {
        executable_and_args += " " + it->first + " " + it->second;
    }
    executable_and_args += " dcmpi_control_args_end";
    bool replaced_executable = false;
    while (1) {
        if (dcmpi_string_replace(run_command,
                              "%n", dcmpi_to_string(hosts.size()))) {
            ;
        }
        else if (dcmpi_string_replace(
                     run_command, "%e", executable_and_args)) {
            replaced_executable = true;
        }
        else if (run_command.find("%h") != std::string::npos) {
            std::string machinefile = dcmpi_get_temp_filename();
            mpi_run_temp_files.push_back(machinefile);
            FILE * f = fopen(machinefile.c_str(), "w");
            uint u;
            for (u = 0; u < hosts.size(); u++) {
                const char * s = hosts[u].c_str();
                if (fprintf(f, "%s\n", s) != (int)(strlen(s) + 1)) {
                    std::cerr << "ERROR: cannot write out machinefile"
                              << " at " << __FILE__ << ":" << __LINE__
                              << std::endl << std::flush;
                    exit(1);
                }
            }
            fclose(f);
            dcmpi_string_replace(run_command, "%h", machinefile);
        }
        else if (dcmpi_string_replace(run_command, "%F", hosts[0])) {
            ;
        }
        else if (run_command.find("%D") != std::string::npos) {
            if (getenv("DCMPI_DEBUG")) {
                dcmpi_string_replace(run_command, "%D", getenv("DCMPI_DEBUG"));
            }
            else {
                dcmpi_string_replace(run_command, "%D", "");
            }
        }
        else {
            break;
        }
    }
    if (!replaced_executable) { // put at end
        run_command += " ";
        run_command += executable_and_args;
    }
}

#ifdef DCMPI_HAVE_PCRE
inline bool dcmpi_pcre_matches(std::string regexp, std::string subject)
{
    pcre * re;
    const char *error;
    int erroffset;
    re = pcre_compile(
        regexp.c_str(),   /* the pattern */
        0,                /* default options */
        &error,           /* for error message */
        &erroffset,       /* for error offset */
        NULL);            /* use default character tables */
    if (!re) {
        std::cerr << "ERROR: compiling regexp " << regexp
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        std::cerr << "ERROR: message from pcre was "
                  << error;
        exit(1);
    }
    int rc;
    int ovector[30];
    rc = pcre_exec(
        re,             /* result of pcre_compile() */
        NULL,           /* we didn't study the pattern */
        subject.c_str(),/* the subject string */
        subject.size(), /* the length of the subject string */
        0,              /* start at offset 0 in the subject */
        0,              /* default options */
        ovector,        /* vector of integers for substring information */
        30);            /* number of elements in the vector */
    pcre_free(re);
    if (rc > 0) {
        return true;
    }
    else {
        return false;
    }
}
#endif

#ifdef DCMPI_HAVE_JAVA
#if DCMPI_SIZE_OF_PTR == 4
#define DCMPI_JNI_CAST_FROM_POINTER_TO_JLONG(thing) (jlong)((int8)((uint4)(thing)))
#define DCMPI_JNI_CAST_FROM_JLONG_TO_POINTER(thing, type) (type)((uint4)(thing))
#elif DCMPI_SIZE_OF_PTR == 8
#define DCMPI_JNI_CAST_FROM_POINTER_TO_JLONG(thing) (jlong)(thing)
#define DCMPI_JNI_CAST_FROM_JLONG_TO_POINTER(thing, type) (type)(thing)
#else
#error "Unknown pointer size, this code won't work"
#endif

#define DCMPI_JNI_EXCEPTION_CHECK(env) if (env->ExceptionOccurred()) { env->ExceptionDescribe(); std::cerr << "ERROR: dcmpi bailing out after java exception at " << __FILE__ << ":" << __LINE__ << " on host " << dcmpi_get_hostname() << std::endl << std::flush; exit(1); }

class DCJavaFilter : public DCFilter
{
    std::string class_name;
    std::string lib_name;

    // will be global references
    JNIEnv * separate_thread_env;
    jclass cls;
    jobject obj;
public:
    DCJavaFilter(
        const std::string & classname,
        const std::string & libname) : class_name(classname), lib_name(libname)
    {}
    virtual int init();
    virtual int process();
    virtual int fini();
};
#endif

#ifdef DCMPI_HAVE_PYTHON
#define PYSHOWTRACEBACK()                       \
        if (PyErr_Occurred()) {                 \
            PyErr_Print();                      \
            exit(1);                            \
        }

class DCPyFilter : public DCFilter
{
public:
    std::string class_name;
    std::string lib_name;

    PyThreadState * myThreadState;
    PyObject * pyclass;
public:
    DCPyFilter(
        const std::string & classname,
        const std::string & libname) :
        class_name(classname), lib_name(libname),
        myThreadState(NULL), pyclass(NULL)
    {}
    virtual int init();
    virtual int process();
    virtual int fini();
};
#endif

#endif /* #ifndef DCMPI_UTIL_H */
