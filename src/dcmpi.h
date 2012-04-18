#ifndef DCMPI_H
#define DCMPI_H

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <map>
#include <set>

#include <pthread.h>
#include <libgen.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <netdb.h>

#include "dcmpi_typedefs.h"

#define KB_1          1024
#define KB_2          2048
#define KB_4          4096
#define KB_8          8192
#define KB_16        16384
#define KB_32        32768
#define KB_64        65536
#define KB_128      131072
#define KB_256      262144
#define KB_512      524288
#define MB_1       1048576
#define MB_2       2097152
#define MB_4       4194304
#define MB_8       8388608
#define MB_16     16777216 
#define MB_32     33554432
#define MB_64     67108864
#define MB_128   134217728
#define MB_256   268435456
#define MB_512   536870912
#define GB_1    1073741824

bool dcmpi_verbose();

/**
 *  Provides a max size buffer, which can be appended to, and keeps track of
 *  the used size.  At deletion time, the buffer is freed only if set to be.
 */
class DCBuffer
{
    friend std::ostream &operator<<(std::ostream &os, DCBuffer &buf);
public:
    DCBuffer(void);
    DCBuffer(int wMax_in);
    DCBuffer(const DCBuffer & t)
    {
        pBuf = NULL;
        char * p = new char[t.getUsedSize()];
        memcpy(p, t.pBuf, t.getUsedSize());
        this->Set(p, t.getUsedSize(), t.getUsedSize(), true);
        this->forceEndian(t.isBigEndian());
    }

    DCBuffer & operator=(const DCBuffer & t)
    {
        if (&t != this) {
            pBuf = NULL;
            char * p = new char[t.getUsedSize()];
            memcpy(p, t.pBuf, t.getUsedSize());
            this->Set(p, t.getUsedSize(), t.getUsedSize(), true);
            this->forceEndian(t.isBigEndian());
        }
        return *this;
    }
    virtual ~DCBuffer(void);

    char *New(int wMax_in);
    void Delete(void); // delete inner-contained memory
    void Set(const char *pBuf_in, int wMax_in, int wSize_in, bool handoff);
    void Empty(void);

    void consume(void) {
        delete this; // suicide for a heap-allocated buffer
    }

    char *getPtr(void)                        { return pBuf; }               // to beginning
    const char *getPtr(void) const            { return pBuf; }               // to beginning
    char *getPtrFree(void)                    { return pBuf+wSize; }         // to free space
    char *getPtrExtract(void)                 { return pBuf+wSizeExtracted; }    

    int getUsedSize(void) const       { return wSize; }              // amount used at beg
    int setUsedSize(int wSize_in);
    void incrementUsedSize(int size) { this->setUsedSize(this->getUsedSize() + size); }

    int getFree(void) const           { return wMax-wSize; }         // free space at end
    int getMax(void) const            { return wMax; }               // total size of buf

    int getExtractAvailSize(void) const { return wSize-wSizeExtracted; } // amt left
    int getExtractedSize(void) const { return wSizeExtracted; } // amt already extracted
    int resetExtract(int w=0)      { assert(w >= 0); return wSizeExtracted=w; }
    // don't extract, but increment the extract pointer
    void incrementExtractPointer(int size) {
        assert(size >= 0);
        wSizeExtracted += size;
        assert(wSizeExtracted <= wSize);
    }

    void forceEndian(bool is_big_endian)       { _isbigendian = is_big_endian; }
    bool isBigEndian() const { return _isbigendian; }

    // format string syntax:
    // b:int1; B:uint1
    // h:int2; H:uint2;
    // i:int4; I:uint4;
    // l:int8; L:uint8;
    // f:float; d:double.
    void pack(const char * format, ...);
    void unpack(const char * format, ...);

    /// if doConversion is true it assumes wSizeAdd a single item of size wSizeAdd
    char *Append(const char *pAdd, int wSizeAdd, bool doConversion=false);
    char *Append(int1 c)    { return Append((char*)&c, 1, false); }
    char *Append(uint1 c)   { return Append((char*)&c, 1, false); }
    char *Append(int2 w)    { return Append((char*)&w, 2, doConv()); }
    char *Append(uint2 w)   { return Append((char*)&w, 2, doConv()); }
    char *Append(int4 w)    { return Append((char*)&w, 4, doConv()); }
    char *Append(uint4 w)   { return Append((char*)&w, 4, doConv()); }
    char *Append(int8 w)    { return Append((char*)&w, 8, doConv()); }
    char *Append(uint8 w)   { return Append((char*)&w, 8, doConv()); }
    char *Append(float r)   { return Append((char*)&r, 4, doConv()); }
    char *Append(double r)  { return Append((char*)&r, 8, doConv()); }
    char * Append(const std::string & str) { Append((int8)str.size()); return Append(str.data(), str.size()); }

