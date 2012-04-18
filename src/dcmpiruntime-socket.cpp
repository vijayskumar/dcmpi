#include "dcmpiruntime-util.h"

#include <sys/time.h>
#include <sys/resource.h>
#include <sys/poll.h>

using namespace std;

//dcmpisock-specifics
int dcmpisock_listensocket = -1;
std::vector<DCCommandThread*> remote_processes;
vector<std::string> rank_hosts;
int * rank_ports = NULL;
int * rank_fds = NULL;
int * rank0_barrier_fds = NULL;
int rank_non0_barrier_fd = -1;

char * appname = NULL;
void usage()
{
    printf("usage: %s ...\n       dcmpi_control_args_begin [args...] "
           "dcmpi_control_args_end ...\n", appname);
    printf("NOTE: this executable is for dcmpi internal use only.\n");
    exit(EXIT_FAILURE);
}

static void socket_bcast(void * message, int sz)
{
    if (rank==0) {
        for (int i = 1; i < size; i++) {
            checkrc(DC_write_all(rank_fds[i], message, sz));
        }
    }
    else {
        checkrc(DC_read_all(rank_fds[0], message, sz));
    }
}

static void socket_barrier(void)
{
    // barrier
    char zero = 0;
    if (rank == 0) {
        int i;
        for (i = 1; i < size; i++) { // wait for all other ranks to get here
            checkrc(DC_read_all(rank0_barrier_fds[i], &zero, 1));
        }
        for (i = 1; i < size; i++) { // tell other ranks they can exit
            checkrc(DC_write_all(rank0_barrier_fds[i], &zero, 1));
        }
    }
    else {
        checkrc(DC_write_all(rank_non0_barrier_fd, &zero, 1));
        checkrc(DC_read_all(rank_non0_barrier_fd, &zero, 1));
    }
}

int dcmpi_socket_get_and_broadcast_layout(char hdr[DCMPI_PACKET_HEADER_SIZE])
{
    DCMPIPacket layout_head;

    socket_bcast(hdr, DCMPI_PACKET_HEADER_SIZE);
    layout_head.init_from_bytearray(hdr);
    DCBuffer layout_buf(layout_head.body_len);
    if (rank == 0) {
        if (cluster_rank == 0) {
            checkrc(DC_read_all(MPIFromConsoleSocket, layout_buf.getPtr(),
                                layout_head.body_len));
            for (int i = 1; i < cluster_size; i++) {
                std::string message = dcmpi_to_string(i) + " " +
                    dcmpi_to_string(layout_head.body_len) + "\n";
                checkrc(DC_write_all_string(proxy_out_socket, message));
                checkrc(DC_write_all(proxy_out_socket,
                                     layout_buf.getPtr(),
                                     layout_head.body_len));
            }
        }
        else {
            std::string line = dcmpi_socket_read_line(proxy_in_socket);
            assert(line == dcmpi_to_string(layout_head.body_len));
            checkrc(DC_read_all(proxy_in_socket, layout_buf.getPtr(),
                                layout_head.body_len));
        }
    }
    else {
        ;
    }

    uint1 rank_zeros_big_endianness = (dcmpi_is_big_endian())?1:0;
    socket_bcast(&rank_zeros_big_endianness, sizeof(uint1));
    
	layout_buf.forceEndian(rank_zeros_big_endianness == 1);
    layout_buf.setUsedSize(layout_head.body_len);
    socket_bcast(layout_buf.getPtr(), layout_head.body_len);
    layout->deSerialize(&layout_buf);

    dcmpi_remote_console_snarfed_layout = true;
    
#ifdef DEBUG3
    cout << "** in rank " << rank << " LAYOUT IS **\n" << *layout << endl;
#endif

    // send/receive cluster rank, size, etc.    
    socket_bcast(&cluster_rank, sizeof(int));
    socket_bcast(&cluster_size, sizeof(int));
    socket_bcast(&cluster_offset, sizeof(int));
    socket_bcast(&totaldcmpiranks, sizeof(int));
    int cfp = -1;
    if (rank == 0) {
        cfp = (console_filter_present)?1:0;
    }
    socket_bcast(&cfp, sizeof(int));
    
    if(dcmpi_is_big_endian() ^ (rank_zeros_big_endianness == 1)) {
    	dcmpi_endian_swap(&cluster_rank, sizeof(int));
    	dcmpi_endian_swap(&cluster_size, sizeof(int));
    	dcmpi_endian_swap(&cluster_offset, sizeof(int));
    	dcmpi_endian_swap(&totaldcmpiranks, sizeof(int));
    	dcmpi_endian_swap(&cfp, sizeof(int));
    }
#ifdef DEBUG3    
    cout << "In rank " << rank << " " << cluster_rank << " " << cluster_size << " " << cluster_offset << " " << totaldcmpiranks << " " << cfp << endl;
#endif     
    if (rank) {
        console_filter_present = (cfp == 1);
    }
    if (rank == 0) {
        int r;
        for (int i = 1; i < size; i++) {
            r = dcmpi_rank + i;
            dcmpi_socket_write_line(rank_fds[i], tostr(r));
        }
    }
    else {
        dcmpi_rank = Atoi(dcmpi_socket_read_line(rank_fds[0]));
    }
    return 0;
}

