// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Data;
using namespace Data::Utilities;

NTSTATUS LockModeComparer::Create(
    __in KAllocator & allocator,
    __out IComparer<LockMode::Enum>::SPtr & result)
{
    result = _new(LOCK_MODE_TAG, allocator) LockModeComparer();

    if (!result)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    return STATUS_SUCCESS;
}

int LockModeComparer::Compare(__in const LockMode::Enum& x, __in const LockMode::Enum& y) const
{
    return static_cast<ULONG32>(x) - static_cast<ULONG32>(y);
}

LockModeComparer::LockModeComparer()
{
}

LockModeComparer::~LockModeComparer()
{
}