    /* the following Append methods are for adding arrays */
    char *Append(int1 * c,  int nelem)  { return _AppendArray((char*)c, 1, false,    nelem); }
    char *Append(uint1 * c, int nelem)  { return _AppendArray((char*)c, 1, false,    nelem); }
    char *Append(int2 * w,  int nelem)  { return _AppendArray((char*)w, 2, doConv(), nelem); }
    char *Append(uint2 * w, int nelem)  { return _AppendArray((char*)w, 2, doConv(), nelem); }
    char *Append(int4 * w,  int nelem)  { return _AppendArray((char*)w, 4, doConv(), nelem); }
    char *Append(uint4 * w, int nelem)  { return _AppendArray((char*)w, 4, doConv(), nelem); }
    char *Append(int8 * w,  int nelem)  { return _AppendArray((char*)w, 8, doConv(), nelem); }
    char *Append(uint8 * w, int nelem)  { return _AppendArray((char*)w, 8, doConv(), nelem); }
    char *Append(float * r, int nelem)  { return _AppendArray((char*)r, 4, doConv(), nelem); }
    char *Append(double * r,int nelem)  { return _AppendArray((char*)r, 8, doConv(), nelem); }
    
    /// if doConversion is true it assumes wSizeAdd a single item of size wSizeAdd
    int Extract(char *pBuf_in, int wSize_in, bool doConversion=false);
    int Extract(int1 *pch)     { return Extract((char*)pch, 1, false);    }
    int Extract(uint1 *pch)    { return Extract((char*)pch, 1, false);    }
    int Extract(int2 *pw)      { return Extract((char*)pw,  2, doConv()); }
    int Extract(uint2 *pw)     { return Extract((char*)pw,  2, doConv()); }
    int Extract(int4 *pw)      { return Extract((char*)pw,  4, doConv()); }
    int Extract(uint4 *pw)     { return Extract((char*)pw,  4, doConv()); }
    int Extract(int8 *pw)      { return Extract((char*)pw,  8, doConv()); }
    int Extract(uint8 *pw)     { return Extract((char*)pw,  8, doConv()); }
    int Extract(float *pr)     { return Extract((char*)pr,  4, doConv()); }
    int Extract(double *pr)    { return Extract((char*)pr,  8, doConv()); }
    int Extract(std::string * str);

    /* the following Extract methods are for extracting arrays */
    int Extract(int1 *pch,  int nelem)     { return _ExtractArray((char*)pch, 1, false, nelem);    }
    int Extract(uint1 *pch, int nelem)     { return _ExtractArray((char*)pch, 1, false, nelem);    }
    int Extract(int2 *pw,   int nelem)     { return _ExtractArray((char*)pw,  2, doConv(), nelem); }
    int Extract(uint2 *pw,  int nelem)     { return _ExtractArray((char*)pw,  2, doConv(), nelem); }
    int Extract(int4 *pw,   int nelem)     { return _ExtractArray((char*)pw,  4, doConv(), nelem); }
    int Extract(uint4 *pw,  int nelem)     { return _ExtractArray((char*)pw,  4, doConv(), nelem); }
    int Extract(int8 *pw,   int nelem)     { return _ExtractArray((char*)pw,  8, doConv(), nelem); }
    int Extract(uint8 *pw,  int nelem)     { return _ExtractArray((char*)pw,  8, doConv(), nelem); }
    int Extract(float *pr,  int nelem)     { return _ExtractArray((char*)pr,  4, doConv(), nelem); }
    int Extract(double *pr, int nelem)     { return _ExtractArray((char*)pr,  8, doConv(), nelem); }

