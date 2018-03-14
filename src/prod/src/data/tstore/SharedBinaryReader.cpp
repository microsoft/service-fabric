// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#define SharedBinaryReader_TAG 'twBS'

using namespace Data::TStore;
using namespace Data::Utilities;

SharedBinaryReader::SharedBinaryReader(__in KBuffer& buffer)
    :BinaryReader(buffer, GetThisAllocator())
{
}

SharedBinaryReader::~SharedBinaryReader()
{
}

NTSTATUS SharedBinaryReader::Create(
    __in KAllocator& allocator,
    __in KBuffer& buffer,
    __out SharedBinaryReader::SPtr& result)
{
    NTSTATUS status;
    SPtr output = _new(SharedBinaryReader_TAG, allocator) SharedBinaryReader(buffer);

    if (!output)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = output->Status();
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    result = Ktl::Move(output);
    return STATUS_SUCCESS;
}
