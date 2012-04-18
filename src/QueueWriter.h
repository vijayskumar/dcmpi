#ifndef QUEUEWRITER_H
#define QUEUEWRITER_H

/***************************************************************************
    $Id: QueueWriter.h,v 1.6 2006/03/17 15:47:42 rutt Exp $
    Copyright (C) 2004 by BMI Department, The Ohio State University.
    For license info, see LICENSE in root directory of source distribution.
 ***************************************************************************/

#include "DCMPIPacket.h"

#include "Queue.h"
#include "Buffer_MUID.h"

class QueueWriter
{
public:
    virtual ~QueueWriter() {}
    virtual void feed(DCBuffer * buf) = 0;
    virtual void remove_putter(int gftid) = 0;
    virtual DCFilterInstance * get_remote_instance() = 0;
};

class LocalQueueWriter : public QueueWriter
{
    Queue<Buffer_MUID> * q;
    DCFilterInstance* remote_instance;
    Buffer_MUID reusable_buffer_muid; // avoid extra copying
public:
    LocalQueueWriter(Queue<Buffer_MUID> * q,
                     int local_MUID,
                     DCFilterInstance * remote_instance) :
        reusable_buffer_muid(NULL,local_MUID)
    {
        this->q = q;
        this->remote_instance = remote_instance;
    }
    void feed(DCBuffer* buf)
    {
        reusable_buffer_muid.buffer = buf;
        q->put(reusable_buffer_muid);
    }
    void remove_putter(int gftid) {
        q->remove_putter(gftid);
    }
    DCFilterInstance * get_remote_instance() { return remote_instance; }
};

class RemoteQueueWriter : public QueueWriter
{
    DCMPIPacketAddress addr;
    Queue<DCMPIPacket*> * q;
    DCFilterInstance* remote_instance;
public:
    RemoteQueueWriter(int torank, int toclusterrank,
                      int togftid, std::string toport,
                      int fromrank, int fromclusterrank,
                      int fromgftid, std::string fromport,
                      Queue<DCMPIPacket*> * q,
                      DCFilterInstance * remote_instance)
    {
        addr = DCMPIPacketAddress(torank, toclusterrank,
                                  togftid, toport,
                                  fromrank, fromclusterrank,
                                  fromgftid, fromport);
        assert(q);
        this->q = q;
        this->remote_instance = remote_instance;
    }
    virtual void feed(DCBuffer * buf);
    void remove_putter(int gftid) {
        assert(gftid == addr.from_gftid);
        q->put(new DCMPIPacket(addr, DCMPI_PACKET_TYPE_REMOVE_PUTTER, 0));
    }
    DCFilterInstance * get_remote_instance() { return remote_instance; }
};

#endif /* #ifndef QUEUEWRITER_H */
