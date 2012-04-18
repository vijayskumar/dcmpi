/***************************************************************************
    $Id: dcmpi_dll.cpp,v 1.2 2005/03/21 18:14:11 rutt Exp $
    Copyright (C) 2004 by BMI Department, The Ohio State University.
    For license info, see LICENSE in root directory of source distribution.
 ***************************************************************************/

#include "dcmpi_clib.h"

#include "dcmpi_dll.h"

using namespace std;

void * dcmpi_dll_get_function(const std::string & filename,
                              const std::string & funcname)
{
    static DllLoader loader;
    return loader.getFunc(filename, funcname);
}