void dcmpisock_deliver_in(DCMPIPacket * p,
                          std::map<int, DCFilterExecutor*> & my_executors,
                          int & num_all_filters_done_received)
{
    switch(p->packet_type)
    {

#define DCMPI_DEBUG_MPIRECV_FUNC(buffer, size, rnk, fn, port) { cout << "DEBUG_MPIRECV on " << dcmpi_get_hostname(true) << ": received buffer of size " << size << " from rank " << rnk << " for filter " << fn << ", port " << port << " with contents: "; for (int i = 0; i < (size); i++) { printf(" %x", (unsigned int)((unsigned char)(buffer[i]))); if (i == 64) { printf("..."); break; } } cout << endl; }
        case DCMPI_PACKET_TYPE_DCBUF:
        {
#ifdef DEBUG
            assert(p->body_len >= 0);
            assert(p->address.to_rank == dcmpi_rank);
#endif
#ifdef DCMPI_DEBUG_MPIRECV
            DCMPI_DEBUG_MPIRECV_FUNC(
                p->body->getPtr(), p->body->getUsedSize(), p->address.from_rank,
                my_executors[p->address.to_gftid]->
                filter_instance->get_distinguished_name(),
                p->address.to_port);
#endif
            Buffer_MUID bm(p->body, MUID_of_dcmpiruntime);
            my_executors[p->address.to_gftid]->in_ports[
                p->address.to_port]->put(bm);
            p->body = NULL; // prevent destructor for p from freeing body
            break;
        }
        case DCMPI_PACKET_TYPE_REMOVE_PUTTER:
            my_executors[p->address.to_gftid]->in_ports[p->address.to_port]->remove_putter(p->address.from_gftid);
            break;
        case DCMPI_PACKET_TYPE_ALL_MY_FILTERS_DONE:
        {
//             cout << id();
//             cout << ": got ALL_FILTERS_DONE from ";
//             cout << p->address.from_rank << endl;
            num_all_filters_done_received++;
            break;
        }
        default:
            assert(0);
    }
    delete p;
}

static DCMutex dcmpisock_deliver_elsewhere_mutex;
void dcmpisock_deliver_elsewhere(DCMPIPacket * p)
{
    if (p->packet_type == DCMPI_PACKET_TYPE_ALL_MY_FILTERS_DONE &&
        p->address.from_rank != dcmpi_rank) {
        dcmpisock_deliver_elsewhere_mutex.acquire();
        all_filter_done_messages_routed += 1;
#ifdef DEBUG
//         cout << "all_filter_done_messages_routed now "
//              << all_filter_done_messages_routed << endl;
        assert(all_filter_done_messages_routed <=
               all_filter_done_messages_to_route);
#endif
        dcmpisock_deliver_elsewhere_mutex.release();
    }
    if ((p->address.to_rank == -1) && handles_console) {
        mpi_to_console_queue->put(p); // memusage reduced over there
    }
    else if (rank == 0 && use_proxy &&
             p->address.to_cluster_rank != cluster_rank) {
//         std::cout << "rank 0 of cluster_rank " << cluster_rank
//                   << ": putting message of size "
//                   << p->body_len << " from local rank "
//                   << p->address.from_rank
//                   << " on mpi_to_proxy_queue\n" << flush;
        mpi_to_proxy_queue->put(p); // memusage reduced over there
    }
    else {
        int outrank;
        if (p->address.to_rank == -1) {
            // send to rank -1 by routing through rank 0
            outrank = 0;
        }
        else if (p->address.to_cluster_rank != cluster_rank) {
            // send to other cluster by routing through rank 0
            outrank = 0;
//             std::cout << "host " << dcmpi_get_hostname()
//                       << " sending out to rank 0 for proxy purposes: "
//                       << " packet of size " << p->body_len << endl << flush;
        }
        else {
            // send via MPI, convert to MPI rank system
            outrank = p->address.to_rank - cluster_offset;
        }
        p->to_bytearray();
        if (rank_fds[outrank] == -1) {
            std::cerr << "ERROR: outrank of " << outrank
                      << " on host " << dcmpi_get_hostname()
                      << " at " << __FILE__ << ":" << __LINE__
                      << std::endl << std::flush;
            std::cout << "packet type: " << p->packet_type << endl;
            assert(0);
        }
        int rc = DC_write_all(rank_fds[outrank],
                              p->internal_hdr, DCMPI_PACKET_HEADER_SIZE);
        if (rc) {
            std::cerr << "ERROR: rc=" << rc
                      << " writing packet header on host "
                      << dcmpi_get_hostname()
                      << " to host " << rank_hosts[outrank]
                      << " at " << __FILE__ << ":" << __LINE__
                      << std::endl << std::flush;
            assert(0);
        }
        if (p->body == NULL) {
            ;
        }
        else {
#define DCMPI_DEBUG_MPISEND_FUNC(buffer, size, rnk) { cout << "DEBUG_MPISEND on " << dcmpi_get_hostname(true) << ": sending out buffer of size " << size << " to rank " << rnk << " with contents: "; for (int i = 0; i < (size); i++) { printf(" %x", (unsigned int)((unsigned char)(buffer[i]))); if (i == 64) { printf("..."); break; } } cout << endl; }

            char * offset = p->body->getPtr();
#ifdef DCMPI_DEBUG_MPISEND
            DCMPI_DEBUG_MPISEND_FUNC(offset, p->body_len, outrank);
#endif
            checkrc(DC_write_all(rank_fds[outrank], offset, p->body_len));

            if (handles_console && p->address.from_rank==-1) {
                MUID_MemUsage[MUID_of_dcmpiruntime]->reduce(
                    p->body->getUsedSize());
            }
            // routed from proxy
            else if (p->address.from_cluster_rank != cluster_rank) {
                MUID_MemUsage[MUID_of_dcmpiruntime]->reduce(
                    p->body->getUsedSize());
            }
            else {
                MUID_MemUsage[p->address.from_gftid]->reduce(
                    p->body->getUsedSize());
            }
        }
        delete p;
    }
}

