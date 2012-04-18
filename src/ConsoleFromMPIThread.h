/***************************************************************************
    $Id: ConsoleFromMPIThread.h,v 1.7 2005/06/30 20:51:12 rutt Exp $
    Copyright (C) 2004 by BMI Department, The Ohio State University.
    For license info, see LICENSE in root directory of source distribution.
 ***************************************************************************/

#include "dcmpi_clib.h"

#include "dcmpi.h"

#include "DCFilterExecutor.h"
#include "DCMPIPacket.h"
#include "Queue.h"
#include "dcmpi_socket.h"
#include "MemUsage.h"
#include "Buffer_MUID.h"

class ConsoleFromMPIThread : public DCThread
{
private:
    bool uses_unix_socket_console_bridge;
    DCFilterExecutor * console_exe; // may be NULL
    int slisten;
    int s;
    DCLayout * layout;
public:
    ConsoleFromMPIThread(bool uses_unix_socket_console_bridge,
                         int slisten,
                         DCFilterExecutor * console_exe,
                         DCLayout * layout)
    {
        this->uses_unix_socket_console_bridge = uses_unix_socket_console_bridge;
        this->slisten = slisten;
        this->console_exe = console_exe;
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
        char hdr[DCMPI_PACKET_HEADER_SIZE];
        DCMPIPacket * p = new DCMPIPacket();

        connect();

        while (1)
        {
            // get next packet
            if (DC_read_all(s, hdr, sizeof(hdr)) != 0) {
                // triggered too many times by user hitting Ctrl+C
                exit(1);
            }
            p->init_from_bytearray(hdr);
            if (p->packet_type == DCMPI_PACKET_TYPE_DCBUF) {
//                 MUID_MemUsage[MUID_of_console_buffer_area]->
//                     accumulate(p->body_len);
                dcmpi_socket_read_dcbuffer(s, p);
            }

            // process p
            if (p->packet_type == DCMPI_PACKET_TYPE_CLOSE) {
                break;
            }
            else if (p->packet_type == DCMPI_PACKET_TYPE_REMOVE_PUTTER) {
                console_exe->in_ports[p->address.to_port]->remove_putter(
                    p->address.from_gftid);
            }
            else if (p->packet_type ==
                     DCMPI_PACKET_TYPE_ALL_MY_FILTERS_DONE) {
                ; // just consume them
            }
            else {
                Buffer_MUID bm(p->body, -1);
                console_exe->in_ports[p->address.to_port]->put(bm);
                p->body = NULL;
            }
        }
        disconnect();
        delete p;
    }
};
