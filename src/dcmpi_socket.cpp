/***************************************************************************
    $Id: dcmpi_socket.cpp,v 1.6 2006/08/21 16:45:37 rutt Exp $
    Copyright (C) 2004 by BMI Department, The Ohio State University.
    For license info, see LICENSE in root directory of source distribution.
 ***************************************************************************/

#include "dcmpi_clib.h"

#include "dcmpi_socket.h"
#include "DCMutex.h"

using namespace std;

int dcmpiOpenClientSocket(const char * serverHost, uint2 port, int socket_bufsize)
{
    int serverSocket = -1;
    struct sockaddr_in remoteAddr;
    struct hostent * hostEnt;

    if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        serverSocket = -1;
        goto Exit;
    }
    if (socket_bufsize) {
        int rc = dcmpiSetSocketBufferSize(serverSocket, socket_bufsize);
        if (rc) {
            std::cerr << "ERROR: dcmpiSetSocketBufferSize() errno=" << errno
                      << " at " << __FILE__ << ":" << __LINE__
                      << std::endl << std::flush;
        }
    }
    memset(&remoteAddr, 0, sizeof(remoteAddr));
    remoteAddr.sin_family = AF_INET;
    remoteAddr.sin_port = htons(port);
    hostEnt = gethostbyname(serverHost);
    if (hostEnt == NULL) {
        close(serverSocket);
        serverSocket = -1;
        goto Exit;
    }
    memcpy(&remoteAddr.sin_addr.s_addr, hostEnt->h_addr_list[0],
           hostEnt->h_length);

    while (1) {
        if (connect(serverSocket, (struct sockaddr*)&remoteAddr,
                    sizeof(remoteAddr)) == -1) {
            if (errno == EAGAIN ||
                errno == ETIMEDOUT) {
                dcmpi_doublesleep(0.01);
                continue;
            }
            else {
                std::cerr << "ERROR: (outgoing tcp connection) errno=" << errno
                          << " connecting to " << serverHost
                          << ":" << port
                          << " on host " << dcmpi_get_hostname()
                          << " at " << __FILE__ << ":" << __LINE__
                          << std::endl << std::flush;
                close(serverSocket);
                serverSocket = -1;
                goto Exit;
            }
        }
        else {
            break;
        }
    }

Exit:
    return serverSocket;
}

