/***************************************************************************
    $Id: dcmpi_home.cpp,v 1.3 2005/07/01 15:32:32 rutt Exp $
    Copyright (C) 2004 by BMI Department, The Ohio State University.
    For license info, see LICENSE in root directory of source distribution.
 ***************************************************************************/

#include "dcmpi_clib.h"

#include "DCMutex.h"
#include "dcmpi_util.h"

#include <sys/types.h>
#include <pwd.h>

using namespace std;

std::string dcmpi_get_home()
{
    static std::string dcmpi_home;
    static DCMutex mutex;

    mutex.acquire();
    if ((!dcmpi_is_local_console && !dcmpi_remote_console_snarfed_layout) ||
        dcmpi_home.empty()) {
        if (getenv("DCMPI_HOME")) {
            dcmpi_home = getenv("DCMPI_HOME");
        }
        else {
            dcmpi_home = getenv("HOME");
            if (!fileExists(dcmpi_home)) {
                struct passwd * pw;
                dcmpi_home = "/tmp/";
                pw = getpwuid(getuid());
                dcmpi_home += pw->pw_name;
                endpwent();
            }
            dcmpi_home += "/.dcmpi";
        }
        if (!fileExists(dcmpi_home)) {
            cout << "NOTE:  creating DCMPI_HOME dir of " << dcmpi_home << endl;
            if (dcmpi_mkdir_recursive(dcmpi_home)) {
                std::cerr << "ERROR: DCMPI_HOME of " << dcmpi_home
                          << " cannot be created, so aborting" << std::endl;
                exit(1);
            }
        }
    }
    mutex.release();
    return dcmpi_home;
}