    // more SWIG-able interface follows
    void pack_i1(int1 b)         {Append(b);}
    void pack_i2(int2 s)         {Append(s);}
    void pack_i4(int4 i)         {Append(i);}
    void pack_i8(int8 l)         {Append(l);}
    void pack_ui1(uint1 b)       {Append(b);}
    void pack_ui2(uint2 s)       {Append(s);}
    void pack_ui4(uint4 i)       {Append(i);}
    void pack_ui8(uint8 l)       {Append(l);}
    void pack_f(float f)         {Append(f);}
    void pack_d(double d)        {Append(d);}
    void pack_s(std::string s)   {Append(s);}
    int1         unpack_i1()  {int1 b; Extract(&b); return b;}
    int2         unpack_i2()  {int2 s; Extract(&s); return s;}
    int4         unpack_i4()  {int4 i; Extract(&i); return i;}
    int8         unpack_i8()  {int8 l; Extract(&l); return l;}
    uint1        unpack_ui1() {uint1 b; Extract(&b); return b;}
    uint2        unpack_ui2() {uint2 s; Extract(&s); return s;}
    uint4        unpack_ui4() {uint4 i; Extract(&i); return i;}
    uint8        unpack_ui8() {uint8 l; Extract(&l); return l;}
    float        unpack_f()   {float f; Extract(&f); return f;} 
    double       unpack_d()   {double d; Extract(&d); return d;} 
    std::string  unpack_s()   {std::string s; Extract(&s); return s;} 
    
    int sprintf(const char *sbFormat, ...);

    void saveToDCBuffer(DCBuffer * buf) const;
    void restoreFromDCBuffer(DCBuffer * buf);

    int saveToDisk(const char * filename) const;      /* sends a DCBuffer to disk */
    int restoreFromDisk(const char * filename); /* fills a DCBuffer from disk */

    int saveToFd(int fd) const;      /* sends a DCBuffer to a file descriptor */
    int restoreFromFd(int fd); /* fills a DCBuffer from a descriptor */

    void compress();
    void decompress();

    void extended_print(std::ostream &os);

protected:
    char *pBuf;               // ptr to start of buffer
    bool owns_pBuf;           // whether to assume ownership of pBuf
    int wMax;                 // max size of buffer
    int wSize;                // size of the "used" prefix in buffer
    int wSizeExtracted;
    bool _isbigendian;
    int _appendReallocs;
    
private:
    void commonInit();
    bool doConv();
    char * _AppendArray(const char *pAdd, int wSizeAdd,
                        bool doConversion, int nelem)
    {
        char * rv = NULL;
        int i = 0;
        for (i = 0; i < nelem; i++) {
            if (!(rv = Append(pAdd, wSizeAdd, doConversion))) {
                return NULL;
            }
            pAdd += wSizeAdd;
        }
        return rv;
    }
    int _ExtractArray(char *pBuf_in, int wSize_in,
                      bool doConversion, int nelem)
    {
        int bytesExtracted = 0;
        int i = 0;
        for (i = 0; i < nelem; i++) {
            bytesExtracted += Extract(pBuf_in, wSize_in, doConversion);
            pBuf_in += wSize_in;
        }
        return bytesExtracted;
    }
};

class DCSerializable
{
public:
    virtual ~DCSerializable() {}
    virtual void serialize(DCBuffer * buf) const = 0;
    virtual void deSerialize(DCBuffer * buf)=0;
};

class DCFilterRegistry;
class DCFilterInstance;
class DCFilterExecutor;
class MultiPort;
class ResolvedMultiPort;
class Gftid_Port;
class DCFilter;
class DCMPIPacket;
template <class T> class Queue;
class ConsoleToMPIThread;
class ConsoleFromMPIThread;
class DCCommandThread;

class DCLayout : public DCSerializable
{
public:
    std::vector<DCFilterInstance*> filter_instances;

    DCFilterRegistry * filter_reg; // knowledge of all filters

    // stores temporarily allocated expansion filters; reclaimed at end of
    // execute
    std::vector<DCFilterInstance*> expansion_filter_instances;

    // consume these in destructor
    std::vector<DCFilterInstance*> deserialized_filter_instances;

    // contains: <console> <local hostname> [other_hosts...].  valid during
    // execution time only.
    std::vector<std::string> run_hosts;

    bool has_console_filter;
    bool uses_unix_socket_console_bridge;

    DCBuffer * init_filter_broadcast;

