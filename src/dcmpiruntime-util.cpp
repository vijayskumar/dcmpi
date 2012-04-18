#include "dcmpiruntime-util.h"

using namespace std;

DCLayout *             layout;
std::string            localhost;
char                   realpath_of_exe[PATH_MAX];
std::string cookie;
int rank;
int size;
uint usize;
int dcmpi_rank;
int totaldcmpiranks;
bool console_filter_present;
bool handles_console = false;
std::string console_method;
std::string console_to_path;
std::string console_from_path;
int all_filter_done_messages_routed = -9999;
int all_filter_done_messages_to_route = -9999;

MPIToConsoleThread *   mpi_to_console_thread = NULL;
MPIFromConsoleThread * mpi_from_console_thread = NULL;
ProxyWriter*           mpi_to_proxy_thread = NULL;
Queue<DCMPIPacket*> *  mpi_from_console_queue = NULL;
Queue<DCMPIPacket*> *  mpi_to_console_queue = NULL;
Queue<DCMPIPacket*> *  mpi_to_proxy_queue = NULL;
int                    MPIFromConsoleSocket = -1;
int                    MPIToConsoleSocket   = -1;
std::vector<int>       unfinished_local_gftids;
DCMutex                unfinished_local_gftids_mutex;
std::map<int, DCFilterExecutor*> my_executors; // gftid to executor

// for multi-cluster executions
int cluster_rank = -9999;
int cluster_size = -9999;
int cluster_offset = -9999;
std::string proxy_host;
bool use_proxy = false;
int proxy_port = -1;
int proxy_in_socket = -1;
int proxy_out_socket = -1;
proxy_state proxy_in_state = PROXY_IN_NEED_HEAD;
DCMPIPacket * proxy_in_packet = NULL;
int proxy_in_body_length = -1;

DCMutex output_share_socket_mutex;

void dcmpi_abort_handler(int sig)
{
//     dcmpi_print_backtrace();
    std::cerr << "ERROR: dcmpi_segv_handler called"
              << " at " << __FILE__ << ":" << __LINE__
              << std::endl << std::flush;
    assert( sig == SIGSEGV || sig == SIGABRT );
    std::string gdb = dcmpi_find_executable("gdb", true);
    std::string command = "xterm -e ";
    command += gdb + " ";
    command += dcmpi_remote_argv_0;
    command += " ";
    command += dcmpi_to_string(getpid());
    std::cerr << "dcmpi_abort_handler running command " << command
              << std::endl << std::flush;
    if (system(command.c_str())) {
        std::cerr << "ERROR:  executing command " << command
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        exit(1);
    }
    std::cerr << "exiting after spawned debugger exited"
              << " at " << __FILE__ << ":" << __LINE__
              << std::endl << std::flush;
    exit(1);
}

void dcmpi_install_abort_signal_handler(void)
{    
    struct sigaction s;
    s.sa_handler = dcmpi_abort_handler;
    sigemptyset(&s.sa_mask);
    s.sa_flags = 0;
    if (sigaction(SIGSEGV, &s, NULL) != 0) {
        std::cerr << "ERROR:  installing signal handler for SIGSEGV"
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        exit(1);
    }
    if (sigaction(SIGABRT, &s, NULL) != 0) {
        std::cerr << "ERROR:  installing signal handler for SIGABRT"
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        exit(1);
    }
    std::cout << "dcmpi signal handler installed!\n";
}

void dcmpi_use_core_files()
{
    struct rlimit rlnew;
    rlnew.rlim_cur = rlnew.rlim_max = (rlim_t)2147483647; // 2gig core files
                                                          // should be
                                                          // sufficient
    if (setrlimit(RLIMIT_CORE, &rlnew) != 0) {
//         std::cerr << "ERROR: calling setrlimit()"
//                   << " at " << __FILE__ << ":" << __LINE__
//                   << ", errno=" << errno
//                   << std::endl << std::flush;
    }
}

