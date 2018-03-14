// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Data::TStore;

NTSTATUS KBufferComparer::Create(
    __in KAllocator & allocator,
    __out SPtr & result)
{
    result = _new(INT_COMPARER_TAG, allocator) KBufferComparer();

    if (!result)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    return STATUS_SUCCESS;
}

KBufferComparer::KBufferComparer()
{
}

KBufferComparer::~KBufferComparer()
{
}


int KBufferComparer::Compare(__in const KBuffer::SPtr& x, __in const KBuffer::SPtr& y) const
{
    return KBufferComparer::CompareBuffers(x, y);
}

int KBufferComparer::CompareBuffers(__in const KBuffer::SPtr& x, __in const KBuffer::SPtr& y)
{
    if (x == nullptr && y == nullptr)
    {
        return 0;
    }
    else if (x != nullptr && y == nullptr)
    {
        return 1;
    }
    else if (x == nullptr && y != nullptr)
    {
        return -1;
    }

    auto xBuffer = static_cast<byte *>(x->GetBuffer());
    auto yBuffer = static_cast<byte *>(y->GetBuffer());

    ULONG xSize = x->QuerySize();
    ULONG ySize = y->QuerySize();

    ULONG cmpSize = __min(xSize, ySize);
    auto comparison = memcmp(xBuffer, yBuffer, cmpSize);
    
    if (comparison != 0)
    {
        return comparison > 0 ? 1 : -1;
    }

    if (xSize < ySize)
    {
        return -1;
    }
     
    if (xSize > ySize)
    {
        return 1;
    }

    return 0;

}

bool KBufferComparer::Equals(__in KBuffer::SPtr & x, __in KBuffer::SPtr & y)
{
    return CompareBuffers(x, y) == 0;
}

ULONG KBufferComparer::Hash(__in KBuffer::SPtr const & key)
{
    return KChecksum::Crc32(key->GetBuffer(), key->QuerySize(), 0);
}

