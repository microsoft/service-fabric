// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Data;
using namespace TStoreTests;

SharedLong::SPtr SharedLong::Create(
    __in LONG64 value,
    __in KAllocator & allocator)
{
    SharedLong * result = _new(SHARED_LONG_TAG, allocator) SharedLong(value);
    return result;
}

SharedLong::SharedLong(__in LONG64 value) : value_(value)
{
}

SharedLong::~SharedLong()
{
}

IComparer<SharedLong::SPtr>::SPtr SharedLongComparer::Create(__in KAllocator & allocator)
{
    IComparer<SharedLong::SPtr>* result = _new(SHARED_LONG_TAG, allocator) SharedLongComparer();
    return result;
}

SharedLongComparer::SharedLongComparer()
{
}

SharedLongComparer::~SharedLongComparer()
{
}

int SharedLongComparer::Compare(
    __in const SharedLong::SPtr & x, 
    __in const SharedLong::SPtr & y) const
{
    return static_cast<int>(x->Value - y->Value);
}
