// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;

namespace Store
{
    ReplicaActivityId::ReplicaActivityId(Store::PartitionedReplicaId const & partitionedReplicaId, Common::ActivityId const & activityId)
        : traceId_()
        , partitionedReplicaId_(partitionedReplicaId)
        , activityId_(activityId)
    {
        StringWriter(traceId_).Write(
            "[({0}:{1})+{2}]", 
            partitionedReplicaId_.PartitionId, 
            partitionedReplicaId_.ReplicaId, 
            activityId_);
    }

    ReplicaActivityId::ReplicaActivityId(ReplicaActivityId const & other)
        : traceId_(other.traceId_)
        , partitionedReplicaId_(other.partitionedReplicaId_)
        , activityId_(other.activityId_)
    {
    }

    ReplicaActivityId::ReplicaActivityId(ReplicaActivityId && other)
        : traceId_(move(other.traceId_))
        , partitionedReplicaId_(move(other.partitionedReplicaId_))
        , activityId_(move(other.activityId_))
    {
    }

    void ReplicaActivityId::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
    {
        w << this->TraceId;
    }

    string ReplicaActivityId::AddField(Common::TraceEvent & traceEvent, string const & name)
    {   
        string format = "[{0}+{1}]";
        size_t index = 0;

        traceEvent.AddEventField<Store::PartitionedReplicaId>(format, name + ".prId", index);
        traceEvent.AddEventField<Common::ActivityId>(format, name + ".actId", index);    

        return format;
    }

    void ReplicaActivityId::FillEventData(Common::TraceEventContext & context) const
    {
        context.Write<Store::PartitionedReplicaId>(partitionedReplicaId_);
        context.Write<Common::ActivityId>(activityId_);
    }
}
