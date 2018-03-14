// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data;
using namespace Data::Utilities;

ComOperationData::ComOperationData(__in OperationData const & innerOperationData)
    : KObject()
    , KShared()
    , innerOperationData_(&innerOperationData)
    , buffersArray_(GetThisAllocator())
{
    ASSERT_IFNOT(innerOperationData_ != nullptr, "Null operation data");
    ASSERT_IFNOT(innerOperationData_->BufferCount > 0, "Buffer count is zero");

    NTSTATUS status = STATUS_SUCCESS;

    if (!NT_SUCCESS(buffersArray_.Status()))
    {
        SetConstructorStatus(status);
        return;
    }

    for (ULONG32 i = 0; i < innerOperationData_->BufferCount; i++)
    {
        FABRIC_OPERATION_DATA_BUFFER buffer;
        buffer.BufferSize = (*innerOperationData_)[i]->QuerySize();
        buffer.Buffer = (BYTE*)((*innerOperationData_)[i]->GetBuffer());

        status = buffersArray_.Append(buffer);

        if (!NT_SUCCESS(status))
        {
            SetConstructorStatus(status);
        }
    }
}

ComOperationData::~ComOperationData()
{
}

NTSTATUS ComOperationData::Create(
    __in OperationData const & innerOperationData,
    __in KAllocator & allocator,
    __out Common::ComPointer<IFabricOperationData> & object)
{
    ComOperationData * pointer = _new(OPERATIONDATA_TAG, allocator) ComOperationData(innerOperationData);

    if (pointer == nullptr)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (!NT_SUCCESS(pointer->Status()))
    {
        return pointer->Status();
    }

    object.SetAndAddRef(pointer);
    return STATUS_SUCCESS;
}

HRESULT ComOperationData::GetData(
    __out ULONG *count,
    __out const FABRIC_OPERATION_DATA_BUFFER **buffers)
{
    if (!NT_SUCCESS(Status()))
    {
        return E_FAIL; // tODO: Convert NTSTATUS to HRESULT
    }

    *count = buffersArray_.Count();
    *buffers = buffersArray_.Data();

    return S_OK;
}
