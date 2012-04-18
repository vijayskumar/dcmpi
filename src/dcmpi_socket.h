#ifndef DCMPI_SOCKET_H
#define DCMPI_SOCKET_H

/***************************************************************************
    $Id: dcmpi_socket.h,v 1.2 2006/03/29 23:06:55 rutt Exp $
    Copyright (C) 2004 by BMI Department, The Ohio State University.
    For license info, see LICENSE in root directory of source distribution.
 ***************************************************************************/

#include "dcmpi_clib.h"

#include "dcmpi.h"

/* these return a valid (not -1) fd on success, or -1 on error.

   When you get back a client socket, you can start doing read/write on it.

   When you get back a listen socket, you still have to do:

       int acceptSd;
       struct sockaddr_in clientAddr;
       socklen_t clientAddrLen = sizeof(clientAddr);

       acceptSd = accept(listenSd, (struct sockaddr*)&clientAddr, (socklen_t*)&clientAddrLen);

   and then you can read/write on acceptSd.
*/
int dcmpiOpenClientSocket(const char * serverHost, uint2 port, int socket_bufsize=0);

int dcmpiOpenListenSocket(uint2 port, int socket_bufsize=0);

/* Open a dynamic port.  Formal parameter 'port' is an outval.*/
int dcmpiOpenListenSocket(uint2 * port, int socket_bufsize=0);

int dcmpiSetSocketReuse(int socket); // must be done before a 'bind'
int dcmpiGetSocketBufferSize(int socket, int & sendbuf, int & recvbuf);
int dcmpiSetSocketBufferSize(int socket, int size);
int dcmpiCloseSocket(int & socket);

// UDP STUFF (client side only for now)
typedef struct UDPSOCKET
{
    int         fd;
    sockaddr_in addr;
} UDPSOCKET;
int dcmpiOpenUdpClientSocket(const char * serverHost,
                             uint2 port,
                             UDPSOCKET * outval);
int dcmpiSendUdp(UDPSOCKET * sock, const char * message, size_t len);
int dcmpiCloseUdpClientSocket(UDPSOCKET * sock);

// UNIX SOCKET STUFF
/* these return a valid (not -1) fd on success, or -1 on error.

   When you get back a client socket, you can start doing read/write on it.

   When you get back a listen socket, you still have to do:

       int acceptSd;
       struct sockaddr_un clientAddr;
       socklen_t clientAddrLen = sizeof(clientAddr);

       acceptSd = accept(listenSd, (struct sockaddr*)&clientAddr, (socklen_t*)&clientAddrLen);

   and then you can read/write on acceptSd.
*/
int dcmpiOpenUnixListenSocket(const char * filename);
int dcmpiOpenUnixClientSocket(const char * filename);
int dcmpiCloseUnixSocket(int sd);

// utility funcs
std::string dcmpiGetPeerOfSocket(int sd);

// valid inputs are "localhost", any IP address, or a hostname resolvable from
// this host
std::string dcmpi_get_fqdn(std::string hostname);

std::vector<std::string> canonicalize_hostvec(
    const std::vector<std::string> & hosts);

#endif /* #ifndef DCMPI_SOCKET_H */
