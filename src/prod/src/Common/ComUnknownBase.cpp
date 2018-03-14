// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Common
{
    ComUnknownBase::ComUnknownBase()
        : refCount_(1), isShared_(false)
    {
    }

    ComUnknownBase::~ComUnknownBase()
    {
        ASSERT_IF(refCount_ != 0, "ComUnknownBase: Destructed with non-zero ref-count.");
    }

    ULONG ComUnknownBase::BaseAddRef(void)
    {
        isShared_ = true;
        ULONG result = InterlockedIncrement(&refCount_);
        ASSERT_IF(result == 1, "Cannot AddRef() on a released object");
        return result;
    }

    ULONG ComUnknownBase::BaseTryAddRef(void)
    {
        LONG current;
        LONG updated;

        do
        {
            current = refCount_;
            if (current == 0u)
            {
                return 0u;
            }
            updated = current + 1;
        }
        while (InterlockedCompareExchange(&refCount_, updated, current) != current);

        return updated;
    }

    ULONG ComUnknownBase::BaseRelease(void)
    {
        ULONG refCount;

        if (isShared_)
        {
            refCount = InterlockedDecrement(&refCount_);
        }
        else
        {
            refCount = --refCount_;
        }

        // Ensure we do not decrement a 0 ref count.
        const ULONG MinusOneULONG = static_cast<ULONG>(0) - static_cast<ULONG>(1);
        ASSERT_IF(refCount == MinusOneULONG, "ComUnknownBase: Too many calls to Release.");

        if (refCount == 0)
        {
            delete this;
        }

        return refCount;
    }
}
