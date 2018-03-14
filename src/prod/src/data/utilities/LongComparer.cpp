// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Data;
using namespace Data::Utilities;

LongComparer::LongComparer()
{
}

LongComparer::~LongComparer()
{
}

NTSTATUS LongComparer::Create(
    __in KAllocator & allocator,
    __out LongComparer::SPtr & result)
{
    result = _new(INT_COMPARER_TAG, allocator) LongComparer();

    if (!result)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    return STATUS_SUCCESS;
}

int LongComparer::Compare(__in const LONG64& x, __in const LONG64& y) const
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
