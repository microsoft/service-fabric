// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Data;
using namespace Data::Utilities;

KStringComparer::KStringComparer()
{
}

KStringComparer::~KStringComparer()
{
}

NTSTATUS KStringComparer::Create(
    __in KAllocator & allocator,
    __out KStringComparer::SPtr & result)
{
    result = _new(SHARED_STRING_COMPARER_TAG, allocator) KStringComparer();

    if (!result)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    return STATUS_SUCCESS;
}

int KStringComparer::Compare(__in const KString::SPtr& x, __in const KString::SPtr& y) const
{
    if (x == nullptr)
    {
        if (y != nullptr)
        {
            return -1;
        }

        return 0;
    }
    else if (y == nullptr)
    {
        return 1;
    }

    return x->Compare(*y);
}
