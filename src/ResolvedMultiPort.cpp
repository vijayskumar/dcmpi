/***************************************************************************
    $Id: ResolvedMultiPort.cpp,v 1.1.1.1 2005/02/17 13:45:36 rutt Exp $
    Copyright (C) 2004 by BMI Department, The Ohio State University.
    For license info, see LICENSE in root directory of source distribution.
 ***************************************************************************/

#include "dcmpi_clib.h"

#include "ResolvedMultiPort.h"

using namespace std;

std::ostream& operator<<(std::ostream &o, const ResolvedMultiPort & i)
{
    uint u;
    for (u = 0; u < i.remotes.size(); u++) {
        o << i.remotes[u];
        if (u != (i.remotes.size()-1))
            o << " ";
    }
    return o;
}
