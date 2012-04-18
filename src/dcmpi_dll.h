#ifndef DCMPI_DLL_H
#define DCMPI_DLL_H

/***************************************************************************
    $Id: dcmpi_dll.h,v 1.4 2005/07/28 15:25:06 rutt Exp $
    Copyright (C) 2004 by BMI Department, The Ohio State University.
    For license info, see LICENSE in root directory of source distribution.
 ***************************************************************************/

#include "dcmpi_clib.h"

#include "DCMutex.h"

class DllLoader
{
public:
    ~DllLoader()
    {
        std::map<std::string, void *>::iterator it;
        for (it = _filename_to_lib_handle.begin();
             it != _filename_to_lib_handle.end();
             it++) {
            dlclose(it->second);
        }

    }
    void * getFunc(const std::string & filename,
                   const std::string & funcname)
    {
        _mutex.acquire();
        if (_filename_to_lib_handle.count(filename) == 0) {
            void * libHandle = dlopen(filename.c_str(), RTLD_LAZY);
            if (!libHandle) {
                std::cerr << "ERROR: loading dynamic library " << filename;
                std::cerr << "\nThe specific error message from libdl was: "
                          << dlerror() << std::endl;
                _mutex.release();
                return NULL;
            }
            _filename_to_lib_handle[filename] = libHandle;
        }
        void * out = dlsym(_filename_to_lib_handle[filename], funcname.c_str());
        if (!out) {
            std::cerr << "dlerror: " << dlerror() << std::endl;
        }
        _mutex.release();
        return out;
    }
        
private:
    DCMutex _mutex;
    std::map<std::string, void *> _filename_to_lib_handle;
};

// thread-safe way to get function from a file
void * dcmpi_dll_get_function(const std::string & filename,
                              const std::string & funcname);

#endif /* #ifndef DCMPI_DLL_H */