    // these only valid on local console side, not serialized
    std::vector<std::string> run_hosts_no_console;
    DCFilterExecutor * console_exe;
    DCFilterInstance * console_instance;
    std::map<int, DCFilterInstance*> gftids_to_instances;
    std::set<int> ranks_console_sends_to;
    std::set<int> ranks_console_hears_from;

    Queue<DCMPIPacket*> *             console_to_mpi_queue;
    ConsoleToMPIThread *              console_to_mpi_thread;
    ConsoleFromMPIThread *            console_from_mpi_thread;
    std::vector<DCCommandThread *>    mpi_command_threads;
    std::string                       consoleToMPITempFn;
    std::string                       consoleFromMPITempFn;
    std::vector<std::string>          mpi_run_temp_files;
    std::string                       unix_socket_tempd;
    DCFilter *                        console_filter;

    std::map<std::string, int>        host_dcmpirank;

    int get_filter_count_for_dcmpi_rank(int rank);
    DCLayout()
    {
        common_init();
    }
    DCLayout(std::vector<DCFilterInstance*> filter_instances)
    {
        for (uint u = 0; u < filter_instances.size(); u++) {
            this->add(filter_instances[u]);
        }
        common_init();
    }
    ~DCLayout();
    void add(DCFilterInstance * filter_instance);
    void add(DCFilterInstance & filter_instance);

    void add_port(DCFilterInstance * filter1, std::string port1,
                  DCFilterInstance * filter2, std::string port2);
    void add_port(DCFilterInstance & filter1, std::string port1,
                  DCFilterInstance & filter2, std::string port2);
    void show_graphviz();

    void set_param_all(std::string key, std::string value);
    void set_param_all(std::string key, int value);

    // NOTE:  copies argument buf
    void set_init_filter_broadcast(DCBuffer * buf)
    {
        init_filter_broadcast = new DCBuffer(*buf);
    }

    
    // set which hosts to draw from when a filter has not explicitly been
    // bound to a host
    void set_exec_host_pool_file(const char * hostfile); // read hosts from
                                                         // file, 1 host per
                                                         // line
    void set_exec_host_pool(std::string host);
    void set_exec_host_pool(const std::vector<std::string> & hosts);

    // BLOCKING EXECUTION
    int execute();

    // NONBLOCKING EXECUTION (returns console filter if there is one)
    DCFilter * execute_start();
    int execute_finish();

    void add_propagated_environment_variable(
        const char * varname,
        bool override_if_present = true);

    // e.g. pass "libfoofilters.so" which will result in filters in
    // "libfoofilters.so" being available
    void use_filter_library(const std::string & library_name);
    
    friend std::ostream& operator<<(std::ostream &o, const DCLayout & i);

    void print_global_thread_ids_table();

    void serialize(DCBuffer * buf) const;
    void deSerialize(DCBuffer * buf);

private:
    void common_init();
    void expand_transparents();
    void expand_composites();
    void do_placement();
    std::string to_graphviz();
    void execute1();
    void execute2();
    int execute3();
    void clear_expansion_filter_instances();    // clean up local stuff
    void clear_deserialized_filter_instances(); // clean up remote stuff
    void mpi_build_run_commands(
        const std::vector<std::string> & hosts,
        std::map<std::string, std::string> launch_params,
        const std::string & cookie,
        std::vector<std::string> & mpi_run_commands,
        std::vector<std::string> & mpi_run_temp_files);

    std::vector<std::string> exec_host_pool;
    std::map<std::string,std::string> set_param_all_params;

    // technically not serialized, this variable maps propagated environment
    // variables to whether or not they should override existing values
    // present on the remote host
    std::map<std::string,bool> propagated_environment_variables;

    bool do_graphviz;
    std::string graphviz_output;
    std::string graphviz_output_posthook;
    bool execute_start_called;
    std::string localhost;
    std::string localhost_shortname;
};

class DCFilterInstance : public DCSerializable
{
public:
    std::string heritage;
    std::string filtername;
    std::string instancename;
    std::map<std::string, MultiPort> outports; // not serialized
    int copy_rank;
    int num_copies;
    int local_copy_rank;
    int local_num_copies;

    bool transparent_copies_expanded;   // not serialized
    std::vector<std::string> bind_hosts;// not serialized
    std::vector<std::vector<std::string> > layout_labels; // not serialized

