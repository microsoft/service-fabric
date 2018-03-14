// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Federation;
using namespace Transport;
using namespace SystemServices;

bool SystemServiceFilterHeader::operator < (SystemServiceFilterHeader const & other)
{
    if (partitionId_ < other.partitionId_)
    {
        return true;
    }
    else if (other.partitionId_ < partitionId_)
    {
        return false;
    }
    else
    {
        if (replicaId_ < other.replicaId_)
        {
            return true;
        }
        else if (replicaId_ > other.replicaId_)
        {
            return false;
        }
        else
        {
            if (replicaInstance_ < other.replicaInstance_)
            {
                return true;
            }
            else
            {
                return false;
            }
        }
    }
}
