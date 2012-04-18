#ifndef MPIFROMCONSOLETHREAD_H
#define MPIFROMCONSOLETHREAD_H

/***************************************************************************
    $Id: MPIFromConsoleThread.h,v 1.3 2006/06/06 15:53:07 rutt Exp $
    Copyright (C) 2004 by BMI Department, The Ohio State University.
    For license info, see LICENSE in root directory of source distribution.
 ***************************************************************************/

class MPIFromConsoleThread : public DCThread
{
private:
    int s;
    Queue<DCMPIPacket*> * q;
    DCLayout * layout;
public:
    MPIFromConsoleThread(int s,
                         Queue<DCMPIPacket*> * q,
                         DCLayout * layout)
    {
        this->s = s;
        this->q = q;
        this->layout = layout;
    }
    void run()
    {
        char hdr[DCMPI_PACKET_HEADER_SIZE];
        DCMPIPacket * p;

        while (1) {
            p = new DCMPIPacket();
            if (DC_read_all(s, hdr, sizeof(hdr)) != 0) {
//                 std::cerr << "ERROR: DC_read_all"
//                           << " at " << __FILE__ << ":" << __LINE__
//                           << std::endl << std::flush;
                exit(1);
            }
            p->init_from_bytearray(hdr);
            if (p->packet_type == DCMPI_PACKET_TYPE_LAYOUT) {
                dcmpi_socket_read_dcbuffer(s, p);
            }
            else if (p->packet_type == DCMPI_PACKET_TYPE_DCBUF) {
                MUID_MemUsage[MUID_of_dcmpiruntime]->accumulate(p->body_len);
                dcmpi_socket_read_dcbuffer(s, p);
            }
            
            if (p->packet_type == DCMPI_PACKET_TYPE_CLOSE) {
                delete p;
                break;
            }
            else {
                q->put(p); // deliver to mpi_from_console_queue
            }
        }
        q->remove_putter(-1); // symbolically -1
        close(s);
    }
};

#endif /* #ifndef MPIFROMCONSOLETHREAD_H */
