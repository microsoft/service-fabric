// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data::TestCommon;

#define TESTSTATEFULPARTITION_TAG 'PFST'

TestComStatefulServicePartition::TestComStatefulServicePartition(__in KGuid partitionId)
{
    Common::Guid guid(partitionId);
    singletonPartitionInfo_.Id = guid.AsGUID();
    singletonPartitionInfo_.Reserved = NULL;
    partitionInfo_.Kind = FABRIC_SERVICE_PARTITION_KIND_SINGLETON;
    partitionInfo_.Value = &singletonPartitionInfo_;
    readStatus_ = FABRIC_SERVICE_PARTITION_ACCESS_STATUS_GRANTED;
    writeStatus_ = FABRIC_SERVICE_PARTITION_ACCESS_STATUS_GRANTED;
}

TestComStatefulServicePartition::~TestComStatefulServicePartition()
{
}

TestComStatefulServicePartition::SPtr TestComStatefulServicePartition::Create(__in KAllocator & allocator)
{
    KGuid guid;
    guid.CreateNew();
    return Create(guid, allocator);
}

TestComStatefulServicePartition::SPtr TestComStatefulServicePartition::Create(__in KGuid partitionId, __in KAllocator & allocator)
{
    TestComStatefulServicePartition * pointer = _new(TESTSTATEFULPARTITION_TAG, allocator)TestComStatefulServicePartition(partitionId);
    THROW_ON_ALLOCATION_FAILURE(pointer);

    return SPtr(pointer);
}

HRESULT TestComStatefulServicePartition::GetPartitionInfo(__out const FABRIC_SERVICE_PARTITION_INFORMATION** BufferedValue)
{
    *BufferedValue = &partitionInfo_;
    return STATUS_SUCCESS;
}

HRESULT TestComStatefulServicePartition::GetReadStatus(__out FABRIC_SERVICE_PARTITION_ACCESS_STATUS* ReadStatus)
{
    *ReadStatus = readStatus_;
    return STATUS_SUCCESS;
}

HRESULT TestComStatefulServicePartition::GetWriteStatus(__out FABRIC_SERVICE_PARTITION_ACCESS_STATUS* WriteStatus)
{
    *WriteStatus = writeStatus_;
    return STATUS_SUCCESS;
}

HRESULT TestComStatefulServicePartition::CreateReplicator(
    __in ::IFabricStateProvider *stateProvider,
    __in const FABRIC_REPLICATOR_SETTINGS* ReplicatorSettings,
    __out ::IFabricReplicator **replicator,
    __out ::IFabricStateReplicator **stateReplicator)
{
    UNREFERENCED_PARAMETER(stateProvider);
    UNREFERENCED_PARAMETER(ReplicatorSettings);
    UNREFERENCED_PARAMETER(replicator);
    UNREFERENCED_PARAMETER(stateReplicator);
    CODING_ERROR_ASSERT(false);
    return STATUS_SUCCESS;
}

HRESULT TestComStatefulServicePartition::ReportLoad(
    __in ULONG MetricCount,
    __in_ecount(MetricCount) const FABRIC_LOAD_METRIC* Metrics)
{
    UNREFERENCED_PARAMETER(MetricCount);
    UNREFERENCED_PARAMETER(Metrics);
    CODING_ERROR_ASSERT(false);
    return STATUS_SUCCESS;
}
        
HRESULT TestComStatefulServicePartition::ReportFault(__in FABRIC_FAULT_TYPE FaultType)
{
    UNREFERENCED_PARAMETER(FaultType);
    return STATUS_SUCCESS;
}

void TestComStatefulServicePartition::SetReadStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS readStatus)
{
    readStatus_ = readStatus;
}

void TestComStatefulServicePartition::SetWriteStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS writeStatus)
{
    writeStatus_ = writeStatus;
}

