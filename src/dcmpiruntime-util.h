#include "dcmpi_internal.h"

#include "dcmpi.h"
#include "DCFilterRegistry.h"
#include "MemUsage.h"
#include "dcmpi_util.h"
#include "dcmpi_socket.h"
#include "DCMPIPacket.h"
#include "DCFilterExecutor.h"
#include "ResolvedMultiPort.h"
#include "MultiOutportManager.h"
#include "MPIFromConsoleThread.h"
#include "MPIToConsoleThread.h"
#include "dcmpi_globals.h"

class ProxyWriter;

extern DCLayout *             layout;
extern std::string            localhost;
extern char                   realpath_of_exe[PATH_MAX];
extern std::string cookie;
extern int rank;
extern int size;
extern int dcmpi_rank;
extern int totaldcmpiranks;
extern bool console_filter_present;
extern bool handles_console;
extern std::string console_method;
extern std::string console_to_path;
extern std::string console_from_path;
extern int all_filter_done_messages_routed;
extern int all_filter_done_messages_to_route;

extern MPIToConsoleThread *   mpi_to_console_thread;
extern MPIFromConsoleThread * mpi_from_console_thread;
extern ProxyWriter*           mpi_to_proxy_thread;
extern Queue<DCMPIPacket*> *  mpi_from_console_queue;
extern Queue<DCMPIPacket*> *  mpi_to_console_queue;
extern Queue<DCMPIPacket*> *  mpi_to_proxy_queue;
extern int                    MPIFromConsoleSocket;
extern int                    MPIToConsoleSocket;
extern std::vector<int>       unfinished_local_gftids;
extern DCMutex                unfinished_local_gftids_mutex;
extern std::map<int, DCFilterExecutor*> my_executors; // gftid to executor

// for multi-cluster executions
extern int cluster_rank;
extern int cluster_size;
extern int cluster_offset;
extern std::string proxy_host;
extern bool use_proxy;
extern int proxy_port;
extern int proxy_in_socket;
extern int proxy_out_socket;
typedef enum proxy_state
{
    PROXY_IN_NEED_HEAD,
    PROXY_IN_NEED_BODY
} proxy_state;
extern proxy_state proxy_in_state;
extern DCMPIPacket * proxy_in_packet;
extern int proxy_in_body_length;

#ifdef DEBUG
inline void dbg(const char * format, ...)
{
    va_list ap;
    va_start(ap, format);
    fprintf(stderr, "<%d>", rank);
    vfprintf(stderr, format, ap);
    va_end(ap);
    fflush(stderr);
}
#else
inline void dbg(const char * format, ...)
{
}
#endif

inline std::string id()
{
    std::string out = "d" + dcmpi_to_string(dcmpi_rank) +
        "r" + dcmpi_to_string(rank) +
        "c" + dcmpi_to_string(cluster_rank);
    return out;
}

void dcmpi_install_abort_signal_handler(void);
void dcmpi_abort_handler(int sig);
void dcmpi_use_core_files();
void dcmpi_new_handler();

class ProxyWriter : public DCThread
{
public:
    void run()
    {
        DCMPIPacket * p;
        char hdr[DCMPI_PACKET_HEADER_SIZE];
        while ((p = mpi_to_proxy_queue->get()) != NULL) {
            p->to_bytearray(hdr);
            std::string message = dcmpi_to_string(p->address.to_cluster_rank)
                + " " + dcmpi_to_string(DCMPI_PACKET_HEADER_SIZE +
                                        p->body_len) + "\n";
//             std::cout << "in ProxyWriter: outgoing message header line is "
//                       << message;
//             std::cout << "in ProxyWriter: outgoing packet is "
//                       << *p << std::endl;
//             if (p->packet_type==0) {
//                 dcmpi_abort_handler(11);
//             }
            checkrc(DC_write_all_string(proxy_out_socket, message));
            checkrc(DC_write_all(proxy_out_socket, hdr,
                                 DCMPI_PACKET_HEADER_SIZE));
            if (p->body) {
                checkrc(DC_write_all(proxy_out_socket, p->body->getPtr(),
                                     p->body_len));
                if (p->address.from_rank == dcmpi_rank) {
                    int source_gftid = p->address.from_gftid;
                    MUID_MemUsage[source_gftid]->reduce(p->body_len);
                }
                else {
                    MUID_MemUsage[MUID_of_dcmpiruntime]->reduce(p->body_len);
                }
            }
            delete p;
        }
        checkrc(DC_write_all_string(proxy_out_socket, "bye\n"));
        checkrc(dcmpiCloseSocket(proxy_out_socket));
    }
};

