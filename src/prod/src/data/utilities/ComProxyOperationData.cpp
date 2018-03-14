// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data;
using namespace Data::Utilities;

NTSTATUS ComProxyOperationData::Create(
    __in ULONG count,
    __in FABRIC_OPERATION_DATA_BUFFER const * const buffers,
    __in KAllocator & allocator,
    __out OperationData::CSPtr & result)
{
    if (!buffers)
    {
        return STATUS_ACCESS_VIOLATION;
    }

    NTSTATUS status = STATUS_SUCCESS;
    KArray<KBuffer::CSPtr> bufferArray(allocator);

    if (!NT_SUCCESS(bufferArray.Status()))
    {
        return bufferArray.Status();
    }

    for (ULONG i = 0; i < count; i++)
    {
        KBuffer::SPtr buffer;
        status = KBuffer::Create(
            buffers[i].BufferSize,
            buffer,
            allocator,
            OPERATIONDATA_TAG);

        if (!NT_SUCCESS(status))
        {
            return status;
        }

        errno_t err = memcpy_s(
            buffer->GetBuffer(),
            buffer->QuerySize(),
            buffers[i].Buffer,
            buffers[i].BufferSize);

        ASSERT_IF(err != 0, "Failed to memcpy_s with error {0} while creating ComProxyOperationData", err);

        status = bufferArray.Append(buffer.RawPtr());

        if (!NT_SUCCESS(status))
        {
            return status;
        }
    }

    result = OperationData::Create(bufferArray, allocator);

    return status;
}
