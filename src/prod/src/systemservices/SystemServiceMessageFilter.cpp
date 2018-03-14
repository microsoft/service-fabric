// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Transport;
using namespace SystemServices;

SystemServiceMessageFilter::SystemServiceMessageFilter(SystemServiceLocation const & serviceLocation)
    : partitionId_(serviceLocation.PartitionId)
    , replicaId_(serviceLocation.ReplicaId)
    , replicaInstance_(serviceLocation.ReplicaInstance)
{
}

SystemServiceMessageFilter::SystemServiceMessageFilter(SystemServiceFilterHeader const & filterHeader)
    : partitionId_(filterHeader.PartitionId)
    , replicaId_(filterHeader.ReplicaId)
    , replicaInstance_(filterHeader.ReplicaInstance)
{
}

SystemServiceMessageFilter::~SystemServiceMessageFilter()
{
}

bool SystemServiceMessageFilter::operator < (SystemServiceMessageFilter const &  other) const
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

bool SystemServiceMessageFilter::Match(Message & message)
{
    SystemServiceFilterHeader filterHeader;
    if (!message.Headers.TryReadFirst(filterHeader))
    {
        Assert::TestAssert("Could not read SystemServiceFilterHeader");
        return false;
    }

    bool match = false;
    if (filterHeader.PartitionId == partitionId_)
    {
        match = true;
    }

    if (match && filterHeader.ReplicaId != SystemServiceLocation::ReplicaId_AnyReplica)
    {
        match = (filterHeader.ReplicaId == replicaId_);
    }

    if (match && filterHeader.ReplicaInstance != SystemServiceLocation::ReplicaInstance_AnyReplica)
    {
        match = (filterHeader.ReplicaInstance == replicaInstance_);
    }

    return match;
}
