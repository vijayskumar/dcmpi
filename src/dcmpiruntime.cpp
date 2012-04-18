/***************************************************************************
    $Id: dcmpiruntime.cpp,v 1.110 2008/02/07 05:49:29 krishnan Exp $
    Copyright (C) 2004 by BMI Department, The Ohio State University.
    For license info, see LICENSE in root directory of source distribution.
 ***************************************************************************/

#include <mpi.h>

#include "dcmpi_clib.h"

#ifndef DCMPI_NO_YIELD
#include <sched.h>
#endif

#include <sys/time.h>
#include <sys/resource.h>
#include <sys/poll.h>

#include "dcmpi.h"
#include "dcmpiruntime-util.h"

using namespace std;

MPI_Status status;
#define DCMPI_MPI_TAG 177
#define DCMPI_STATS_TAG 178

char * appname = NULL;
void usage()
{
    printf("usage: %s ...\n       dcmpi_control_args_begin [args...] "
           "dcmpi_control_args_end ...\n", appname);
    printf("NOTE: this executable is for dcmpi internal use only.\n");
    exit(EXIT_FAILURE);
}

int dcmpi_mpi_get_and_broadcast_layout(char hdr[DCMPI_PACKET_HEADER_SIZE])
{
    DCMPIPacket layout_head;

    if (rank == 0) {
        checkrc(MPI_Bcast(hdr, DCMPI_PACKET_HEADER_SIZE,
                          MPI_CHAR, 0, MPI_COMM_WORLD));
        layout_head.init_from_bytearray(hdr);
        DCBuffer layout_buf(layout_head.body_len);
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
        layout_buf.setUsedSize(layout_head.body_len);
        checkrc(MPI_Bcast(layout_buf.getPtr(), layout_head.body_len, MPI_CHAR,
                          0, MPI_COMM_WORLD));
        layout->deSerialize(&layout_buf);
    }
    else {
        checkrc(MPI_Bcast(hdr, DCMPI_PACKET_HEADER_SIZE,
                          MPI_CHAR, 0, MPI_COMM_WORLD));
        DCMPIPacket layout_head;
        layout_head.init_from_bytearray(hdr);
        DCBuffer buf(layout_head.body_len);
        checkrc(MPI_Bcast(buf.getPtr(), layout_head.body_len, MPI_CHAR,
                          0, MPI_COMM_WORLD));
        buf.setUsedSize(layout_head.body_len);
        layout->deSerialize(&buf);
    }
    dcmpi_remote_console_snarfed_layout = true;
    
#ifdef DEBUG3
    cout << "** in rank " << rank << " LAYOUT IS **\n" << *layout << endl;
#endif

    // send/receive cluster rank, size, etc.
    checkrc(MPI_Bcast(&cluster_rank, 1, MPI_INT, 0, MPI_COMM_WORLD));
    checkrc(MPI_Bcast(&cluster_size, 1, MPI_INT, 0, MPI_COMM_WORLD));
    checkrc(MPI_Bcast(&cluster_offset, 1, MPI_INT, 0, MPI_COMM_WORLD));
    checkrc(MPI_Bcast(&totaldcmpiranks, 1, MPI_INT, 0, MPI_COMM_WORLD));
    int cfp = -1;
    if (rank == 0) {
        cfp = (console_filter_present)?1:0;
    }
    checkrc(MPI_Bcast(&cfp, 1, MPI_INT, 0, MPI_COMM_WORLD));
    if (rank) {
        console_filter_present = (cfp == 1);
    }
    if (rank == 0) {
        int r;
        for (int i = 1; i < size; i++) {
            r = dcmpi_rank + i; 
            checkrc(MPI_Send(&r, 1, MPI_INT, i, DCMPI_MPI_TAG, MPI_COMM_WORLD));
        }
    }
    else {
        checkrc(MPI_Recv(&dcmpi_rank, 1, MPI_INT, 0,
                         DCMPI_MPI_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE));
    }
    return 0;
}

#define MPI_RECV_INFO_REQ_INACTIVE                 (uint1)0
#define MPI_RECV_INFO_REQ_ACTIVE                   (uint1)1
#define MPI_RECV_INFO_REQ_COMPLETED_READY_FOR_BODY (uint1)2