class DCFilterThread : public DCThread
{
    DCFilterExecutor * filter_executor;
public:
    DCFilterThread(DCFilterExecutor * filter_executor)
    {
        this->filter_executor = filter_executor;
    }
    void run()
    {
        int result;
        int i;

        result = filter_executor->filter_runnable->init();
        if (result != 0) {
            std::cerr << "ERROR: filter " << filter_executor->filter_instance->get_distinguished_name()
                      << " init method returned " << result
                      << " on host " << dcmpi_get_hostname()
                      << std::endl << std::flush;
            exit(1);
        }

        filter_executor->filter_runnable->stats.timestamp_process_start =
            dcmpi_doubletime();
        result = filter_executor->filter_runnable->process();
        if (result != 0) {
            std::cerr << "ERROR: filter " << filter_executor->filter_instance->get_distinguished_name()
                      << " process method returned " << result
                      << " on host " << dcmpi_get_hostname()
                      << std::endl << std::flush;
            exit(1);
        }
        filter_executor->filter_runnable->stats.timestamp_process_stop =
            dcmpi_doubletime();

        result = filter_executor->filter_runnable->fini();
        if (result != 0) {
            std::cerr << "ERROR: filter " << filter_executor->filter_instance->get_distinguished_name()
                      << " fini method returned " << result
                      << std::endl << std::flush;
            exit(1);
        }

        // notify all queues this filter writes to that we are done
        std::map<std::string, MultiOutportManager*>::iterator it;
        for (it = filter_executor->out_ports.begin();
             it != filter_executor->out_ports.end();
             it++) {
            MultiOutportManager * mom = it->second;
            for (i = 0; i < mom->num_targets; i++) {
                mom->targets[i]->remove_putter(
                    filter_executor->filter_instance->gftid);
            }
        }

        // "notify" the MPI thread that we are done
        unfinished_local_gftids_mutex.acquire();
        unfinished_local_gftids.erase(
            std::find(unfinished_local_gftids.begin(),
                      unfinished_local_gftids.end(),
                      filter_executor->filter_instance->gftid));
        unfinished_local_gftids_mutex.release();
    }
};

extern DCMutex output_share_socket_mutex;
class OutputRedirector : public DCThread
{
    std::string fifo;
    int s;
    bool is_stderr;
public:
    OutputRedirector(std::string fifo,
                     int s,
                     bool is_stderr = false)
    {
        this->fifo = fifo;
        this->s = s;
        this->is_stderr = is_stderr;
    }
    void run()
    {
        char meta[512];
        char line[2048];
        std::string localhost = dcmpi_get_hostname();
        FILE * f = fopen(fifo.c_str(), "r");
        assert(f);
        char prefix[512];
        strcpy(prefix, localhost.c_str());
        if (is_stderr) {
            strcat(prefix, " red");
        }
        else {
            strcat(prefix, " black");
        }
        while (fgets(line, sizeof(line), f)) {
            output_share_socket_mutex.acquire();
            int len = strlen(line);
            snprintf(meta, sizeof(meta), "%s %d\n", prefix, len);
            if ((DC_write_all(s, meta, strlen(meta)) != 0) ||
                (DC_write_all(s, line, len) != 0)) {
                assert(0);
            }
            output_share_socket_mutex.release();
        }
        fclose(f);
    }
};

inline void parse_args(int argc, char * argv[])
{
    bool in_control_args = false;
    int i = 1;
    while (1) {
        char * arg = strdup(argv[i]);
        char * arg_orig = arg;
        if (arg[0] == '"') {
            arg++;
        }
        if (arg[strlen(arg)-1] == '"') {
            arg[strlen(arg)-1] = 0;
        }
        if (strcmp(arg, "dcmpi_control_args_begin") == 0) {
            in_control_args = true;
        }
        else if (strcmp(arg, "dcmpi_control_args_end") == 0) {
            free(arg_orig);
            break;
        }
        else if (in_control_args) {
            i++;
            char * arg2 = strdup(argv[i]);
            char * arg2_orig = arg2;
            if (arg2[0] == '"') {
                arg2++;
            }
            if (arg2[strlen(arg2)-1] == '"') {
                arg2[strlen(arg2)-1] = 0;
            }
            std::string a1 = arg;
            std::string a2 = arg2;

            if (a1 == "-cluster_rank") {
                cluster_rank = Atoi(a2);
            }
            else if (a1 == "-cluster_size") {
                cluster_size = Atoi(a2);
            }
            else if (a1 == "-cluster_offset") {
                cluster_offset = Atoi(a2);
            }
            else if (a1=="-cookie") {
                cookie = a2;
            }
            else if (a1=="-console") {
                std::vector<std::string> toks = dcmpi_string_tokenize(a2,":");
                console_method = toks[0];
                console_from_path = toks[1];
                console_to_path = toks[2];
            }
            else if (a1 == "-proxy") {
                std::vector<std::string> toks = dcmpi_string_tokenize(a2,":");
                proxy_host = toks[0];
                proxy_port = Atoi(toks[1]);
                use_proxy= true;
            }
            else if (a1 == "-dcmpi_rank") {
                dcmpi_rank = Atoi(a2);
            }
            else if (a1 == "-totaldcmpiranks") {
                totaldcmpiranks = Atoi(a2);
            }
            else if (a1 == "-hasconsolefilter") {
                console_filter_present = (a2 == "yes");
            }
            free(arg2_orig);
        }
        else {
            ;
        }
        i++;
        free(arg_orig);
    }
}

int dcmpiruntime_common_execute(
    int (*broadcast_layout_func)(char hdr[DCMPI_PACKET_HEADER_SIZE]),
    int (*mainloop_func)());

// convenient breakpoint to set; once this breakpoint is hit, it is guaranteed
// that other breakpoints can be set in user filter code, as the .so library
// files have been loaded by now.
void dcmpi_filters_loaded(int num_filters);

void dcmpiruntime_common_startup(int argc, char * argv[]);
void dcmpiruntime_common_cleanup();
std::string dcmpiruntime_common_statsprep();
