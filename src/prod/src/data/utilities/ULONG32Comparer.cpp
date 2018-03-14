// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Data;
using namespace Data::Utilities;

ULONG32Comparer::ULONG32Comparer()
{
}

ULONG32Comparer::~ULONG32Comparer()
{
}

NTSTATUS ULONG32Comparer::Create(
    __in KAllocator & allocator,
    __out ULONG32Comparer::SPtr & result)
{
    result = _new(INT_COMPARER_TAG, allocator) ULONG32Comparer();

    if (!result)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    return STATUS_SUCCESS;
}

int ULONG32Comparer::Compare(__in const ULONG32& x, __in const ULONG32& y) const
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

ULONG ULONG32Comparer::HashFunction(const ULONG32 & key)
{
    return key;
}