void dcmpi_new_handler()
{
    std::cerr << "ERROR: dcmpi ran out of heap memory on host "
              << dcmpi_get_hostname() << "!  Aborting "
              << " at " << __FILE__ << ":" << __LINE__
              << std::endl << std::flush;
    std::string command = "/usr/bin/top -b -n 1 -p ";
    command += dcmpi_to_string(getpid());
    system(command.c_str());
    exit(1);
}

void dcmpi_filters_loaded(int num_filters)
{
    if (dcmpi_verbose()) {
        std::cerr << "host " << dcmpi_get_hostname()
                  << ": " << num_filters << " filters loaded\n";
    }
}

int dcmpiruntime_common_execute(
    int (*broadcast_layout_func)(char hdr[DCMPI_PACKET_HEADER_SIZE]),
    int (*mainloop_func)())
{
    DCThread * stdout_redirector_thread = NULL;
    DCThread * stderr_redirector_thread = NULL;
    std::string stdout_log;
    std::string stderr_log;
    std::string log_dir;
    std::string localhost = dcmpi_get_hostname();
    int rc = 0;
    char * remote_output_env;
    int redirect_socket = -1;

    char hdr[DCMPI_PACKET_HEADER_SIZE];
    if (rank == 0) {
        if (cluster_rank == 0) {
            handles_console = true;
        
            // get the layout first, distribute, then hand console duties off to
            // threads
            if (console_method == "unix") {
                MPIFromConsoleSocket = dcmpiOpenUnixClientSocket(
                    console_from_path.c_str());
                MPIToConsoleSocket = dcmpiOpenUnixClientSocket(
                    console_to_path.c_str());
            }
            else if (console_method == "tcp") {
                std::vector<std::string> from_ips;
                uint2 from_port;
                std::vector<std::string> to_ips;
                uint2 to_port;
                std::vector<std::string> toks;

                toks = dcmpi_string_tokenize(console_from_path, "/");
                from_ips = dcmpi_string_tokenize(toks[0],",");
                from_port = atoi(toks[1].c_str());
            
                toks = dcmpi_string_tokenize(console_to_path, "/");
                to_ips = dcmpi_string_tokenize(toks[0],",");
                to_port = atoi(toks[1].c_str());

                for (uint i3 = 0; i3 < from_ips.size(); i3++) {
                    MPIFromConsoleSocket = dcmpiOpenClientSocket(
                        from_ips[i3].c_str(),
                        from_port);
                    MPIToConsoleSocket = dcmpiOpenClientSocket(
                        to_ips[i3].c_str(),
                        to_port);
                    if (MPIFromConsoleSocket != -1 &&
                        MPIToConsoleSocket != -1) {
                        break;
                    }
                }
            }

            if (MPIFromConsoleSocket == -1) {
                std::cerr << "ERROR: cannot open socket"
                          << " at " << __FILE__ << ":" << __LINE__
                          << std::endl << std::flush;
                exit(1);
            }
            if (MPIToConsoleSocket == -1) {
                std::cerr << "ERROR: cannot open socket"
                          << " at " << __FILE__ << ":" << __LINE__
                          << std::endl << std::flush;
                exit(1);
            }
        }

        // connect to proxy server
        if (use_proxy) {
            proxy_in_socket = dcmpiOpenClientSocket(
                proxy_host.c_str(), proxy_port);
            proxy_out_socket = dcmpiOpenClientSocket(
                proxy_host.c_str(), proxy_port);
            if (proxy_in_socket==-1 || proxy_out_socket==-1) {
                std::cerr << "ERROR: cannot connect to proxy "
                          << proxy_host << ":" << proxy_port
                          << " from host " << dcmpi_get_hostname()
                          << " at " << __FILE__ << ":" << __LINE__
                          << std::endl << std::flush;
                exit(1);
            }

            // write temporary auth line to proxy server
            checkrc(DC_write_all_string(proxy_in_socket, "dcmpiwuzzy\n"));
            checkrc(DC_write_all_string(proxy_out_socket, "dcmpiwuzzy\n"));

            // write cookie line to proxy server
            checkrc(DC_write_all_string(proxy_in_socket, cookie));
            checkrc(DC_write_all_string(proxy_in_socket, "\n"));
            checkrc(DC_write_all_string(proxy_out_socket, cookie));
            checkrc(DC_write_all_string(proxy_out_socket, "\n"));

            // write my cluster rank to proxy server
            checkrc(DC_write_all_string(proxy_in_socket, dcmpi_to_string(cluster_rank)));
            checkrc(DC_write_all_string(proxy_in_socket, "\n"));
            checkrc(DC_write_all_string(proxy_out_socket, dcmpi_to_string(cluster_rank)));
            checkrc(DC_write_all_string(proxy_out_socket, "\n"));

            // write cluster size to proxy server
            checkrc(DC_write_all_string(proxy_in_socket, dcmpi_to_string(cluster_size)));
            checkrc(DC_write_all_string(proxy_in_socket, "\n"));
            checkrc(DC_write_all_string(proxy_out_socket, dcmpi_to_string(cluster_size)));
            checkrc(DC_write_all_string(proxy_out_socket, "\n"));

            // write "in" or "out" (from proxy point of view) to proxy
            checkrc(DC_write_all_string(proxy_in_socket, "out\n"));
            checkrc(DC_write_all_string(proxy_out_socket, "in\n"));
        }

        int i;
        if (cluster_rank == 0) {
            checkrc(DC_read_all(MPIFromConsoleSocket, hdr, sizeof(hdr)));
            if (cluster_size > 1) {
                for (i = 1; i < cluster_size; i++) {
                    std::string message = dcmpi_to_string(i) + " " +
                        dcmpi_to_string(sizeof(hdr)) + "\n";
                    checkrc(DC_write_all_string(proxy_out_socket, message));
                    checkrc(DC_write_all(proxy_out_socket,
                                         hdr, sizeof(hdr)));
                }
            }
        }
        else {
            std::cout << "INFO: on " << dcmpi_get_hostname()
                      << ": cluster_rank is " << cluster_rank
                      << endl;
            std::string line = dcmpi_socket_read_line(proxy_in_socket);
            assert(line == dcmpi_to_string(sizeof(hdr)));
            checkrc(DC_read_all(proxy_in_socket, hdr, sizeof(hdr)));
        }
    }
    
    layout = new DCLayout();
    checkrc(broadcast_layout_func(hdr));

    if (dcmpi_verbose()) {
        cout << "dcmpi_rank " << dcmpi_rank
             << " rank " << rank
             << " cluster_rank " << cluster_rank
             << " cluster_size " << cluster_size
             << " cluster_offset " << cluster_offset
             << " totaldcmpiranks " << totaldcmpiranks
             << " console_filter_present " << console_filter_present
             << " machine " << dcmpi_get_hostname()
             << endl;
    }

    // init globals after env. vars have been propagated
    init_dcmpiruntime_globals(layout->filter_instances.size());

    if (getenv("DCMPI_FILTER_SUPPORT_LIBRARY_PATH")) {
        std::string newval = "LD_LIBRARY_PATH=";
        newval += getenv("DCMPI_FILTER_SUPPORT_LIBRARY_PATH");
        char * oldval = getenv("LD_LIBRARY_PATH");
        if (oldval) {
            newval += ":";
            newval += oldval;
        }
        putenv(strdup(newval.c_str()));
    }

    if (getenv("DCMPI_JIT_GDB")) {
        dcmpi_install_abort_signal_handler();
    }

    if ((rank == 0) && (cluster_rank == 0)) {
        mpi_from_console_queue = new Queue<DCMPIPacket*>;
        std::set<int> s;
        s.insert(-1);
        mpi_from_console_queue->set_putters(s); // symbolically -1
        mpi_from_console_thread = new MPIFromConsoleThread(
            MPIFromConsoleSocket, mpi_from_console_queue, layout);
     
        mpi_to_console_queue = new Queue<DCMPIPacket*>;
        mpi_to_console_thread = new MPIToConsoleThread(
            MPIToConsoleSocket, mpi_to_console_queue, layout);
    }

    if (rank == 0 && use_proxy) {
        mpi_to_proxy_queue = new Queue<DCMPIPacket*>;
        mpi_to_proxy_thread = new ProxyWriter;
    }

    // execute local environment hooks
    std::vector<std::string> env_hooks =
        get_config_filenames("env_hooks");
    for (int i = 0; i < 2; i++) {
        if (env_hooks[i] != "") {
            std::vector<std::string> lines =
                get_config_noncomment_nonempty_lines(env_hooks[i]);
            for (uint u2 = 0; u2 < lines.size(); u2++) {
                std::string line = lines[u2];
                if (line.find(" ") == std::string::npos) {
                    std::cerr << "ERROR: invalid line " << line
                              << " in " << env_hooks[i]
                              << std::endl << std::flush;
                    exit(1);
                }
                std::string key = line.substr(0, line.find(" "));
                std::string val = line.substr(line.find(" ") + 1);
                setenv(key.c_str(), val.c_str(), 1);
                if (dcmpi_verbose()) {
                    std::cout << "env_hooks: setenv "
                              << key << " "
                              << val
                              << endl;
                }
            }
        }
    }
    
    if (getenv("DCMPI_FORCE_DISPLAY")) {
        cout << "NOTE:  forcing display to " << getenv("DCMPI_FORCE_DISPLAY")
             << " on host " << dcmpi_get_hostname()
             << endl;
        setenv("DISPLAY", getenv("DCMPI_FORCE_DISPLAY"), 1);
    }

    remote_output_env = getenv("DCMPI_REMOTE_OUTPUT");

    // relocate output if desired
    if (remote_output_env) {
        if (!strcmp(remote_output_env,"xterm-vi")) {
            stdout_log = dcmpi_get_temp_filename();
            stderr_log = dcmpi_get_temp_filename();
            ostr o;
            o << "xterm -T " << localhost << " -e ssh -t "
              << localhost
              << " vi " << stdout_log << " &";
            cerr << o.str() << endl;
            freopen(stdout_log.c_str(),"w",stdout);
            freopen(stderr_log.c_str(),"w",stderr);
        }
        // syntax rove:hostname:port
        else if (!strncmp(remote_output_env,"rove:",5)) {
            log_dir = dcmpi_get_temp_dir();
            stdout_log = log_dir + "/stdout";
            stderr_log = log_dir + "/stderr";
            // make a fifo
            if (mkfifo(stdout_log.c_str(), 0600) ||
                mkfifo(stderr_log.c_str(), 0600)) {
                std::cerr << "ERROR: calling mkfifo()"
                          << " at " << __FILE__ << ":" << __LINE__
                          << std::endl << std::flush;
                exit(1);
            }
            std::vector<std::string> tokens = dcmpi_string_tokenize(remote_output_env, ":");
            if (tokens.size()!= 3) {
                std::cerr << "ERROR:  invalid syntax '" << remote_output_env << "', "
                          << "please use syntax rove:hostname:port"
                          << " at " << __FILE__ << ":" << __LINE__
                          << std::endl << std::flush;
                exit(1);
            }
            std::string rove_hostname = tokens[1];
            int rove_port = Atoi(tokens[2]);
            redirect_socket = dcmpiOpenClientSocket(rove_hostname.c_str(), rove_port);
            if (redirect_socket == -1) {
                std::cerr << "ERROR:  cannot connect to rove at "
                          << rove_hostname << ":" << rove_port
                          << " from host " << localhost
                          << " at " << __FILE__ << ":" << __LINE__
                          << std::endl << std::flush;
                exit(1);
            }
            std::string meta = localhost + " blue ";
            std::string message = "dcmpi started at " + dcmpi_get_time() + "\n";
            meta += dcmpi_to_string(message.size()) + "\n";
            if (DC_write_all(redirect_socket, meta.c_str(), meta.size()) != 0 || DC_write_all(redirect_socket, message.c_str(), message.size()) != 0) {
                std::cerr << "ERROR:  writing to " << rove_hostname << ":"
                          << rove_port
                          << " at " << __FILE__ << ":" << __LINE__
                          << std::endl << std::flush;
                exit(1);
            }
            stdout_redirector_thread = new OutputRedirector(stdout_log, redirect_socket);
            stderr_redirector_thread = new OutputRedirector(stderr_log, redirect_socket, true);
            stdout_redirector_thread->start();
            stderr_redirector_thread->start();
            freopen(stdout_log.c_str(),"w",stdout);
            freopen(stderr_log.c_str(),"w",stderr);
        }
    }

    if (getenv("DCMPI_GDBLAUNCHER")) {
        // send my details to host:port
        std::vector<std::string> tokens = dcmpi_string_tokenize(getenv("DCMPI_GDBLAUNCHER"), ":");
        if (tokens.size()!=2) {
            std::cerr << "ERROR: invalid DCMPI_GDBLAUNCHER value"
                      << " at " << __FILE__ << ":" << __LINE__
                      << std::endl << std::flush;
            exit(1);
        }
        std::string host = tokens[0];
        int port = Atoi(tokens[1]);
        dcmpi_doublesleep(rank * 0.5);
        int s = dcmpiOpenClientSocket(host.c_str(), port);
        if (s < 0) {
            std::cerr << "ERROR:  connecting to gdblauncher at "
                      << getenv("DCMPI_GDBLAUNCHER")
                      << " on host " << dcmpi_get_hostname()
                      << std::endl << std::flush;
            exit(1);
        }
        std::string message = dcmpi_get_hostname() + " ";
        message += realpath_of_exe;
        message += " " + dcmpi_to_string(getpid()) + "\n";
        if (DC_write_all(s, message.c_str(), message.size())) {
            std::cerr << "ERROR:  writing to " << getenv("DCMPI_GDBLAUNCHER")
                      << " at " << __FILE__ << ":" << __LINE__
                      << std::endl << std::flush;
            exit(1);
        }
        checkrc(close(s));
    }
    
    rc = mainloop_func();

    if (handles_console) {
        mpi_to_console_queue->put(NULL);
        mpi_from_console_thread->join();
        delete mpi_from_console_queue;
        delete mpi_from_console_thread;
        mpi_to_console_thread->join();
        delete mpi_to_console_queue;
        delete mpi_to_console_thread;
    }
    if (rank == 0 && use_proxy) {
        mpi_to_proxy_queue->put(NULL);
        mpi_to_proxy_thread->join();
        delete mpi_to_proxy_thread;
    }
    
    if (remote_output_env) {
        fclose(stdout);
        fclose(stderr);
    }
    if (stdout_redirector_thread) {
        stdout_redirector_thread->join();
        stderr_redirector_thread->join();
        remove(stdout_log.c_str());
        remove(stderr_log.c_str());
        rmdir(log_dir.c_str());
        std::string meta = localhost + " blue ";
        std::string message = "dcmpi stopped at " + dcmpi_get_time() + "\n";
        meta += dcmpi_to_string(message.size()) + "\n";
        if (DC_write_all(redirect_socket, meta.c_str(), meta.size()) != 0 || DC_write_all(redirect_socket, message.c_str(), message.size()) != 0) {
            std::cerr << "ERROR:  writing to rove"
                      << " at " << __FILE__ << ":" << __LINE__
                      << std::endl << std::flush;
            exit(1);
        }
        close(redirect_socket);
    }

    fini_dcmpiruntime_globals();

    delete layout;

    return rc;
}

