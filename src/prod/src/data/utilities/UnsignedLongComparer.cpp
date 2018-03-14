// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Data;
using namespace Data::Utilities;

UnsignedLongComparer::UnsignedLongComparer()
{
}

UnsignedLongComparer::~UnsignedLongComparer()
{
}

NTSTATUS UnsignedLongComparer::Create(
    __in KAllocator & allocator,
    __out UnsignedLongComparer::SPtr & result)
{
    result = _new(INT_COMPARER_TAG, allocator) UnsignedLongComparer();

    if (!result)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    return STATUS_SUCCESS;
}

int UnsignedLongComparer::Compare(__in const ULONG64& x, __in const ULONG64& y) const
{
    if (x < y)
    {
        return -1;
    }
    else if (x > y)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}
