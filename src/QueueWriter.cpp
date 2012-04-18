#include "dcmpi_clib.h"
#include "QueueWriter.h"

using namespace std;
void RemoteQueueWriter::feed(DCBuffer * buf)
{
    DCMPIPacket * p = new DCMPIPacket(
        addr, DCMPI_PACKET_TYPE_DCBUF, buf->getUsedSize(), buf);
    q->put(p);
}
