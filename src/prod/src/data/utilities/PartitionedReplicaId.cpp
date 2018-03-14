// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data::Utilities;

#define PARTITIONED_REPLICAID_TAG 'idRP'
#define MAX_TRACEID_LENGTH 128

PartitionedReplicaId::PartitionedReplicaId(
    __in KGuid const & partitionId, 
    __in ::FABRIC_REPLICA_ID replicaId)
    : traceId_() 
    , partitionId_(partitionId)
    , replicaId_(replicaId)
    , tracePartitionId_(partitionId)
{
    traceId_ = KStringA::Create(GetThisAllocator(), MAX_TRACEID_LENGTH);

    ASSERT_IF(traceId_ == nullptr, "OOM While allocating Trace String in PartitionedReplicaId");

    BOOLEAN result = traceId_->FromGUID(partitionId_);
    ASSERT_IFNOT(result == TRUE, "PartitionedReplicaId: Failed to convert from partitionId to string");

    KStringA::SPtr replicaIdString = KStringA::Create(GetThisAllocator(), MAX_TRACEID_LENGTH);
    result = replicaIdString->FromLONGLONG(replicaId);
    ASSERT_IFNOT(result == TRUE, "PartitionedReplicaId: Failed to convert from replicaId {0} to string", replicaId);

    result = traceId_->Concat(":");
    ASSERT_IFNOT(result == TRUE, "PartitionedReplicaId: Failed to concat1 to string");

    result = traceId_->Concat(*replicaIdString);
    ASSERT_IFNOT(result == TRUE, "PartitionedReplicaId: Failed to concat2 to string");

    result = traceId_->SetNullTerminator();
    ASSERT_IFNOT(result == TRUE, "PartitionedReplicaId: Failed to set null terminator");
}

PartitionedReplicaId::~PartitionedReplicaId()
{
}

PartitionedReplicaId::SPtr PartitionedReplicaId::Create(
    __in KGuid const & partitionId,
    __in ::FABRIC_REPLICA_ID replicaId,
    __in KAllocator & allocator)
{
    PartitionedReplicaId * pointer = _new(PARTITIONED_REPLICAID_TAG, allocator)PartitionedReplicaId(partitionId, replicaId);
    ASSERT_IF(pointer == nullptr, "PartitionedReplicaId: OOM while allocating");

    return PartitionedReplicaId::SPtr(pointer);
}

void PartitionedReplicaId::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
{
    w.Write("{0,08:x}-{1,04:x}-{2,04:x}-{3,02:x}{4,02:x}-",
        partitionId_.Data1, partitionId_.Data2, partitionId_.Data3, partitionId_.Data4[0], partitionId_.Data4[1]);
    w.Write("{0,02:x}{1,02:x}{2,02:x}{3,02:x}{4,02:x}{5,02:x}",
        partitionId_.Data4[2], partitionId_.Data4[3], partitionId_.Data4[4], 
        partitionId_.Data4[5], partitionId_.Data4[6], partitionId_.Data4[7]); 
}

std::string PartitionedReplicaId::AddField(Common::TraceEvent & traceEvent, std::string const & name)
{   
    traceEvent.AddField<int32>(name + ".data1");
    traceEvent.AddField<int16>(name + ".data2");
    traceEvent.AddField<int16>(name + ".data3");
    traceEvent.AddField<char>(name + ".data40");
    traceEvent.AddField<char>(name + ".data41");
    traceEvent.AddField<char>(name + ".data42");
    traceEvent.AddField<char>(name + ".data43");
    traceEvent.AddField<char>(name + ".data44");
    traceEvent.AddField<char>(name + ".data45");
    traceEvent.AddField<char>(name + ".data46");
    traceEvent.AddField<char>(name + ".data47");
    traceEvent.AddField<int64>(name + ".replicaId");    

    return "({0,08:x}-{1,04:x}-{2,04:x}-{3,02:x}{4,02:x}-{5,02:x}{6,02x}{7:02x}{8:02x}{9:02x}{10:02x}:{11})";
}

void PartitionedReplicaId::FillEventData(Common::TraceEventContext & context) const
{
    context.Write<int32>(static_cast<int32>(partitionId_.Data1));
    context.Write<int16>(static_cast<int16>(partitionId_.Data2));
    context.Write<int16>(static_cast<int16>(partitionId_.Data3));
    context.Write<char>(partitionId_.Data4[0]);
    context.Write<char>(partitionId_.Data4[1]);
    context.Write<char>(partitionId_.Data4[2]);
    context.Write<char>(partitionId_.Data4[3]);
    context.Write<char>(partitionId_.Data4[4]);
    context.Write<char>(partitionId_.Data4[5]);
    context.Write<char>(partitionId_.Data4[6]);
    context.Write<char>(partitionId_.Data4[7]);
    context.Write<int64>(static_cast<int64>(replicaId_));
}
