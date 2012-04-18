#ifndef DCFILELOCK_H
#define DCFILELOCK_H

#include "dcmpi_internal.h"

#include "dcmpi_util.h"

class DCFileLock
{
    std::string filename;
    int fd;
    off_t filesize;
public:
    DCFileLock(std::string filename)
    {
        this->filename = filename;
        fd = open(filename.c_str(), O_RDWR);
        if (fd == -1) {
            std::cerr << "ERROR: opening file " << filename
                      << ", which cannot be opened"
                      << " at " << __FILE__ << ":" << __LINE__
                      << std::endl << std::flush;
            exit(1);
        }
        filesize = file_size(filename);
     }
    // int lockf(int fd, int cmd, off_t len);
    void lock()
    {
        int tries = 0;
        while (tries < 120) {
            if (lockf(fd, F_TLOCK, filesize) == 0) {
                return;
            }
            tries++;
            dcmpi_doublesleep(0.5);
        }
        std::cerr << "ERROR: locking file "
                  << filename << ", tried 120 times with half-second delay "
                  << "in between, errno=" << errno
                  << " at " << __FILE__ << ":" << __LINE__
                  << std::endl << std::flush;
        exit(1);

        // old way, waited too long on NFS
//         if (lockf(fd, F_LOCK, filesize) == -1) {
//             std::cerr << "ERROR: calling lockf(), errno=" << errno
//                       << " at " << __FILE__ << ":" << __LINE__
//                       << std::endl << std::flush;
//             exit(1);
//         }
    }
    void unlock() // note: an alternative way to unlock is to simply close the
                  // file (see the man page for lockf)
    {
        if (lockf(fd, F_ULOCK, filesize) == -1) {
            std::cerr << "ERROR: calling lockf(), errno=" << errno
                      << " at " << __FILE__ << ":" << __LINE__
                      << std::endl << std::flush;
            exit(1);
        }
    }
    int getfd()
    {
        return fd;
    }
    
};

#endif /* #ifndef DCFILELOCK_H */
