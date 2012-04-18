#ifndef BUFFER_MUID_H
#define BUFFER_MUID_H

#include "dcmpi.h"

class Buffer_MUID
{
public:
    Buffer_MUID() : buffer(NULL), muid(-1) {}
    Buffer_MUID(DCBuffer * b, int m) : buffer(b), muid(m) {}
    DCBuffer * buffer;
    int muid; // indicates whose write buffer space was consumed to write
              // this packet
};

#endif /* #ifndef BUFFER_MUID_H */