    bool is_inbound_composite_designate;
    bool is_outbound_composite_designate;
    std::vector<std::string> composite_cluster_binding;
    int gftid;
    std::map<std::string, ResolvedMultiPort> resolved_outports;
    std::map<std::string, std::set<Gftid_Port> > resolved_inports;
    std::map<std::string, std::string> user_params;
    std::map<std::string, DCBuffer> user_buffer_params;
    int dcmpi_rank;
    std::string given_bind_host; // most likely same as dcmpi_host
    std::string dcmpi_host;
    int dcmpi_cluster_rank;
    
    // the one or more runtime labels that this filter answers to
    std::vector<std::string> runtime_labels;

public:
    DCFilterInstance(std::string filtername,
                     std::string instancename);
    ~DCFilterInstance();

    std::string get_filter_name();
    std::string get_instance_name();
    std::string get_distinguished_name();

    // explicit filter directives
    void bind_to_host(const std::string & host);
    void add_label(const std::string & label);
    
    // transparent copy directives
    void make_transparent(int num_copies);
    void bind_to_hosts(const std::vector<std::string> & hosts);
    void add_labels_to_transparents(const std::vector<std::string> & labels);

    // composite directives
    void bind_to_cluster(std::vector<std::string> hosts);
    void designate_as_inbound_for_composite();
    void designate_as_outbound_for_composite();
    
    void set_param(std::string key, std::string val)
    {
        user_params[key] = val;
    }
    void set_param(std::string key, int val);
    
    void set_param_buffer(std::string key, const DCBuffer & val)
    {
        user_buffer_params[key] = DCBuffer(val);
    }
    bool has_param(std::string key)
    {
        return user_params.count(key) != 0;
    }
    bool has_param_buffer(std::string key)
    {
        return user_buffer_params.count(key) != 0;
    }
    std::string get_param(const std::string & key)
    {
        if (user_params.count(key) == 0) {
            std::cerr << "ERROR: invalid get_param call on key '" << key
                      << "' in filter " << get_distinguished_name()
                      << std::endl << std::flush;
            exit(1);
        }
        return user_params[key];
    }
    int get_param_as_int(const std::string & key)
    {
        std::string s = get_param(key);
        return atoi(s.c_str());
    }
    double get_param_as_double(const std::string & key)
    {
        std::string s = get_param(key);
        return atof(s.c_str());
    }
private:
    void serialize(DCBuffer * buf) const;
    void deSerialize(DCBuffer * buf);
    void add_port(std::string local_port_name,
                  DCFilterInstance * remote_filter_instance,
                  std::string remote_port_name);

    void to_graphviz(
        DCLayout *                 layout_obj,
        bool                       print_host_if_applicable,
        std::string &              instance_text,
        std::vector<std::string> & connections);

    DCFilterInstance();    friend class DCLayout;
};

class DCFilterStats
{
public:
    DCFilterStats() {}
    double timestamp_process_start;
    double timestamp_process_stop;
    std::map<std::string, double> read_block_time;
    std::map<std::string, double> write_block_time;
    std::map<std::string, std::string> user_dict;
};

class DCFilter
{
public:
    DCFilter();
    virtual ~DCFilter()
    {
      //commented out by k2 because it crashes jvm
      //delete init_filter_broadcast;
    }
    
    DCBuffer * get_init_filter_broadcast();

    // inquire about my name
    std::string get_filter_name();
    std::string get_instance_name();
    std::string get_distinguished_name();
    int get_global_filter_thread_id(); // will range from 0..#filters-1,
                                       // unique for every filter (console
                                       // always has 0 though, if it's
                                       // involved)

    std::string get_bind_host(); // which host am I running on?

    // inquire about input, output ports
    std::vector<std::string> get_in_port_names();
    std::vector<std::string> get_out_port_names();
    bool has_in_port(const std::string & port);
    bool has_out_port(const std::string & port);
    
    // transparent copy information, globally
    int get_copy_rank();
    int get_num_copies();

    // transparent copy information, per node
    int get_local_copy_rank();
    int get_local_num_copies();

    // filter authors should either define the method
    // process (and optionally init/finalize), or define
    // the method get_composite_layout()
    virtual int init() { return 0; }
    virtual int process();
    virtual int fini() { return 0; }
    
    virtual DCLayout * get_composite_layout(std::vector<std::string> cluster_hosts);