class mpi_recv_info
{
public:
    mpi_recv_info() : state(MPI_RECV_INFO_REQ_INACTIVE),
                      bodyless_packet(NULL)
    {
        hdr = new char[DCMPI_PACKET_HEADER_SIZE];
    }
    ~mpi_recv_info() { delete[] hdr; }
    char * hdr;
    MPI_Request req;
    uint1 state;
    DCMPIPacket * bodyless_packet;
    friend std::ostream& operator<<(std::ostream &o, const mpi_recv_info& i);
private:
    mpi_recv_info(const mpi_recv_info & t);
    mpi_recv_info & operator=(const mpi_recv_info & t);
};

std::ostream& operator<<(std::ostream &o, const mpi_recv_info& i)
{
    return o << "state=" << (int)i.state
             << ", bodyless_packet=" << i.bodyless_packet;
}

void deliver_in(DCMPIPacket * p,
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

void deliver_elsewhere(DCMPIPacket * p,
                 std::vector<MPI_Request*> & outreq,
                 std::vector<DCMPIPacket*> & outreq_pending_state)
{

    if (p->packet_type == DCMPI_PACKET_TYPE_ALL_MY_FILTERS_DONE &&
        p->address.from_rank != dcmpi_rank) {
        all_filter_done_messages_routed += 1;
#ifdef DEBUG
//         cout << "all_filter_done_messages_routed now "
//              << all_filter_done_messages_routed << endl;
        assert(all_filter_done_messages_routed <=
               all_filter_done_messages_to_route);
#endif
    }
    if ((p->address.to_rank == -1) && handles_console) {
        mpi_to_console_queue->put(p);
    }
    else if (rank == 0 && use_proxy &&
             p->address.to_cluster_rank != cluster_rank) {
//         std::cout << "rank 0 of cluster_rank " << cluster_rank
//                   << ": putting packet " << *p
//                   << " on mpi_to_proxy_queue\n" << flush;
        mpi_to_proxy_queue->put(p);
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
        MPI_Request * reqhdr = new MPI_Request;
        p->to_bytearray();
        checkrc(MPI_Isend(p->internal_hdr,
                          DCMPI_PACKET_HEADER_SIZE, MPI_CHAR, outrank,
                          DCMPI_MPI_TAG, MPI_COMM_WORLD, reqhdr));
        if (p->body == NULL) {
            outreq.push_back(reqhdr);
            outreq_pending_state.push_back(p);
            p->addref();
#ifdef DEBUG4
            cout << "outreq:  added reqhdr(only) " << reqhdr << ", now has:";
            std::copy(outreq.begin(), outreq.end(), ostream_iterator<MPI_Request*>(cout, " "));
            cout << endl;
#endif
        }
        else {
            outreq.push_back(reqhdr);
            outreq_pending_state.push_back(p);
            p->addref();
#ifdef DEBUG4
            cout << "outreq:  added reqhdr " << reqhdr << ", now has:";
            std::copy(outreq.begin(), outreq.end(), ostream_iterator<MPI_Request*>(cout, " "));
            cout << endl;
#endif

#define DCMPI_DEBUG_MPISEND_FUNC(buffer, size, rnk) { cout << "DEBUG_MPISEND on " << dcmpi_get_hostname(true) << ": sending out buffer of size " << size << " to rank " << rnk << " with contents: "; for (int i = 0; i < (size); i++) { printf(" %x", (unsigned int)((unsigned char)(buffer[i]))); if (i == 64) { printf("..."); break; } } cout << endl; }

            char * offset = p->body->getPtr();
            MPI_Request * reqbody = new MPI_Request;

#ifdef DCMPI_DEBUG_MPISEND
            DCMPI_DEBUG_MPISEND_FUNC(offset, p->body_len, outrank);
#endif
            checkrc(MPI_Isend(offset, p->body_len, MPI_CHAR, outrank,
                              DCMPI_MPI_TAG, MPI_COMM_WORLD, reqbody));
            outreq.push_back(reqbody);
            outreq_pending_state.push_back(p);
            p->addref();
#ifdef DEBUG4
            cout << "outreq:  added reqbody " << reqbody << ", now has:";
            std::copy(outreq.begin(), outreq.end(), ostream_iterator<MPI_Request*>(cout, " "));
            cout << endl;
#endif
        }
    }
}

int dcmpi_mpi_mainloop()
{
    int i;
    uint u, u2;
    Queue<DCMPIPacket*> single_mpi_outqueue;
    std::vector<DCThread*> my_threads;
    std::map<int, DCFilterInstance*> gftids_to_instances;
    std::set<int> dcmpi_send_ranks;
    std::map<int, int> dcmpirank_clusterrank;
    
    std::vector<mpi_recv_info*> mpi_recv_ranks;
    std::vector<MPI_Request*> outreq;
    // stores packets that should be deleted when the send is complete.  for a
    // send of a body, the header state will be NULL;
    std::vector<DCMPIPacket*> outreq_pending_state;
    int req_done; // reusable 'done' variable

    // manage sleep times in the main loop below
    double sleeptime_init;
    double sleeptime_max;
    if (getenv("DCMPI_MAINLOOP_SLEEPTIMES")) {
        std::string s = getenv("DCMPI_MAINLOOP_SLEEPTIMES");
        std::vector<std::string> toks = dcmpi_string_tokenize(s);
        sleeptime_init = atof(toks[0].c_str());
        sleeptime_max = atof(toks[1].c_str());
    }
    else {
        sleeptime_init = 0.0001;
        sleeptime_max = 0.001;
    }
    double sleeptime = sleeptime_init;
    int consecutive_useless_iters = 0;
    bool progressed;
    int iters = 0;
    int multi_output_policy = DCMPI_MULTIOUT_POLICY_ROUND_ROBIN;
    std::string write_policy_file = dcmpi_get_home() + "/portconfig";
    if (fileExists(write_policy_file)) {
        std::map<std::string, std::string> pairs =
            file_to_pairs(write_policy_file);
        if (pairs.count("write_policy")) {
            if (pairs["write_policy"] == "RR") {
                multi_output_policy = DCMPI_MULTIOUT_POLICY_ROUND_ROBIN;
                if (dcmpi_verbose()) {
                    std::cerr << "write_policy set to RR\n";
                }
            }
            else if (pairs["write_policy"] == "random") {
                multi_output_policy = DCMPI_MULTIOUT_POLICY_RANDOM;
                if (dcmpi_verbose()) {
                    std::cerr << "write_policy set to random\n";
                }
                srand(time(NULL));
            }
            else {
                std::cerr << "ERROR: invalid write_policy ("
                          << pairs["write_policy"] << "), check port config\n"
                          << std::flush;
                exit(1);
            }
        }
    }


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

//     // see who writes to us
//     for (u = 0; u < layout->filter_instances.size(); u++) {
//         DCFilterInstance * fi = layout->filter_instances[u];
//         int its_rank = fi->dcmpi_rank;
//         int its_cluster_rank = fi->dcmpi_cluster_rank;
//         if (its_rank != rank) {
//             // OK, it's a remote instance...does it write to one of ours?
//             std::map<std::string, ResolvedMultiPort>::iterator it2;
//             for (it2 = fi->resolved_outports.begin();
//                  it2 != fi->resolved_outports.end();
//                  it2++) {
//                 ResolvedMultiPort & rmp = it2->second;
//                 for (u2 = 0; u2 < rmp.remotes.size(); u2++) {
//                     Gftid_Port & gp = rmp.remotes[u2];
//                     int its_remote_rank =
//                         gftids_to_instances[gp.gftid]->dcmpi_rank;
//                     int its_remote_cluster_rank =
//                         gftids_to_instances[gp.gftid]->dcmpi_cluster_rank;
//                 }
//             }
//         }
//     }

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

    // setup mpi_recv_ranks
    for (i = 0; i < size; i++) {
        if (i != rank) {
            mpi_recv_ranks.push_back(new mpi_recv_info);
        }
        else {
            mpi_recv_ranks.push_back(NULL);
        }
    }

    // do MPI stuff here (don't need another thread)
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
    unfinished_local_gftids_mutex.acquire();
    int last_local_gftids_count = unfinished_local_gftids.size();
    unfinished_local_gftids_mutex.release();

    while (1) {
        progressed = false;
        if (!sent_all_filters_done_message) {
            unfinished_local_gftids_mutex.acquire();
	    /* added k2 to see which filters are yet to finish */
	    std::string output = "Filters left on " + dcmpi_get_hostname() + " : \n";
	    if (dcmpi_verbose()) {
	      int current_local_gftids_count = unfinished_local_gftids.size();
	      
	      if (current_local_gftids_count < last_local_gftids_count) {
		for (std::vector<int>::iterator it = unfinished_local_gftids.begin(); it != unfinished_local_gftids.end(); it++) {
		  DCFilterInstance *fi = gftids_to_instances[*it];
		  output = output + fi->get_instance_name() + "\n";
		}
		std::cout << output << std::endl;
		last_local_gftids_count = current_local_gftids_count;
	      }
	    }
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
                        deliver_elsewhere(
                            new DCMPIPacket(
                                DCMPIPacketAddress(
                                    rnk, dcmpirank_clusterrank[rnk], -1, "",
                                    dcmpi_rank, cluster_rank, -1, ""),
                                DCMPI_PACKET_TYPE_ALL_MY_FILTERS_DONE, 0),
                            outreq, outreq_pending_state);
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

                            // we have no pending requests
                            if (outreq.empty()) { 

                                if (!handles_console ||
                                    (!mpi_from_console_queue->
                                     has_more_putters() &&
                                     mpi_from_console_queue->empty())) {
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }

        // start speculative receives for packet headers from this cluster
        for (i = 0; i < size; i++) {
            if (i == rank) continue;
            mpi_recv_info * info = mpi_recv_ranks[i];
            if (info->state == MPI_RECV_INFO_REQ_INACTIVE) {
#ifdef DEBUG
                memset(info->hdr, 0, DCMPI_PACKET_HEADER_SIZE);
#endif
                checkrc(MPI_Irecv(info->hdr, DCMPI_PACKET_HEADER_SIZE,
                                  MPI_CHAR, i, DCMPI_MPI_TAG,
                                  MPI_COMM_WORLD, &info->req));
                info->state = MPI_RECV_INFO_REQ_ACTIVE;
                progressed = true;
            }
        }

        // start sends, if applicable
        if (single_mpi_outqueue.getall_if_item_present(packets)) {
            progressed = true;
            for (u = 0; u < packets.size(); u++) {
                deliver_elsewhere(packets[u], outreq, outreq_pending_state);
            }
        }

        // finish receives
        for (i = 0; i < size; i++) {
            if (i == rank) continue;
            mpi_recv_info * info = mpi_recv_ranks[i];
            if ((info->state == MPI_RECV_INFO_REQ_ACTIVE) ||
                (info->state == MPI_RECV_INFO_REQ_COMPLETED_READY_FOR_BODY)){
                bool get_body = true;
                if (info->state == MPI_RECV_INFO_REQ_ACTIVE) {
                    checkrc(MPI_Test(&info->req, &req_done, &status));
                    if (req_done) {
                        progressed = true; // got a head
                    }
                    else {
                        get_body = false;
                    }
                }
                if (get_body) {
                    DCMPIPacket * p;

                    if (info->state == MPI_RECV_INFO_REQ_ACTIVE) {
                        p = new DCMPIPacket;
                        p->init_from_bytearray(info->hdr);
                    }
                    else {
                        p = info->bodyless_packet;
                    }
                    DCBuffer * buf = NULL;
                    int sz;
                    sz = p->body_len;
                    if (p->packet_type == DCMPI_PACKET_TYPE_DCBUF) {
                        if (MUID_MemUsage[MUID_of_dcmpiruntime]->
                            accumulate_if_not_exceeding_limit(sz) == false) {
                            // defer completing this incoming req until later
                            info->state =
                                MPI_RECV_INFO_REQ_COMPLETED_READY_FOR_BODY;
                            info->bodyless_packet = p;
                            continue; // go no further here, wait for memory
                                      // usage to be reduced before continuing
                        }
                        buf = new DCBuffer(sz);
                        char * offset = buf->getPtr();
                        checkrc(MPI_Recv(
                                    offset, p->body_len,
                                    MPI_CHAR, i,
                                    DCMPI_MPI_TAG, MPI_COMM_WORLD,
                                    &status));
                        buf->setUsedSize(sz);
                        p->body = buf;
                    }
                    if (p->address.to_rank == dcmpi_rank) {
                        deliver_in(p, my_executors,
                                   num_all_filters_done_received);
                    }
                    else {
//                         std::cout << "pkt from here2 is " << *p << endl;
                        deliver_elsewhere(p, outreq, outreq_pending_state);
                    }
                    info->state = MPI_RECV_INFO_REQ_INACTIVE;
                    info->bodyless_packet = NULL;
                    progressed = true; // got a body
                }
            }
        }


        // finish any pending sends
        if (!outreq.empty()) {
            std::vector<MPI_Request*> new_outreq;
            std::vector<DCMPIPacket*> new_outreq_pending_state;
            for (u = 0; u < outreq.size(); u++) {
                MPI_Request * req = outreq[u];
                DCMPIPacket * p = outreq_pending_state[u];
#ifdef DEBUG4
                cout << "testing outreq " << req << endl;
#endif
                checkrc(MPI_Test(req, &req_done, &status));
                if (req_done) {
#ifdef DEBUG4
                    cout << "outreq " << req << " is done" << endl;
#endif
                    delete req;
                    if (p->body && p->getrefcount() == 1) {
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
//                     std::cout << "calling delref on p " << p
//                               << ", p->body " << p->body
//                               << " on host " << dcmpi_get_hostname()
//                               << endl;
                    p->delref();
                    progressed = true;
                }
                else {
                    new_outreq.push_back(req);
                    new_outreq_pending_state.push_back(p);
                }
            }
            outreq = new_outreq;
            outreq_pending_state = new_outreq_pending_state;
        }

        // handle incoming queue from console
        if (handles_console &&
            (mpi_from_console_queue->getall_if_item_present(packets))) {
            // deliver traffic in from console
            u = 0;
            while (u < packets.size()) {
                DCMPIPacket * p = packets[u];
                if (p->address.to_rank == dcmpi_rank) {
                    deliver_in(p, my_executors,
                               num_all_filters_done_received);
                }
                else {
                    deliver_elsewhere(p, outreq, outreq_pending_state);
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
                    deliver_in(proxy_in_packet, my_executors,
                               num_all_filters_done_received);
                }
                else {
                    deliver_elsewhere(proxy_in_packet,
                                      outreq, outreq_pending_state);
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

    // cleanup executors
    bool print_stats = (getenv("DCMPI_STATS") != NULL);
    if (print_stats) {
        // aggregate on rank 0
        std::string s = dcmpiruntime_common_statsprep();
        const char * msg = s.c_str();
        char msg_len[256];
        if (rank > 0) {
            snprintf(msg_len, sizeof(msg_len), "%d", (int)(strlen(msg)));
            checkrc(MPI_Send(msg_len, sizeof(msg_len), MPI_CHAR, 0,
                             DCMPI_STATS_TAG, MPI_COMM_WORLD));
            checkrc(MPI_Send((void*)msg, strlen(msg)+1, MPI_CHAR, 0,
                             DCMPI_STATS_TAG, MPI_COMM_WORLD));
        }
        else {
            std::vector<char*> incoming_messages;
            char * incoming;
            int i;
            for (i = 1; i < size; i++) {
                checkrc(
                    MPI_Recv(
                        msg_len, sizeof(msg_len), MPI_CHAR, i,
                        DCMPI_STATS_TAG, MPI_COMM_WORLD, &status));
                int len = atoi(msg_len);
                incoming = new char[len+1];
                checkrc(
                    MPI_Recv(
                        incoming, len+1, MPI_CHAR, i,
                        DCMPI_STATS_TAG, MPI_COMM_WORLD, &status));
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

    // abandon any pending speculative in-requests
    for (i = 0; i < size; i++) {
        if (i == rank) continue;
        mpi_recv_info * info = mpi_recv_ranks[i];
        if (info->state == MPI_RECV_INFO_REQ_ACTIVE) {
            if (dcmpi_verbose())
                dbg("cancelling speculative inreq\n");
            (void)MPI_Cancel(&info->req);
        }
        delete info;
    }

    if (dcmpi_verbose())
        dbg("dcmpi_mainloop: exiting, iters was %d\n", iters);

    return 0;
}

int main(int argc, char * argv[])
{
    int rc = 0;
    int outrc;
    appname = argv[0];

    if (argc == 1) {
        usage();
    }
    
    dcmpiruntime_common_startup(argc, argv);

    if ((rc = MPI_Init(&argc, &argv)) != 0) {
        std::cerr << "ERROR: MPI_Init " << rc
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        exit(1);
    }

    if ((rc = MPI_Comm_rank(MPI_COMM_WORLD, &rank)) != 0) {
        std::cerr << "ERROR: MPI_Comm_rank " << rc
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        exit(1);
    }
    if ((rc = MPI_Comm_size(MPI_COMM_WORLD, &size)) != 0) {
        std::cerr << "ERROR: MPI_Comm_size " << rc
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        exit(1);
    }

    if (rank == 0) {
        // arguments are only propagated to rank 0
        parse_args(argc, argv);
    }

    outrc = dcmpiruntime_common_execute(dcmpi_mpi_get_and_broadcast_layout,
                                        dcmpi_mpi_mainloop);

    if (dcmpi_verbose())
        cout << id() << ": at finalize\n";
    if ((rc = MPI_Finalize()) != 0) {
        std::cerr << "ERROR: MPI_Finalize " << rc
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        exit(1);
    }
    if (dcmpi_verbose())
        cout << id() << ": ran finalize\n";
    cout << flush;
    cerr << flush;

    dcmpiruntime_common_cleanup();

    return outrc;
}
