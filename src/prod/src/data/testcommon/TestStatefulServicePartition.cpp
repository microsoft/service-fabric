// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data::TestCommon;

#define TESTSTATEFULPARTITION_TAG 'PFST'

TestStatefulServicePartition::TestStatefulServicePartition(__in KGuid partitionId)
{
    Common::Guid guid(partitionId);
    singletonPartitionInfo_.Id = guid.AsGUID();
    singletonPartitionInfo_.Reserved = NULL;
    partitionInfo_.Kind = FABRIC_SERVICE_PARTITION_KIND_SINGLETON;
    partitionInfo_.Value = &singletonPartitionInfo_;
    readStatus_ = FABRIC_SERVICE_PARTITION_ACCESS_STATUS_GRANTED;
    writeStatus_ = FABRIC_SERVICE_PARTITION_ACCESS_STATUS_GRANTED;
}

TestStatefulServicePartition::~TestStatefulServicePartition()
{
}

TestStatefulServicePartition::SPtr TestStatefulServicePartition::Create(__in KAllocator & allocator)
{
    KGuid guid;
    guid.CreateNew();
    return Create(guid, allocator);
}

TestStatefulServicePartition::SPtr TestStatefulServicePartition::Create(__in KGuid partitionId, __in KAllocator & allocator)
{
    TestStatefulServicePartition * pointer = _new(TESTSTATEFULPARTITION_TAG, allocator)TestStatefulServicePartition(partitionId);
    THROW_ON_ALLOCATION_FAILURE(pointer);

    return SPtr(pointer);
}

NTSTATUS TestStatefulServicePartition::GetPartitionInfo(__out const FABRIC_SERVICE_PARTITION_INFORMATION** BufferedValue)
{
    *BufferedValue = &partitionInfo_;
    return STATUS_SUCCESS;
}

NTSTATUS TestStatefulServicePartition::GetReadStatus(__out FABRIC_SERVICE_PARTITION_ACCESS_STATUS* ReadStatus)
{
    *ReadStatus = readStatus_;
    return STATUS_SUCCESS;
}

NTSTATUS TestStatefulServicePartition::GetWriteStatus(__out FABRIC_SERVICE_PARTITION_ACCESS_STATUS* WriteStatus)
{
    *WriteStatus = writeStatus_;
    return STATUS_SUCCESS;
}

NTSTATUS TestStatefulServicePartition::ReportFault(__in FABRIC_FAULT_TYPE FaultType)
{
    UNREFERENCED_PARAMETER(FaultType);
    return STATUS_SUCCESS;
}

void TestStatefulServicePartition::SetReadStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS readStatus)
{
    readStatus_ = readStatus;
}

void TestStatefulServicePartition::SetWriteStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS writeStatus)
{
    writeStatus_ = writeStatus;
}
