#ifndef DCMPI_CLIB_H
#define DCMPI_CLIB_H

/***************************************************************************
    $Id: dcmpi_clib.h,v 1.5 2006/09/29 13:27:18 rutt Exp $
    Copyright (C) 2004 by BMI Department, The Ohio State University.
    For license info, see LICENSE in root directory of source distribution.
 ***************************************************************************/

#ifdef DCMPI_HAVE_PYTHON
#include <Python.h>
#endif

// orig C library
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

// unix/posix libraries
#include <arpa/inet.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <pwd.h>
#include <semaphore.h>
#include <sys/poll.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

// C++ includes
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <iterator>
#include <list>
#include <map>
#include <set>
#include <string>
#include <queue>
#include <vector>

#ifdef DCMPI_OLD_SSTREAM
#include <strstream.h>
#else
#include <sstream>
#endif

#endif /* #ifndef DCMPI_CLIB_H */
