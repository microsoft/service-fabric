// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;

namespace Store
{
    PartitionedReplicaId::PartitionedReplicaId(Common::Guid const & partitionId, ::FABRIC_REPLICA_ID replicaId)
        : traceId_() 
        , partitionId_(partitionId)
        , replicaId_(replicaId)
    {
        StringWriter(traceId_).Write("({0}:{1})", partitionId_, replicaId_);
    }

    PartitionedReplicaId::PartitionedReplicaId(PartitionedReplicaId const & other)
        : traceId_(other.TraceId) 
        , partitionId_(other.PartitionId)
        , replicaId_(other.ReplicaId)
    {
    }

    PartitionedReplicaId::PartitionedReplicaId(PartitionedReplicaId && other)
        : traceId_(move(other.TraceId))
        , partitionId_(move(other.PartitionId))
        , replicaId_(move(other.ReplicaId))
    {
    }


    void PartitionedReplicaId::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
    {
        w << this->TraceId;
    }

    string PartitionedReplicaId::AddField(Common::TraceEvent & traceEvent, string const & name)
    {   
        traceEvent.AddField<Guid>(name + ".partitionId");
        traceEvent.AddField<int64>(name + ".replicaId");    

        return "({0}:{1})";
    }

    void PartitionedReplicaId::FillEventData(Common::TraceEventContext & context) const
    {
        context.Write<Guid>(partitionId_);
        context.Write<int64>(static_cast<int64>(replicaId_));
    }
}
