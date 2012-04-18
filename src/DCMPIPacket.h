#ifndef DCMPIPACKET_H
#define DCMPIPACKET_H

#include "dcmpi.h"
#include "dcmpi_typedefs.h"
#include "dcmpi_home.h"
#include "dcmpi_globals.h"
#include "RefCounted.h"

#include "dcmpi_backtrace.h"

class DCBuffer;

#define DCMPI_PORT_MAXLEN_SIZE 64
#define DCMPI_PACKET_HEADER_SIZE (1 + 4*8 + DCMPI_PORT_MAXLEN_SIZE*2)

// special packets
#define DCMPI_PACKET_TYPE_DCBUF               (int4)1
#define DCMPI_PACKET_TYPE_REMOVE_PUTTER       (int4)2
#define DCMPI_PACKET_TYPE_LAYOUT              (int4)3
#define DCMPI_PACKET_TYPE_ALL_MY_FILTERS_DONE (int4)4
#define DCMPI_PACKET_TYPE_CLOSE               (int4)5

class DCMPIPacketAddress
{
public:
    friend std::ostream& operator<<(std::ostream &o, const DCMPIPacket & p);
    DCMPIPacketAddress(){}
    DCMPIPacketAddress(int torank, int toclusterrank,
                       int togftid, std::string toport,
                       int fromrank, int fromclusterrank,
                       int fromgftid, std::string fromport)
        : to_rank(torank), to_cluster_rank(toclusterrank),
          to_gftid(togftid), to_port(toport),
          from_rank(fromrank), from_cluster_rank(fromclusterrank),
          from_gftid(fromgftid), from_port(fromport)
    {
        ;
    }
    int4 to_rank;
    int4 to_cluster_rank;
    int4 to_gftid;
    std::string to_port;

    int4 from_rank;
    int4 from_cluster_rank;
    int4 from_gftid;
    std::string from_port;
};

class DCMPIPacket : public RefCounted
{
public:
    friend std::ostream& operator<<(std::ostream &o, const DCMPIPacket & p);
    virtual ~DCMPIPacket();
    DCMPIPacket();
    DCMPIPacket(DCMPIPacketAddress addr, int4 packet_type, int4 body_len,
                DCBuffer * body=NULL);
    // hdr is of size PACKET_HEADER_SIZE
    void to_bytearray(char * hdr=NULL)
    {
        if (hdr == NULL) {
            if (!internal_hdr) {
                internal_hdr = new char[DCMPI_PACKET_HEADER_SIZE];
            }
            hdr = internal_hdr;
        }
        uint4 fill;
        uint4 len;

        uint1 endianVal = (dcmpi_is_big_endian())?1:0;
        hdr[0] = endianVal; hdr += 1;

        memcpy(hdr, &packet_type, 4);
        hdr += 4;
        memcpy(hdr, &address.to_rank, 4);
        hdr += 4;
        memcpy(hdr, &address.to_cluster_rank, 4);
        hdr += 4;
        memcpy(hdr, &address.to_gftid, 4);
        hdr += 4;
        len = address.to_port.length();
        memcpy(hdr, address.to_port.c_str(), len);
        hdr += len;
        fill = DCMPI_PORT_MAXLEN_SIZE-len;
        memset(hdr, 0, fill);
        hdr += fill;

        memcpy(hdr, &address.from_rank, 4);
        hdr += 4;
        memcpy(hdr, &address.from_cluster_rank, 4);
        hdr += 4;
        memcpy(hdr, &address.from_gftid, 4);
        hdr += 4;
        len = address.from_port.length();
        memcpy(hdr, address.from_port.c_str(), len);
        hdr += len;
        fill = DCMPI_PORT_MAXLEN_SIZE-len;
        memset(hdr, 0, fill);
        hdr += fill;
        memcpy(hdr, &body_len, 4);
        hdr += 4;
    }
#define DCMPI_SWP() if (swap_endian) { dcmpi_endian_swap(hdr,4); }
    void init_from_bytearray(char * hdr)
    {
        bool swap_endian = false;
        uint1 native_is_big_endian = (dcmpi_is_big_endian())?1:0;
        char s[DCMPI_PORT_MAXLEN_SIZE+1];

        if (hdr[0] != native_is_big_endian) {
            swap_endian = true;
        }
        hdr += 1;

		DCMPI_SWP();
        memcpy(&packet_type, hdr, 4);               hdr += 4;
        
        DCMPI_SWP();
        memcpy(&address.to_rank, hdr, 4);           hdr += 4;
        DCMPI_SWP(); 
        memcpy(&address.to_cluster_rank, hdr, 4);   hdr += 4;
        DCMPI_SWP(); 
        memcpy(&address.to_gftid, hdr, 4);          hdr += 4; 
        memcpy(s, hdr, DCMPI_PORT_MAXLEN_SIZE);
        s[DCMPI_PORT_MAXLEN_SIZE] = 0;
        address.to_port = s;
        hdr += DCMPI_PORT_MAXLEN_SIZE;
        
        DCMPI_SWP();
        memcpy(&address.from_rank, hdr, 4);         hdr += 4;
        DCMPI_SWP(); 
        memcpy(&address.from_cluster_rank, hdr, 4); hdr += 4;
        DCMPI_SWP(); 
        memcpy(&address.from_gftid, hdr, 4);        hdr += 4; 
        memcpy(s, hdr, DCMPI_PORT_MAXLEN_SIZE);
        s[DCMPI_PORT_MAXLEN_SIZE] = 0;
        address.from_port = s;
        hdr += DCMPI_PORT_MAXLEN_SIZE;

		DCMPI_SWP();
        memcpy(&body_len, hdr, 4);                  hdr += 4; 
    }
    DCMPIPacketAddress address;
    int4 packet_type;
    int4 body_len; // length of each body
    DCBuffer * body;
    char * internal_hdr;
};

#endif /* #ifndef DCMPIPACKET_H */
