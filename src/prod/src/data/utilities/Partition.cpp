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
    __out IStatefulPartition::SPtr & result)
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
    NTSTATUS status = StatusConverter::Convert(hr);

    return status;
}

NTSTATUS Partition::GetReadStatus(__out FABRIC_SERVICE_PARTITION_ACCESS_STATUS * readStatus)
{
    HRESULT hr = fabricServicePartition_->GetReadStatus(readStatus);
    NTSTATUS status = StatusConverter::Convert(hr);

    return status;
}

NTSTATUS Partition::GetWriteStatus(__out FABRIC_SERVICE_PARTITION_ACCESS_STATUS * writeStatus)
{
    HRESULT hr = fabricServicePartition_->GetWriteStatus(writeStatus);
    NTSTATUS status = StatusConverter::Convert(hr);

    return status;
}

NTSTATUS Partition::GetPartitionInfo(__out const FABRIC_SERVICE_PARTITION_INFORMATION** BufferedValue)
{
    HRESULT hr = fabricServicePartition_->GetPartitionInfo(BufferedValue);
    NTSTATUS status = StatusConverter::Convert(hr);

    return status;
}