class outq_handler : public DCThread
{
    Queue<DCMPIPacket*> * single_mpi_outqueue;
public:
    outq_handler(Queue<DCMPIPacket*> * single_mpi_outqueue_) :
    single_mpi_outqueue(single_mpi_outqueue_)
    {
        ;
    }
    void run()
    {
        while (1) {
            DCMPIPacket * p = single_mpi_outqueue->get();
            if (!p) {
                break;
            }
            dcmpisock_deliver_elsewhere(p);
        }
        assert(single_mpi_outqueue->size() == 0);
    }
};

int dcmpi_socket_mainloop()
{
    int i;
    uint u, u2;
    Queue<DCMPIPacket*> single_mpi_outqueue;
    std::vector<DCThread*> my_threads;
    std::map<int, DCFilterInstance*> gftids_to_instances;
    std::set<int> dcmpi_send_ranks;
    std::map<int, int> dcmpirank_clusterrank;
    int multi_output_policy = DCMPI_MULTIOUT_POLICY_ROUND_ROBIN;

    // allocate all filters
    for (u = 0; u < layout->filter_instances.size(); u++) {
        DCFilterInstance * fi = layout->filter_instances[u];
        gftids_to_instances[fi->gftid] = fi;
        if (fi->dcmpi_rank == dcmpi_rank) {
            DCFilter * f = layout->filter_reg->get_filter(fi->filtername);
            if (layout->init_filter_broadcast) {
                f->init_filter_broadcast =
                    new DCBuffer(*(layout->init_filter_broadcast));
            }
            DCFilterExecutor * new_executor = new DCFilterExecutor();
            DCFilterThread * t = new DCFilterThread(new_executor);
            my_threads.push_back(t);
            new_executor->setup(fi, f); // creates in-queues
            my_executors[fi->gftid] = new_executor;
            f->executor = new_executor;
            unfinished_local_gftids.push_back(fi->gftid);
        }
        dcmpirank_clusterrank[fi->dcmpi_rank] = fi->dcmpi_cluster_rank;
    }

    // determine memory usage size per filter
    // divide it up equally among all filters+runtime for now
    int8 user_specified_memory = get_user_specified_write_buffer_space();
    int8 amount_per_memusage = user_specified_memory;
    int memory_occupying_entities = my_executors.size()
        + 1; // one extra for dcmpiruntime
    if (handles_console) {
        int local_consoles = layout->get_filter_count_for_dcmpi_rank(-1);
        if (local_consoles) {
            // one for local console filter, one for local console incoming
            // area
            memory_occupying_entities += 2;
        }
    }
    amount_per_memusage /= memory_occupying_entities;
    // rutt: every filter should be able to write-buffer at least 32 MB
    amount_per_memusage = max(amount_per_memusage, (int8)MB_32);
    std::map<int, DCFilterExecutor*>::iterator it55;
    for (it55 = my_executors.begin(); it55 != my_executors.end(); it55++) {
        // allocate MemUsage structures for every local filter
        MUID_MemUsage[it55->first] = new MemUsage(
            amount_per_memusage,
            it55->second->filter_instance->get_distinguished_name());
    }
    MUID_MemUsage[MUID_of_dcmpiruntime] = new MemUsage(amount_per_memusage,
                                                       "dcmpiruntime process");
    
    // link up out-queues for my filters' outbound ports
    std::map<int, DCFilterExecutor*>::iterator it;
    for (it = my_executors.begin(); it != my_executors.end(); it++) {
        DCFilterExecutor * exe = it->second;
        DCFilterInstance * fi = exe->filter_instance;
        std::map<std::string, ResolvedMultiPort>::iterator it2;
        for (it2 = fi->resolved_outports.begin();
             it2 != fi->resolved_outports.end();
             it2++) {
            const std::string & multiportKey = it2->first;
            ResolvedMultiPort & rmp = it2->second;
            for (u2 = 0; u2 < rmp.remotes.size(); u2++) {
                Gftid_Port & gp = rmp.remotes[u2];
#ifdef DEBUG
                assert(gftids_to_instances.count(gp.gftid) > 0);
#endif
                int remote_rank = gftids_to_instances[gp.gftid]->dcmpi_rank;
                int remote_cluster_rank =
                    gftids_to_instances[gp.gftid]->dcmpi_cluster_rank;
                dcmpi_send_ranks.insert(remote_rank);
                QueueWriter * qw = NULL;
                // make a local connection
                if (my_executors.count(gp.gftid) > 0) {
                    qw = new LocalQueueWriter(
                        my_executors[gp.gftid]->in_ports[gp.port],
                        fi->gftid,
                        gftids_to_instances[gp.gftid]);
                    
                }
                // feed local console via a RemoteQueueWriter
                else if ((remote_rank == -1) && handles_console) {
                    qw = new RemoteQueueWriter(
                        remote_rank, remote_cluster_rank,
                        gp.gftid, gp.port,
                        fi->dcmpi_rank, fi->dcmpi_cluster_rank,
                        fi->gftid, multiportKey,
                        mpi_to_console_queue, gftids_to_instances[gp.gftid]);
                }
                // make a remote connection via proxy
                else if (rank == 0 && cluster_rank != remote_cluster_rank) {
                    qw = new RemoteQueueWriter(
                        remote_rank, remote_cluster_rank,
                        gp.gftid, gp.port,
                        fi->dcmpi_rank, fi->dcmpi_cluster_rank,
                        fi->gftid, multiportKey,
                        mpi_to_proxy_queue, gftids_to_instances[gp.gftid]);
                }
                // make a remote connection via MPI
                else {
                    qw = new RemoteQueueWriter(
                        remote_rank, remote_cluster_rank,
                        gp.gftid, gp.port,
                        fi->dcmpi_rank, fi->dcmpi_cluster_rank,
                        fi->gftid, multiportKey,
                        &single_mpi_outqueue, gftids_to_instances[gp.gftid]);
                }
                if (exe->out_ports.count(multiportKey) == 0) {
                    exe->out_ports[multiportKey] =
                        new MultiOutportManager(multi_output_policy, fi);
                }
                exe->out_ports[multiportKey]->add_target(qw);
            }
            exe->out_ports[multiportKey]->note_final_target();
        }
    }
    dcmpi_send_ranks.erase(rank);

    int num_all_filters_done_expected = totaldcmpiranks - 1;
    int num_all_filters_done_received = 0;

//     cerr << id() << ": num_all_filters_done_expected "
//          << num_all_filters_done_expected << endl << flush;

    // start console threads
    if (handles_console) {
        mpi_from_console_thread->start();
        mpi_to_console_thread->start();
    }

    // start "to proxy" thread
    if (use_proxy && rank == 0) {
        mpi_to_proxy_thread->start();
    }

    // so users can set breakpoint here, to set breakpoints in their filters
    dcmpi_filters_loaded(my_executors.size());
    
    // start filter threads
    for (u = 0; u < my_threads.size(); u++) {
        my_threads[u]->start();
    }

    std::vector<DCMPIPacket*> packets;
    bool any_of_my_filters_running = true;
    bool sent_all_filters_done_message = false;
    
    all_filter_done_messages_routed = 0;
    all_filter_done_messages_to_route = 0;
    if (rank == 0) {
        int other_dcmpi_ranks_within_cluster = size - 1;
        if (cluster_rank == 0) {
            if (console_filter_present) {
                // handle routing among cluster 0
                all_filter_done_messages_to_route = 2 * (size - 1);
                other_dcmpi_ranks_within_cluster += 1;
            }
        }
        all_filter_done_messages_to_route +=
            other_dcmpi_ranks_within_cluster *
            (totaldcmpiranks - (other_dcmpi_ranks_within_cluster+1)) * 2;

    }
//     cout << "all_filter_done_messages_to_route: "
//          << all_filter_done_messages_to_route << endl;


    outq_handler outq_handler_thread( &single_mpi_outqueue);
    outq_handler_thread.start();

    std::vector<DCMPIPacket*> heads_without_bodies;
    for (i = 0; i < size; i++) {
        heads_without_bodies.push_back(NULL);
    }

    double sleeptime_init = 0.0001;
    double sleeptime = sleeptime_init;
    double sleeptime_max = 0.1;

    bool progressed;
    int iters = 0, consecutive_useless_iters = 0;
    char hdr[DCMPI_PACKET_HEADER_SIZE];
    while (1) {
        progressed = false;
        if (!sent_all_filters_done_message) {
            unfinished_local_gftids_mutex.acquire();
            any_of_my_filters_running = !unfinished_local_gftids.empty();
            unfinished_local_gftids_mutex.release();
            if ((any_of_my_filters_running == false) &&
                single_mpi_outqueue.empty() &&
                (!mpi_to_proxy_queue || mpi_to_proxy_queue->empty())) {
                sent_all_filters_done_message = true;
                int rnk = 0;
                if (console_filter_present) {
                    rnk = -1;
                }
                for (i = 0; i < totaldcmpiranks; i++) {
                    if (rnk != dcmpi_rank) {
                        dcmpisock_deliver_elsewhere(
                            new DCMPIPacket(
                                DCMPIPacketAddress(
                                    rnk, dcmpirank_clusterrank[rnk], -1, "",
                                    dcmpi_rank, cluster_rank, -1, ""),
                                DCMPI_PACKET_TYPE_ALL_MY_FILTERS_DONE, 0));
//                         cout << id();
//                         cout << ":sending ALL_FILTERS_DONE to ";
//                         cout << "dcmpi_rank " << rnk << endl;
                    }
                    rnk += 1;
                }
                progressed = true;
            }
        }

#ifdef DEBUG
        assert(num_all_filters_done_received <= num_all_filters_done_expected);
#endif
        
        /* test conditions for breaking of the main loop */

        // we've sent our "done" message
        if (sent_all_filters_done_message) {
            
            // we'll receive nothing more from others
            if (num_all_filters_done_received ==
                num_all_filters_done_expected) {

                // we have no filters running
                if (!any_of_my_filters_running) {

                    // nothing pending to go out via MPI
                    if (single_mpi_outqueue.empty()) { 

                        // we've routed all we need to
                        if (all_filter_done_messages_routed ==
                            all_filter_done_messages_to_route) {

                            if (!handles_console ||
                                (!mpi_from_console_queue->
                                 has_more_putters() &&
                                 mpi_from_console_queue->empty())) {
                                single_mpi_outqueue.put(NULL);
                                outq_handler_thread.join();
                                break;
                            }
                        }
                    }
                }
            }
        }

        for (i = 0; i < size; i++) {
            if (i != rank && rank_fds[i] != -1) {
                if (dcmpi_socket_data_ready_for_reading(rank_fds[i])) {
                    DCMPIPacket * p;
                    if (heads_without_bodies[i] == NULL) {
                        if (DC_read_all(rank_fds[i], hdr,
                                        DCMPI_PACKET_HEADER_SIZE)) {
                            std::cerr << "ERRORE reading from other rank " << dcmpi_get_hostname()
                                      << " at " << __FILE__ << ":" << __LINE__
                                      << std::endl << std::flush;
                            exit(1);
                        }
                        p = new DCMPIPacket;
                        p->init_from_bytearray(hdr);
                        heads_without_bodies[i] = p;
                        progressed = true;
                    }
                    else {
                        p = heads_without_bodies[i];
                    }

                    if (p->packet_type == DCMPI_PACKET_TYPE_DCBUF) {
                        if (!MUID_MemUsage[MUID_of_dcmpiruntime]->
                            accumulate_if_not_exceeding_limit(
                                p->body_len)) {
                            continue;
                        }
                        DCBuffer * buf = new DCBuffer(p->body_len);
                        char * offset = buf->getPtr();
                        int rc = DC_read_all(rank_fds[i], offset, p->body_len);
                        if (rc) {
                            fprintf(stderr, "ERROR: on host %s reading from rank %d at %s:%d\n",
                                    dcmpi_get_hostname().c_str(), i, __FILE__, __LINE__);
                            exit(1);
                        }
                        buf->setUsedSize(p->body_len);
                        buf->forceEndian(hdr[0]?1:0);
                        p->body = buf;
                        progressed = true;
                    }
                    if (p->address.to_rank == dcmpi_rank) {
                        dcmpisock_deliver_in(p, my_executors,
                                             num_all_filters_done_received);
                    }
                    else {
                        dcmpisock_deliver_elsewhere(p);
                    }
                    heads_without_bodies[i] = NULL;
                }
            }
        }
        
        // handle incoming queue from console
        if (handles_console &&
            (mpi_from_console_queue->getall_if_item_present(packets))) {
            // deliver traffic in from console
            u = 0;
            while (u < packets.size()) {
                DCMPIPacket * p = packets[u];
                if (p->address.to_rank == dcmpi_rank) {
                    dcmpisock_deliver_in(p, my_executors,
                                         num_all_filters_done_received);
                }
                else {
                    dcmpisock_deliver_elsewhere(p);
                }
                progressed = true;
                u++;
            }
        }

        // handle incoming queue from proxy
        if (rank == 0 && proxy_in_socket != -1) {
            bool deliver_packet = false;
            if (proxy_in_state == PROXY_IN_NEED_HEAD &&
                dcmpi_socket_data_ready_for_reading(proxy_in_socket)) {
                std::string line = dcmpi_socket_read_line(proxy_in_socket);
                if (line == "bye") {
                    dcmpiCloseSocket(proxy_in_socket);
                    proxy_in_socket = -1;
                }
                else {
                    char hdr[DCMPI_PACKET_HEADER_SIZE];
                    checkrc(DC_read_all(proxy_in_socket,
                                        hdr,
                                        DCMPI_PACKET_HEADER_SIZE));
                    int length = Atoi(line);
                    proxy_in_body_length = length - DCMPI_PACKET_HEADER_SIZE;
                    proxy_in_packet = new DCMPIPacket;
                    proxy_in_packet->init_from_bytearray(hdr);
                    if (proxy_in_packet->packet_type ==
                        DCMPI_PACKET_TYPE_DCBUF) {
                        proxy_in_state = PROXY_IN_NEED_BODY;
                    }
                    else {
                        deliver_packet = true;
                    }
                }
                progressed = true;
            }
            if (proxy_in_state == PROXY_IN_NEED_BODY) {
                if (MUID_MemUsage[MUID_of_dcmpiruntime]->
                    accumulate_if_not_exceeding_limit(proxy_in_body_length)) {
                    DCBuffer * b = new DCBuffer(proxy_in_body_length);
                    if (proxy_in_body_length) {
                        checkrc(DC_read_all(proxy_in_socket,
                                            b->getPtr(), proxy_in_body_length));
                        b->setUsedSize(proxy_in_body_length);
                    }
                    proxy_in_packet->body = b;
                    proxy_in_state = PROXY_IN_NEED_HEAD;
                    deliver_packet = true;
                }
                else {
                    ;
                }
            }
            
            if (deliver_packet) {
                if (proxy_in_packet->address.to_rank == dcmpi_rank) {
                    dcmpisock_deliver_in(proxy_in_packet, my_executors,
                               num_all_filters_done_received);
                }
                else {
                    dcmpisock_deliver_elsewhere(proxy_in_packet);
                }
                progressed = true;
                assert(proxy_in_state == PROXY_IN_NEED_HEAD);
            }
        }

        if (!progressed) {
            consecutive_useless_iters++;
#ifndef DCMPI_NO_YIELD
            sched_yield();
            if (consecutive_useless_iters > 2000) {
#endif
                dcmpi_doublesleep(sleeptime);
                sleeptime *= 2;
                sleeptime = std::min(sleeptime, sleeptime_max);
#ifndef DCMPI_NO_YIELD
            }
#endif
        }
        else {
            consecutive_useless_iters = 0;
            sleeptime = sleeptime_init;
        }

        iters++;
    }

    // join/cleanup filter threads
    for (u = 0; u < my_threads.size(); u++) {
        my_threads[u]->join();
        delete my_threads[u];
    }

    socket_barrier();
    
    // cleanup executors
    bool print_stats = (getenv("DCMPI_STATS") != NULL);
    if (print_stats) {
        // aggregate on rank 0
        std::string s = dcmpiruntime_common_statsprep();
        const char * msg = s.c_str();
        char msg_len[256];
        if (rank > 0) {
            snprintf(msg_len, sizeof(msg_len), "%d", (int)(strlen(msg)));
            checkrc(DC_write_all(rank_fds[0], msg_len, sizeof(msg_len)));
            checkrc(DC_write_all(rank_fds[0], (void*)msg, strlen(msg)+1));
        }
        else {
            std::vector<char*> incoming_messages;
            char * incoming;
            int i;
            for (i = 1; i < size; i++) {
                checkrc(DC_read_all(rank_fds[i], msg_len, sizeof(msg_len)));
                int len = atoi(msg_len);
                incoming = new char[len+1];
                checkrc(DC_read_all(rank_fds[i], incoming, len+1));
                incoming_messages.push_back(incoming);
            }
            cout << s;
            for (uint u = 0; u < incoming_messages.size(); u++) {
                cout << endl << incoming_messages[u];
                delete[] incoming_messages[u];
            }
            cout << flush;
        }
    }


    for (it = my_executors.begin(); it != my_executors.end(); it++) {
        DCFilterExecutor * exe = it->second;
        DCFilter * runnable = exe->filter_runnable;
        delete runnable;
        delete exe;
    }

    if (dcmpi_verbose())
        dbg("dcmpi_mainloop: exiting, iters was %d\n", iters);

    return 0;
}

