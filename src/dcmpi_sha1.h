#ifndef _SHA1_H
#define _SHA1_H

#include <dcmpi.h>

typedef struct
{
    uint4 total[2];
    uint4 state[5];
    uint1 buffer[64];
}
sha1_context;

void sha1_starts( sha1_context *ctx );
void sha1_update( sha1_context *ctx, uint1 *input, uint4 length );
void sha1_finish( sha1_context *ctx, uint1 digest[20] );

#endif /* sha1.h */