    // blocks until returns, always returns non-NULL
    DCBuffer * read(const std::string & port);
    
    // returns NULL or non-NULL, depending on if anything is available or not
    // at the time it is called
    DCBuffer * read_nonblocking(const std::string & port);

    // are any writers to me still running?
    bool any_upstream_running();

    // will read from port, or return NULL if all upstream writers to me (on
    // any port) have exited
    DCBuffer * read_until_upstream_exit(const std::string & port);
    
    // will read from any port, until all upstream filters (writers to this
    // filter) have exited, in which case it will return NULL.  If the
    // optional parameter 'port' is set, then the port written to will be
    // filled in.
    DCBuffer * readany(std::string * port=NULL);
    
    // these write methods copy the argument buffer
    void write(DCBuffer * buf,
               const std::string & port);
    void write(DCBuffer * buf,
               const std::string & port,
               const std::string & label);
    void write(DCBuffer * buf,
               const std::string & port,
               int port_ticket);
    void write_broadcast(DCBuffer * buf, const std::string & port);

    // these write methods avoid copying the argument buffer, and hands it off
    // to another thread of the same process if possible.  Either way, it can
    // only be used on a buffer allocated via 'new' that the caller will not
    // call 'delete' on
    void write_nocopy(DCBuffer * buf, const std::string & port);
    void write_nocopy(DCBuffer * buf,
                      const std::string & port,
                      const std::string & label);
    void write_nocopy(DCBuffer * buf,
                      const std::string & port,
                      int port_ticket);

    // write on a random port (or on the only port if there's only 1)
    void writeany(DCBuffer * buf);
    void writeany_nocopy(DCBuffer * buf);

    std::map< std::string, std::vector<DCFilterInstance*> > get_write_labels(
        std::string port);

    // retrieve 'ticket' associated with last write; can be used to write again
    // to the same filter, if presented to the appropriate write method
    int get_last_write_ticket(const std::string & port);

    // key/value pair lookup
    bool has_param(const std::string & key);
    std::string get_param(const std::string & key);
    int get_param_as_int(const std::string & key);
    double get_param_as_double(const std::string & key);
    DCBuffer get_param_buffer(std::string key);

    // set user statistics
    void set_user_stat(std::string key, std::string value) { stats.user_dict[key] = value; }
    std::string get_user_stat(std::string key) { return stats.user_dict[key];}
    bool has_user_stat(std::string key) { return (stats.user_dict.count(key) > 0);}

private:
    DCBuffer * init_filter_broadcast;
    DCFilterExecutor * executor;
    friend int dcmpi_mpi_mainloop();
    friend int dcmpi_socket_mainloop();
    friend std::string dcmpiruntime_common_statsprep();
    friend class DCLayout;
    friend class DCFilterThread;
    friend class DCFilterExecutor;
    DCFilterStats stats;
};

// some utility methods
int dcmpi_rand(); // makes reentrant calls to rand(); serves as a
                  // non-repeatable random number generator (seeded off of
                  // microsecond-precision wall time)

template<typename T>
inline std::string dcmpi_to_string(const T & value)
{
    ostr streamOut;
    streamOut << value;
    return streamOut.str();
}

inline bool dcmpi_string_starts_with(
    const std::string & subject,
    const std::string & arg)
{
    return (subject.find(arg) == 0);
}

inline bool dcmpi_string_ends_with(
    const std::string & subject,
    const std::string & arg)
{
    return (subject.rfind(arg) == (subject.size() - arg.size()));
}

inline std::vector<std::string> dcmpi_string_tokenize(
    const std::string & str, const std::string & delimiters=" \t\n")
{
    std::vector<std::string> tokens;
    // Skip delimiters at beginning.
    std::string::size_type lastPos = str.find_first_not_of(delimiters, 0);
    // Find first "non-delimiter".
    std::string::size_type pos     = str.find_first_of(delimiters, lastPos);

    while (std::string::npos != pos || std::string::npos != lastPos) {
        // Found a token, add it to the vector.
        tokens.push_back(str.substr(lastPos, pos - lastPos));
        // Skip delimiters.  Note the "not_of"
        lastPos = str.find_first_not_of(delimiters, pos);
        // Find next "non-delimiter"
        pos = str.find_first_of(delimiters, lastPos);
    }
    return tokens;
}