#define SOCKET_BUF_SIZE MB_4
void dcmpisock_startup(int & argc, char ** argv)
{
    if (argc-1 < 5) {
        usage();
    }
    int i, i2;
    uint u;
    std::string wrapper;
    std::vector<std::string> wrapperhosts;
    std::string master_host;
    std::string hostfile;
    std::string rsh;
    int master_port = -1;
    rsh = "ssh -n -x";
    bool run_rank0_on_localhost = false;
    while (argc-1 >= 2) {
        if (strcmp(argv[1],"-rank")==0) {
            rank = atoi(argv[2]);
            dcmpi_args_shift(argc, argv);
            dcmpi_args_shift(argc, argv);
        }
        else if (strcmp(argv[1],"-forcerank0local")==0) {
            run_rank0_on_localhost = true;
            dcmpi_args_shift(argc, argv);
        }
        else if (strcmp(argv[1],"-hostfile")==0) {
            hostfile = argv[2];
            dcmpi_args_shift(argc, argv);
            dcmpi_args_shift(argc, argv);
            rank_hosts = dcmpi_file_lines_to_vector(hostfile);
            assert(rank_hosts.size());
            if (resembles_localhost(rank_hosts[0])) {
                run_rank0_on_localhost = true;
            }
        }
        else if (strcmp(argv[1],"-h")==0) {
            rank_hosts.push_back(argv[2]);
            dcmpi_args_shift(argc, argv);
            dcmpi_args_shift(argc, argv);
        }
        else if (strcmp(argv[1], "-size")==0) {
            size = atoi(argv[2]);
            dcmpi_args_shift(argc, argv);
            dcmpi_args_shift(argc, argv);
        }
        else if (strcmp(argv[1], "-rsh")==0) {
            rsh = shell_unquote(argv[2]);
            dcmpi_args_shift(argc, argv);
            dcmpi_args_shift(argc, argv);
        }
        else if (strcmp(argv[1], "-master_host")==0) {
            master_host = argv[2];
            dcmpi_args_shift(argc, argv);
            dcmpi_args_shift(argc, argv);
        }
        else if (strcmp(argv[1], "-master_port")==0) {
            master_port = atoi(argv[2]);
            dcmpi_args_shift(argc, argv);
            dcmpi_args_shift(argc, argv);
        }
        else if (strcmp(argv[1], "-wrapper")==0) {
            wrapper = shell_unquote(argv[2]);
            dcmpi_args_shift(argc, argv);
            dcmpi_args_shift(argc, argv);
        }
        else if (strcmp(argv[1], "-wrapperhosts")==0) {
            wrapperhosts = dcmpi_string_tokenize(shell_unquote(argv[2]),",");
            dcmpi_args_shift(argc, argv);
            dcmpi_args_shift(argc, argv);
        }
        else if (strcmp(argv[1], "-exe") == 0) {
            dcmpi_args_shift(argc, argv);
            dcmpi_args_shift(argc, argv);
        }
        else {
            break;
        }
    }
    assert(!rsh.empty());

    uint2 listen_port;
    dcmpisock_listensocket = dcmpiOpenListenSocket(&listen_port,
                                                   SOCKET_BUF_SIZE);
    assert(dcmpisock_listensocket!=-1);
    
    rank_ports = new int[size];
    rank_fds = new int[size];
    if (rank == 0) {
        rank0_barrier_fds = new int[size];
    }
    for (i = 0; i < size; i++) {
        rank_ports[i] = -1;
        rank_fds[i] = -1;
        if (rank == 0) {
            rank0_barrier_fds[i] = -1;
        }
    }
    
    if (rank == 0 && !run_rank0_on_localhost) {
        assert(0); // shouldn't happen (rank 0 always on localhost)
    }
    else if (run_rank0_on_localhost && size > 1) {
        assert(rank == 0);
        master_port = listen_port;
        // launch remote processes
        std::string common;
        for (u = 0; u < rank_hosts.size(); u++) {
            common += " -h " + rank_hosts[u];
        }
        for (i = 1; i < argc; i++) {
            common += " ";
            common += argv[i];
        }
        for (u = 1; u < rank_hosts.size(); u++) {
            std::string command = rsh;
            command += " " + rank_hosts[u];
            if (!wrapper.empty() &&
                (wrapperhosts.empty() ||
                 std::find(wrapperhosts.begin(),
                           wrapperhosts.end(),
                           rank_hosts[u]) != wrapperhosts.end())) {
                command += " " + wrapper;
            }
            command += " "; command += argv[0];
            command += " -rank " + tostr(u);
            command += " -size " + tostr(size);
//            command += " -master_host " + dcmpi_get_hostname();
            command += " -master_host ";
            std::vector<std::string> ips = dcmpi_get_ip_addrs();
            for (uint i = 0; i < ips.size(); i++) {
                if (i) {
                    command += ',';
                }
                command += ips[i];
            }
            command += " -master_port " + tostr(master_port);
            command += common;
            if (dcmpi_verbose()) {
                std::cout << "running remote command " << command << endl;
            }
            DCCommandThread * t = new DCCommandThread(command, false);
            remote_processes.push_back(t);
            t->start();
            if (dcmpi_string_starts_with(rsh, "ssh") &&
                rsh.find(" -x") == std::string::npos) {
                sleep(1);
            }
        }
    }

    // make connection
    if (rank == 0) {
        struct sockaddr_in clientAddr;
        socklen_t clientAddrLen = sizeof(clientAddr);
        for (i = 0; i < (size-1)*2; i++) {
            int sd = accept(dcmpisock_listensocket,
                            (struct sockaddr*)&clientAddr,
                            (socklen_t*)&clientAddrLen);
            if (sd==-1) {
                std::cerr << "ERROR: accept()"
                          << " at " << __FILE__ << ":" << __LINE__
                          << std::endl << std::flush;
                exit(1);
            }
            dcmpiSetSocketBufferSize(sd, SOCKET_BUF_SIZE);
            std::string hn = dcmpi_socket_read_line(sd);
            std::string rnk = dcmpi_socket_read_line(sd);
            int rnk2 = Atoi(rnk);
            if (rank_fds[rnk2] == -1) {
                std::string prt = dcmpi_socket_read_line(sd);
                int prt2 = Atoi(prt);
                rank_fds[rnk2] = sd;
                rank_ports[rnk2] = prt2;
            }
            else {
                rank0_barrier_fds[rnk2] = sd;
            }
        }
        // disseminate port info
        for (i = 1; i < size; i++) {
            for (i2 = 1; i2 < size; i2++) {
                if (i != i2) {
                    dcmpi_socket_write_line(rank_fds[i2], rank_hosts[i]);
                    dcmpi_socket_write_line(rank_fds[i2], tostr(i));
                    dcmpi_socket_write_line(rank_fds[i2], tostr(rank_ports[i]));
                }
            }
        }
    }
    else {
        rank_ports[0] = master_port;
        std::vector<std::string> candidates = dcmpi_string_tokenize(master_host, ",");
        for (uint c = 0; c < candidates.size(); c++) {
            
            rank_fds[0] = dcmpiOpenClientSocket(candidates[c].c_str(),
                                                master_port, SOCKET_BUF_SIZE);
            if (rank_fds[0] != -1) {
                break;
            }
        }
        if (rank_fds[0] == -1) {
            std::cerr << "ERROR: connecting to master_host "
                      << master_host
                      << " at " << __FILE__ << ":" << __LINE__
                      << std::endl << std::flush;
            exit(1);
        }
        dcmpi_socket_write_line(rank_fds[0], dcmpi_get_hostname());
        dcmpi_socket_write_line(rank_fds[0], tostr(rank));
        dcmpi_socket_write_line(rank_fds[0], tostr(listen_port));

        for (uint c = 0; c < candidates.size(); c++) {
            rank_non0_barrier_fd = dcmpiOpenClientSocket(candidates[c].c_str(),
                                                         master_port);
            if (rank_non0_barrier_fd != -1) {
                break;
            }
        }
        if (rank_non0_barrier_fd == -1) {
            std::cerr << "ERROR: opening barrier port to master"
                      << " at " << __FILE__ << ":" << __LINE__
                      << std::endl << std::flush;
            exit(1);
        }
        dcmpi_socket_write_line(rank_non0_barrier_fd, dcmpi_get_hostname());
        dcmpi_socket_write_line(rank_non0_barrier_fd, tostr(rank));

        for (i = 1; i < size-1; i++) {
            std::string hn = dcmpi_socket_read_line(rank_fds[0]);
            std::string rnk = dcmpi_socket_read_line(rank_fds[0]);
            std::string prt = dcmpi_socket_read_line(rank_fds[0]);
            int rnk2 = Atoi(rnk);
            int prt2 = Atoi(prt);
            assert(rank_hosts[rnk2] == hn);
            rank_ports[rnk2] = prt2;
        }

        // do accepts
        for (i = rank; i >= 2; i--) {
            struct sockaddr_in clientAddr;
            socklen_t clientAddrLen = sizeof(clientAddr);
            int sd = accept(dcmpisock_listensocket,
                            (struct sockaddr*)&clientAddr,
                            (socklen_t*)&clientAddrLen);
            std::string hn = dcmpi_socket_read_line(sd);
            std::string rnk = dcmpi_socket_read_line(sd);
            rank_fds[Atoi(rnk)] = sd;
        }
        // do outgoing conns
        for (i = rank; i < size-1; i++) {
if (dcmpi_string_ends_with(rank_hosts[i+1], "gen2")) {
dcmpi_string_replace(rank_hosts[i+1], "-gen2", "");
}
            int sd = dcmpiOpenClientSocket(rank_hosts[i+1].c_str(),
                                           rank_ports[i+1], SOCKET_BUF_SIZE);
            if (sd == -1) {
                std::cerr << "ERRORE: " << rank_hosts[i+1]
                          << " at " << __FILE__ << ":" << __LINE__
                          << std::endl << std::flush;
            }
            dcmpi_socket_write_line(sd, dcmpi_get_hostname());
            dcmpi_socket_write_line(sd, tostr(rank));
            rank_fds[i+1] = sd;
        }
    }

    for (i = 0; i < rank; i++) {
        if (i != rank) {
            int s = rank_fds[i];
            checkrc(dcmpiSetSocketBufferSize(s, SOCKET_BUF_SIZE));
        }
    }

//     for (i = 0; i < size; i++) {
//         int sndsize, recvsize;
//         if (i != rank) {
//             int sd = rank_fds[i];
//             assert(sd != -1);
//             checkrc(dcmpiGetSocketBufferSize(sd, sndsize, recvsize));
//             std::cout << "send " << rank << " to " << i << ": "
//                       << sndsize << endl;
//             std::cout << "recv " << rank << " from " << i << ": "
//                       << recvsize << endl;
//         }
//     }
}

