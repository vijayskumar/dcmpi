/***************************************************************************
    $Id: Gftid_Port.cpp,v 1.1.1.1 2005/02/17 13:45:36 rutt Exp $
    Copyright (C) 2004 by BMI Department, The Ohio State University.
    For license info, see LICENSE in root directory of source distribution.
 ***************************************************************************/

#include "dcmpi_clib.h"

#include "Gftid_Port.h"

using namespace std;

std::ostream& operator<<(std::ostream &o, const Gftid_Port& i)
{
    return o << i.gftid << "[" << i.port << "]";
}