int dcmpiOpenUdpClientSocket(const char * serverHost,
                             uint2 port,
                             UDPSOCKET * outval)
{
    int serverSocket = -1;
    struct sockaddr_in remoteAddr;
    struct hostent * hostEnt;

    if ((serverSocket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        return -1;
    }
    remoteAddr.sin_family = AF_INET;
    remoteAddr.sin_port = htons(port);
    hostEnt = gethostbyname(serverHost);
    if (hostEnt == NULL) {
        close(serverSocket);
        return -1;
    }
    memcpy(&remoteAddr.sin_addr.s_addr, hostEnt->h_addr_list[0],
           hostEnt->h_length);
    outval->fd = serverSocket;
    outval->addr = remoteAddr;
    return 0;
}

int dcmpiSendUdp(UDPSOCKET * sock, const char * message, size_t len)
{
    return sendto(sock->fd, message, len, 0,
                  (const sockaddr*)&(sock->addr),
                  sizeof(struct sockaddr_in));
}

int dcmpiCloseUdpClientSocket(UDPSOCKET * sock)
{
    return dcmpiCloseSocket(sock->fd);
}

int dcmpiOpenListenSocket(uint2 port, int socket_bufsize)
{
    struct sockaddr_in serverAddr;
    int serverSd;
    
    if ((serverSd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        return -1;
    }
    if (dcmpiSetSocketReuse(serverSd)) {
        return -1;
    }
    if (socket_bufsize) {
        int rc = dcmpiSetSocketBufferSize(serverSd, socket_bufsize);
        if (rc) {
            std::cerr << "ERROR: dcmpiSetSocketBufferSize() errno=" << errno
                      << " at " << __FILE__ << ":" << __LINE__
                      << std::endl << std::flush;
        }
    }
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    if ((bind(serverSd, (struct sockaddr*)&serverAddr,
              sizeof(serverAddr)) != 0)) {
        return -1;
    }
    if (listen(serverSd, 5) != 0) {
        return -1;
    }
    return serverSd;
}

int dcmpiOpenListenSocket(uint2 * port, int socket_bufsize)
{
    struct sockaddr_in serverAddr;
    struct sockaddr_in actualAddr;
    socklen_t actualAddrLen = sizeof(actualAddrLen);
    int serverSd;
    
    if ((serverSd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        return -1;
    }
    if (dcmpiSetSocketReuse(serverSd)) {
        return -1;
    }
    if (socket_bufsize) {
        int rc = dcmpiSetSocketBufferSize(serverSd, socket_bufsize);
        if (rc) {
            std::cerr << "ERROR: dcmpiSetSocketBufferSize() errno=" << errno
                      << " at " << __FILE__ << ":" << __LINE__
                      << std::endl << std::flush;
        }
    }
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(0);
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(serverSd, (struct sockaddr*)&serverAddr,sizeof(serverAddr)) != 0) {
        return -1;
    }
    if (listen(serverSd, 5) != 0) {
        return -1;
    }

    if (getsockname(serverSd, (struct sockaddr*)&actualAddr, &actualAddrLen)
        != 0) {
        return -1;
    }
    *port = (uint2)ntohs(actualAddr.sin_port);

    return serverSd;
}

int dcmpiSetSocketReuse(int socket)
{
    int opt = 1;
    if (setsockopt(socket, SOL_SOCKET, SO_REUSEADDR,
                   &opt, sizeof(opt)) != 0) {
        return -1;
    }
    return 0;
}

int dcmpiSetSocketBufferSize(int socket, int size)
{
    int rc;
    rc = setsockopt(socket, SOL_SOCKET, SO_SNDBUF, (char*)&size, sizeof(size));
    if (!rc) {
        rc = setsockopt(socket, SOL_SOCKET, SO_RCVBUF,
                        (char*)&size, sizeof(size));
    }
    return rc;
}

int dcmpiGetSocketBufferSize(int socket, int & sendbuf, int & recvbuf)
{
    int rc;
    socklen_t sz = sizeof(int);
    rc=getsockopt(socket, SOL_SOCKET, SO_SNDBUF,
                  (char*)&sendbuf, &sz);
    if (rc==0) {
        rc = getsockopt(socket, SOL_SOCKET, SO_RCVBUF,
                        (char*)&recvbuf, &sz);
    }
    return rc;
}

int dcmpiCloseSocket(int & socket)
{
    if (socket >= 0) {
        if (shutdown(socket, SHUT_RDWR) != 0) {
            return -1;
        }
        if (close(socket) != 0) {
            return -1;
        }
        socket = -1;
    }
    return 0;
}

std::string dcmpiGetPeerOfSocket(int sd)
{
    static DCMutex mutex;

    mutex.acquire();
    struct sockaddr_in tmpAddr;
    socklen_t tmpLen = sizeof(tmpAddr);
    getpeername(sd, (struct sockaddr*)&tmpAddr, &tmpLen);
    std::string out = inet_ntoa(tmpAddr.sin_addr);
    mutex.release();
    return out;
}

static std::string dcmpi_gethostbyname(std::string in)
{
    std::string out;
    static DCMutex mutex;
    mutex.acquire();
    struct hostent *phost = gethostbyname(in.c_str());
    if (phost)
        out = phost->h_name;
    else
        out = in;
    mutex.release();
    return out;
}

std::string dcmpi_get_fqdn(std::string hostname)
{
    std::string out;
    if ((hostname == "localhost") ||
        (hostname == "0.0.0.0")) {
        char localhost[1024];
        gethostname(localhost, sizeof(localhost));
        localhost[sizeof(localhost)-1] = 0;
        out = dcmpi_gethostbyname(localhost);
    }
    else {
        out = dcmpi_gethostbyname(hostname);
    }
    return out;
}

std::vector<std::string> canonicalize_hostvec(
    const std::vector<std::string> & hosts)
{
    std::vector<std::string> out;
    for (uint u = 0; u < hosts.size(); u++) {
        out.push_back(dcmpi_get_fqdn(hosts[u]));
    }
    return out;
}

int dcmpiOpenUnixListenSocket(const char * filename)
{
//               #define UNIX_PATH_MAX    108

//               struct sockaddr_un {
//                   sa_family_t  sun_family;              /* AF_UNIX */
//                   char         sun_path[UNIX_PATH_MAX]; /* pathname */
//               };


    struct sockaddr_un serverAddr;
    int serverSd;
    int len;
    
    assert(strlen(filename) < 108);
    if ((serverSd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        return -1;
    }
    serverAddr.sun_family = AF_UNIX;
    strcpy(serverAddr.sun_path, filename);
    len = strlen(serverAddr.sun_path) + sizeof(serverAddr.sun_family);
    if (bind(serverSd, (struct sockaddr *)&serverAddr, len) == -1) {
        std::cerr << "ERROR: bind()"
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        return -1;
    }
    if (listen(serverSd, 5) != 0) {
        return -1;
    }
    return serverSd;
}

int dcmpiCloseUnixSocket(int sd)
{
    return close(sd);
}

int dcmpiOpenUnixClientSocket(const char * filename)
{

    int s;
    struct sockaddr_un remote;
    assert(strlen(filename) < 108);

    if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        std::cerr << "ERROR: calling socket()"
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        return -1;
    }

    remote.sun_family = AF_UNIX;
    strcpy(remote.sun_path, filename);
    int len = strlen(remote.sun_path) + sizeof(remote.sun_family);
    if (connect(s, (struct sockaddr *)&remote, len) == -1) {
        std::cerr << "ERROR: calling connect()"
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
    }
    return s;
}