void dcmpiruntime_common_startup(int argc, char * argv[])
{
    dcmpi_is_local_console = false;
    set_new_handler(dcmpi_new_handler);
    dcmpi_use_core_files();
    localhost = dcmpi_get_hostname();
    if (realpath(argv[0], realpath_of_exe) == NULL) {
        std::cerr << "ERROR: calling realpath"
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        exit(1);
    }
    dcmpi_remote_argv_0 = realpath_of_exe;

}
void dcmpiruntime_common_cleanup()
{
    if (proxy_in_socket != -1) {
        std::string line = dcmpi_socket_read_line(proxy_in_socket);
        assert(line == "bye");
        dcmpiCloseSocket(proxy_in_socket);
        proxy_in_socket = -1;
    }

#ifdef DCMPI_HAVE_JAVA
    fini_java();
#endif

#ifdef DCMPI_HAVE_PYTHON
    fini_python();
#endif
}

std::string dcmpiruntime_common_statsprep()
{
    ostr o;
    o << "BEGIN FILTER STATS ON " << dcmpi_get_hostname() << endl;
    o << 
        "FILTER_NAME                           ELAPSED      READ     WRITE   COMPUTE\n"
        "-----------                           -------      ----     -----   -------\n";
    char name[128];
    std::map<int, DCFilterExecutor*>::iterator it;
    for (it = my_executors.begin(); it != my_executors.end(); it++) {
        DCFilterExecutor * exe = it->second;
        DCFilter * runnable = exe->filter_runnable;
        DCFilterStats & stats = runnable->stats;
        double read_block_time = 0.0;
        double write_block_time = 0.0;
        std::map< std::string, double>::iterator it;
        for (it = stats.read_block_time.begin();
             it != stats.read_block_time.end();
             it++) {
            read_block_time += it->second;
        }
        for (it = stats.write_block_time.begin();
             it != stats.write_block_time.end();
             it++) {
            write_block_time += it->second;
        }
        char line[512];
        double elapsed = stats.timestamp_process_stop - stats.timestamp_process_start;
        double compute_time = (elapsed - read_block_time)-write_block_time;
        if (write_block_time < 0.0001) {
            write_block_time = 0.0; // negligible
        }
        snprintf(line, sizeof(line), "%-12.2f %-8.2f %-7.2f %-.2f",
                 elapsed, read_block_time, write_block_time, compute_time);
        snprintf(name,sizeof(name), "%-37s",
                 exe->filter_instance->get_distinguished_name().c_str());
        o << name << " " << line << endl;
    }

    o << "\nBEGIN PORT READ/WRITE STATS ON "
      << dcmpi_get_hostname() << endl;
    o << "*******************************************************************************\n";
    for (it = my_executors.begin(); it != my_executors.end(); it++) {
        DCFilterExecutor * exe = it->second;
        DCFilter * runnable = exe->filter_runnable;
        DCFilterStats & stats = runnable->stats;
        std::map< std::string, double>::iterator it;
        char fl[64];
        for (it = stats.read_block_time.begin();
             it != stats.read_block_time.end();
             it++) {
            snprintf(fl, sizeof(fl), "%-4.2f", it->second);
            o << "filter " << exe->filter_instance->get_distinguished_name()
              << " read for " << fl
              << " seconds on port " << it->first << endl;
        }
        for (it = stats.write_block_time.begin();
             it != stats.write_block_time.end();
             it++) {
            snprintf(fl, sizeof(fl), "%-4.2f", it->second);
            o << "filter " << exe->filter_instance->get_distinguished_name()
              << " wrote for " << fl
              << " seconds on port " << it->first << endl;
        }
    }

    bool printed_hdr = false;
    for (it = my_executors.begin(); it != my_executors.end(); it++) {
        DCFilterExecutor * exe = it->second;
        DCFilter * runnable = exe->filter_runnable;
        DCFilterStats & stats = runnable->stats;
        std::map<std::string, std::string>::iterator it = stats.user_dict.begin();
        while (it != stats.user_dict.end()) {
            if (!printed_hdr) {
                o << "\nBEGIN USER STATS ON "
                  << dcmpi_get_hostname() << endl;
                o << "*******************************************************************************\n";
                printed_hdr=true;
            }
            o << exe->filter_instance->get_distinguished_name() << ":" << it->first << " = " << it->second << endl;
            it++;  
        }
    }
    o << endl << endl;
    return o.str();
}
