#include "dcmpi_clib.h"
#include "DCMPIPacket.h"

using namespace std;

DCMPIPacket::~DCMPIPacket()
{
    delete body;
    delete[] internal_hdr;
}
DCMPIPacket::DCMPIPacket()
{
    body = NULL;
    internal_hdr = NULL;
}
DCMPIPacket::DCMPIPacket(
    DCMPIPacketAddress addr, int4 packet_type, int4 body_len,
    DCBuffer * body)
{
    address = addr;
    this->packet_type = packet_type;
    this->body_len = body_len;
    this->body = body;
    internal_hdr = NULL;
}

std::ostream& operator<<(std::ostream &o, const DCMPIPacketAddress & a)
{
    o << "to_rank=" << a.to_rank;
    o << ",to_cluster_rank=" << a.to_cluster_rank;
    o << ",to_gftid=" << a.to_gftid;
    o << ",to_port=" << a.to_port;
    return o;
}
std::ostream& operator<<(std::ostream &o, const DCMPIPacket & p)
{
    o << "[packet " << &p << "]" << endl;
    o << "\n  packet_type=" << p.packet_type;
    o << "\n  address:" << p.address;
    o << "\n  body_len=" << p.body_len;
    if (p.body) {
        o << "\n  buffer=" << *p.body;
    }
    return o;
}
