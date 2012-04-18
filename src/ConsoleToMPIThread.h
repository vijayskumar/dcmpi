/***************************************************************************
    $Id: ConsoleToMPIThread.h,v 1.6 2006/05/26 22:06:53 rutt Exp $
    Copyright (C) 2004 by BMI Department, The Ohio State University.
    For license info, see LICENSE in root directory of source distribution.
 ***************************************************************************/

class ConsoleToMPIThread : public DCThread
{
    bool uses_unix_socket_console_bridge;
    int slisten;
    int s;
    Queue<DCMPIPacket*> * q;
    DCLayout * layout;
public:
    ConsoleToMPIThread(bool uses_unix_socket_console_bridge,
                       int slisten,
                       Queue<DCMPIPacket*> * q,
                       DCLayout * layout)
    {
        this->uses_unix_socket_console_bridge = uses_unix_socket_console_bridge;
        this->slisten = slisten;
        this->q = q;
        this->layout = layout;
    }
    void connect()
    {
        // accept just 1 client, rank 0 of mpi
        if (uses_unix_socket_console_bridge) {
            struct sockaddr_un addr;
            socklen_t addr_len = sizeof(addr);
            s = accept(slisten, (struct sockaddr*)&addr, (socklen_t*)&addr_len);
        }
        else {
            struct sockaddr_in addr;
            socklen_t addr_len = sizeof(addr);
            s = accept(slisten, (struct sockaddr*)&addr, (socklen_t*)&addr_len);
        }
        if (s == -1) {
            std::cerr << "ERROR: accepting on socket"
                      << " at " << __FILE__ << ":" << __LINE__
                      << std::endl << std::flush;
            exit(1);
        }
    }
    void disconnect()
    {
        close(s);
        close(slisten);
    }
    void run()
    {
        int rc;
        char hdr[DCMPI_PACKET_HEADER_SIZE];
        connect();
        DCMPIPacket * p;

        // get until you get a NULL from the console process which signals it
        // to stop
        while ((p = q->get()) != NULL) {
            p->to_bytearray(hdr);
            if ((rc = DC_write_all(s, hdr, DCMPI_PACKET_HEADER_SIZE) != 0)) {
                std::cerr << "ERROR: writing to MPI from console process"
                          << " at " << __FILE__ << ":" << __LINE__
                          << std::endl << std::flush;
                exit(1);
            }
            if (p->packet_type == DCMPI_PACKET_TYPE_LAYOUT) {
                dcmpi_socket_write_dcbuffer(s, p);
            }
            else if (p->packet_type == DCMPI_PACKET_TYPE_DCBUF) {
                dcmpi_socket_write_dcbuffer(s, p);
                local_console_outgoing_memusage.reduce(p->body_len);
            }
            delete p;
        }

        // inform rank 0 that we're closing
        DCMPIPacket bye(DCMPIPacketAddress(0, 0, -1, "",
                                           -1, 0, -1, ""),
                        DCMPI_PACKET_TYPE_CLOSE, 0);
        bye.to_bytearray(hdr);
        if ((rc = DC_write_all(s, hdr, DCMPI_PACKET_HEADER_SIZE) != 0)) {
            std::cerr << "ERROR: writing to MPI from console process"
                      << " at " << __FILE__ << ":" << __LINE__
                      << std::endl << std::flush;
            exit(1);
        }

        disconnect();
    }
};
