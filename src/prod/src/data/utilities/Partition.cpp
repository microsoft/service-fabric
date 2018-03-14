// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data::Utilities;

Partition::Partition(__in IFabricStatefulServicePartition & partition)
    : fabricServicePartition_()
{
    fabricServicePartition_.SetAndAddRef(&partition);
}

Partition::~Partition()
{
}

NTSTATUS Partition::Create(
    __in IFabricStatefulServicePartition & fabricPartition,
    __in KAllocator& allocator, 
    __out KWfStatefulServicePartition::SPtr & result)
{
    NTSTATUS status;
    SPtr output = _new(STATEFUL_SERVICE_PARTITION_TAG, allocator) Partition(fabricPartition);

    if (!output)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        return status;
    }

    status = output->Status();
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    result = output.RawPtr();
    return STATUS_SUCCESS;
}

NTSTATUS Partition::ReportFault(__in FABRIC_FAULT_TYPE faultType)
{
    HRESULT hr = fabricServicePartition_->ReportFault(faultType);

    if(SUCCEEDED(hr))
    {
        return STATUS_SUCCESS;
    }

    // TODO: Improved conversion.
    return ERROR_SEVERITY_ERROR;
}

NTSTATUS Partition::GetReadStatus(__out FABRIC_SERVICE_PARTITION_ACCESS_STATUS * readStatus)
{
    HRESULT hr = fabricServicePartition_->GetReadStatus(readStatus);
    if (SUCCEEDED(hr))
    {
        return STATUS_SUCCESS;
    }

    // TODO: Improved conversion.
    return ERROR_SEVERITY_ERROR;
}

NTSTATUS Partition::GetWriteStatus(__out FABRIC_SERVICE_PARTITION_ACCESS_STATUS * writeStatus)
{
    HRESULT hr = fabricServicePartition_->GetWriteStatus(writeStatus);
    if (SUCCEEDED(hr))
    {
        return STATUS_SUCCESS;
    }

    // TODO: Improved conversion.
    return ERROR_SEVERITY_ERROR;
}

NTSTATUS Partition::GetPartitionInfo(__out const FABRIC_SERVICE_PARTITION_INFORMATION** BufferedValue)
{
    HRESULT hr = fabricServicePartition_->GetPartitionInfo(BufferedValue);

    if (SUCCEEDED(hr))
    {
        return STATUS_SUCCESS;
    }

    // TODO: Improved conversion.
    return ERROR_SEVERITY_ERROR;
}

NTSTATUS Partition::CreateReplicator(
    __in KWfStateProvider* StateProvider,
    __in const FABRIC_REPLICATOR_SETTINGS* ReplicatorSettings,
    __out KSharedPtr<KWfReplicator>& Replicator,
    __out KSharedPtr<KWfStateReplicator>& StateReplicator)
{
    UNREFERENCED_PARAMETER(StateProvider);
    UNREFERENCED_PARAMETER(ReplicatorSettings);
    UNREFERENCED_PARAMETER(Replicator);
    UNREFERENCED_PARAMETER(StateReplicator);
    ASSERT_IF(true, "Partition: CreateReplicator should not be invoked");
    return STATUS_SUCCESS;
}

NTSTATUS Partition::ReportLoad(
    __in ULONG MetricCount,
    __in_ecount(MetricCount) const FABRIC_LOAD_METRIC* Metrics)
{
    UNREFERENCED_PARAMETER(MetricCount);
    UNREFERENCED_PARAMETER(Metrics);
    ASSERT_IF(true, "Partition: ReportLoad should not be invoked");
    return STATUS_SUCCESS;
}