inline bool dcmpi_string_replace(std::string & s,
                                 const std::string & before,
                                 const std::string & after,
                                 bool replace_all = true)
{
    bool out = false;
    std::string::size_type pos;
    while ((pos = s.find(before)) != std::string::npos) {
        out = true;
        s.replace(pos, before.size(), after);
        if (!replace_all) {
            break;
        }
    }
    return out;
}

 // returns true if given string only contains digits,
inline bool dcmpi_string_isnum(std::string s) {
    char c;
    bool isnum = true;
    for(unsigned int i=0; i < s.length() ; i++){
        c = s.at(i);
        if(!isdigit(c)){
            isnum = false;
            break;
        }
    }
    return isnum;
}

inline void dcmpi_endian_swap(void * v, size_t len)
{
    char *_bs,*_be;
    char _btemp;
    _bs = ((char*)v);
    _be = ((char*)v)+len-1;
    while (_bs<_be) {
        _btemp = *_bs; *_bs = *_be; *_be = _btemp;
        _bs++; _be--;
    }
}

#if defined(DCMPI_BIG_ENDIAN)
#define dcmpi_is_big_endian() true
#elif defined(DCMPI_LITTLE_ENDIAN)
#define dcmpi_is_big_endian() false
#else
#error "one of DCMPI_BIG_ENDIAN, DCMPI_LITTLE_ENDIAN must be defined to build dcmpi"
#endif

void dcmpi_args_shift(int & argc, char ** argv, int frompos = 1);
void dcmpi_args_pushback(int & argc, char ** & argv, const char * addme);

int dcmpi_file_lines(const std::string & filename);
std::vector<std::string> dcmpi_file_lines_to_vector(
    const std::string & filename);
bool dcmpi_file_exists(const std::string & filename);
std::string dcmpi_file_dirname(const std::string & filename);
std::string dcmpi_file_basename(const std::string & filename);
std::string dcmpi_get_temp_filename();
std::string dcmpi_get_temp_dir();
std::string dcmpi_find_executable(const std::string & name,
                                  bool fail_on_missing=true);

int dcmpi_mkdir_recursive(const std::string & dir);
int dcmpi_rmdir_recursive(const std::string & dir);

void dcmpi_string_trim_front(std::string & s);
void dcmpi_string_trim_rear(std::string & s);
void dcmpi_string_trim(std::string & s);
void dcmpi_doublesleep(double secs);
double dcmpi_doubletime();

void dcmpi_hostname_to_shortname(std::string & hostname);
std::string dcmpi_get_hostname(bool force_short_name=false);
void dcmpi_gethostbyname(const std::string & name,
                         std::string & out_name,
                         std::vector<std::string> & out_aliases);

std::string dcmpi_get_time();

unsigned long long dcmpi_csnum(const std::string & num);

void dcmpi_sha1(void * message, size_t message_len, unsigned char digest[20]);
std::string dcmpi_sha1_tostring(void * message, size_t message_len);

class DCThread
{
private:
    pthread_t thread;
    bool thread_started;

public:
    DCThread()
    {
        thread_started = false;
    }
    virtual ~DCThread(){}
    void start();
    virtual void run() = 0;
    void join()
    {
        int rc = 0;
        if (thread_started) {
            if ((rc = pthread_join(thread, NULL)) != 0) {
                std::cerr << "ERROR: pthread_join returned " << rc
                          << " at " << __FILE__ << ":" << __LINE__
                          << std::endl << std::flush;
                exit(1);
            }
        }
        else {
            std::cerr << "ERROR: joining non-started thread"
                      << std::endl << std::flush;
        }
    }
        
};

class DCCommandThread : public DCThread
{
    char * command;
    int status;
    bool verbose;
public:
    DCCommandThread(std::string command, bool verbose=false)
    {
        this->command = strdup(command.c_str());
        this->verbose = verbose;
    }
    ~DCCommandThread() {
        free(command);
    }
    void run()
    {
        if (verbose) {
            std::cout << "running command '" << command << "'\n" << std::flush;
        }
        status = system(command);
        if (verbose) {
            std::cout << "command '" << command << "' returned "
                      << status << "\n";
        }
    }
    int getStatus()
    {
        return status;
    }
private:
    DCCommandThread(){} // don't want this
};

#include "dcmpi_provide_defs.h"

#endif /* #ifndef DCMPI_H */
