// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;

namespace Store
{
    StringLiteral const TraceComponent("StoreData");

    StoreData::StoreData()
        : ReplicaActivityTraceComponent(PartitionedReplicaId(Guid::Empty(), 0), Common::ActivityId())
        , cachedKey_(L"")
        , sequenceNumber_(0)
    {
    }
  
    StoreData::StoreData(
        PartitionedReplicaId const & partitionedReplicaId,
        Common::ActivityId const & activityId)
        : ReplicaActivityTraceComponent(partitionedReplicaId, activityId)
        , cachedKey_(L"")
        , sequenceNumber_(0)
    {
    }

    StoreData::StoreData(
        Store::ReplicaActivityId const & replicaActivityId)
        : ReplicaActivityTraceComponent(replicaActivityId)
        , cachedKey_(L"")
        , sequenceNumber_(0)
    {
    }

    StoreData::StoreData(StoreData const & other)
        : ReplicaActivityTraceComponent(other)
        , cachedKey_(other.cachedKey_)
        , sequenceNumber_(other.sequenceNumber_)
    {
    }

    StoreData & StoreData::operator = (StoreData const & other)
    {
        if (this != &other)
        {
            cachedKey_ = other.cachedKey_;
            sequenceNumber_ = other.sequenceNumber_;
        }

        ReplicaActivityTraceComponent::operator=(other);
        return *this;
    }

    StoreData::StoreData(StoreData && other)
        : ReplicaActivityTraceComponent(move(other))
        , cachedKey_(move(other.cachedKey_))
        , sequenceNumber_(move(other.sequenceNumber_))
    {
    }

    StoreData & StoreData::operator = (StoreData && other)
    {
        if (this != &other)
        {
            cachedKey_ = move(other.cachedKey_);
            sequenceNumber_ = move(other.sequenceNumber_);
        }

        ReplicaActivityTraceComponent::operator=(move(other));
        return *this;
    }

    StoreData::~StoreData()
    {
    }

    std::wstring const & StoreData::get_Key() const
    {
        if (cachedKey_.empty())
        {
            cachedKey_ = ConstructKey();
        }

        return cachedKey_;
    }

    void StoreData::SetSequenceNumber(FABRIC_SEQUENCE_NUMBER sequenceNumber)
    {
        if (sequenceNumber >= 0)
        {
            sequenceNumber_ = sequenceNumber;
        }
        else
        {
            TRACE_LEVEL_AND_TESTASSERT(Trace.WriteError, TraceComponent, "Ignoring negative sequence number: {0}", sequenceNumber);
        }
    }
}