static void sig_pipe(int arg, siginfo_t *psi, void *p)
{
    printf("sig_pipe on %s noticed...some remote closed the connection\n",
           dcmpi_get_hostname().c_str());
    exit(1);
}

static void sig_pipe_install()
{
    struct sigaction s;
    s.sa_sigaction = sig_pipe;
    sigemptyset(&s.sa_mask);
    s.sa_flags = SA_SIGINFO;
    if (sigaction(SIGPIPE, &s, NULL) != 0)
        perror("Cannot install SIGPIPE handler");
}

int main(int argc, char * argv[])
{
    int outrc, i;
    appname = argv[0];

    if (argc == 1) {
        usage();
    }
    sig_pipe_install();
    dcmpisock_startup(argc, argv);
    
    dcmpiruntime_common_startup(argc, argv);

    if (rank == 0) {
        parse_args(argc, argv);
    }

    outrc = dcmpiruntime_common_execute(dcmpi_socket_get_and_broadcast_layout,
                                        dcmpi_socket_mainloop);

    socket_barrier();

    dcmpiruntime_common_cleanup();
    if (dcmpisock_listensocket != -1) {
        close(dcmpisock_listensocket);
    }
    for (uint u = 0; u < remote_processes.size(); u++) {
        remote_processes[u]->join();
        delete remote_processes[u];
    }
    remote_processes.clear();

    for (i = 0; i < size; i++) {
        if (rank_fds[i] != -1) {
            close(rank_fds[i]);
        }
        if (rank == 0 && rank0_barrier_fds[i] != -1) {
            close(rank0_barrier_fds[i]);
        }
    }
    if (rank > 0) {
        close(rank_non0_barrier_fd);
    }

//     std::cout << "dcmpiruntime-socket exiting on " << dcmpi_get_hostname() << endl;

    return outrc;
}
