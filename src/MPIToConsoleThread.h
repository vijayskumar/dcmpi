#ifndef MPITOCONSOLETHREAD_H
#define MPITOCONSOLETHREAD_H

/***************************************************************************
    $Id: MPIToConsoleThread.h,v 1.5 2005/10/28 16:51:52 rutt Exp $
    Copyright (C) 2004 by BMI Department, The Ohio State University.
    For license info, see LICENSE in root directory of source distribution.
 ***************************************************************************/

#include "dcmpi.h"
#include "DCMPIPacket.h"
#include "Queue.h"
#include "dcmpi_util.h"
#include "MemUsage.h"

class MPIToConsoleThread : public DCThread
{
private:
    int s;
    Queue<DCMPIPacket*> * q;
    DCLayout * layout;
    std::set<int> local_gftids;
public:
    MPIToConsoleThread(int s,
                       Queue<DCMPIPacket*> * q,
                       DCLayout * layout)
    {
        this->s = s;
        this->q = q;
        this->layout = layout;
        for (uint u = 0; u < layout->filter_instances.size(); u++) {
            DCFilterInstance * fi = layout->filter_instances[u];
            if (fi->dcmpi_rank == 0) {
                local_gftids.insert(fi->gftid);
            }
        }
    }
    void run()
    {
        int rc = 0;
        char hdr[DCMPI_PACKET_HEADER_SIZE];
        DCMPIPacket * p;

        while ((p = q->get()) != NULL) {
            memset(hdr, 0, sizeof(hdr));
            p->to_bytearray(hdr);
            if ((rc = DC_write_all(s, hdr, DCMPI_PACKET_HEADER_SIZE) != 0)) {
                std::cerr << "ERROR: writing console to from MPI process"
                          << " at " << __FILE__ << ":" << __LINE__
                          << std::endl << std::flush;
                assert(0);
            }
            
            if (p->packet_type == DCMPI_PACKET_TYPE_DCBUF) {
                int length = p->body_len; // dcmpi_socket_write_dcbuffer() may
                                          // fragment the packet
                dcmpi_socket_write_dcbuffer(s, p);
                int source_gftid = p->address.from_gftid;
                if (local_gftids.count(source_gftid) == 0) {
                    source_gftid = MUID_of_dcmpiruntime;
                }
                MUID_MemUsage[source_gftid]->reduce(length);
            }
            delete p;
        }

        // inform rank -1 that we're closing
        DCMPIPacket bye(DCMPIPacketAddress(-1, 0, -1, "",
                                           0, 0, -1, ""),
                        DCMPI_PACKET_TYPE_CLOSE, 0);
        bye.to_bytearray(hdr);
        if ((rc = DC_write_all(s, hdr, DCMPI_PACKET_HEADER_SIZE) != 0)) {
            std::cerr << "ERROR: writing to MPI from console process"
                      << " at " << __FILE__ << ":" << __LINE__
                      << std::endl << std::flush;
            assert(0);
        }

        close(s);
    }
};

#endif /* #ifndef MPITOCONSOLETHREAD_H */
