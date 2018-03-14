// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Common;
using namespace Data::Utilities;
using namespace Data::LoggingReplicator;
using namespace TxnReplicator;

ComProxyOperation::ComProxyOperation(__in ComPointer<IFabricOperation> comOperation)
    : KObject()
    , KShared()
    , comOperation_(comOperation)
{
}

ComProxyOperation::~ComProxyOperation()
{
}

NTSTATUS ComProxyOperation::Create(
    __in ComPointer<IFabricOperation> comOperation,
    __in KAllocator & allocator,
    __out ComProxyOperation::SPtr & comProxyOperation)
{
    ComProxyOperation * pointer = _new(TRANSACTIONALREPLICATOR_TAG, allocator) ComProxyOperation(comOperation);

    if (pointer == nullptr)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    comProxyOperation = pointer;

    return STATUS_SUCCESS;
}

void ComProxyOperation::Acknowledge()
{
    // Intentionally ignore error codes as it implies replica is closing
    comOperation_->Acknowledge();
}

FABRIC_OPERATION_TYPE ComProxyOperation::get_OperationType() const
{
    return comOperation_->get_Metadata()->Type;
}

LONG64 ComProxyOperation::get_SequenceNumber() const
{
    return comOperation_->get_Metadata()->SequenceNumber;
}

LONG64 ComProxyOperation::get_AtomicGroupId() const
{
    ASSERT_IFNOT(false, "get_AtomicGroupId is not supposed to be called.");
    return 0;
}

OperationData::CSPtr ComProxyOperation::get_Data() const
{
    ULONG count;
    FABRIC_OPERATION_DATA_BUFFER const * buffers;

    HRESULT hr = comOperation_->GetData(&count, &buffers);

    ASSERT_IFNOT(SUCCEEDED(hr), "Failed to GetData");

    OperationData::CSPtr operationData;
    NTSTATUS status = ComProxyOperationData::Create(
        count,
        buffers,
        GetThisAllocator(),
        operationData);

    ASSERT_IFNOT(NT_SUCCESS(status), "Failed to Create Proxy Operation Data");

    return operationData;
}
