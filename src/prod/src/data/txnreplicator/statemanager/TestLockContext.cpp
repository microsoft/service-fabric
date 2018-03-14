// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data::Utilities;
using namespace StateManagerTests;

TestLockContext::SPtr TestLockContext::Create(
    __in ReaderWriterAsyncLock & lock,
    __in KAllocator & allocator)
{
    TestLockContext * pointer = _new(TEST_LOCKCONTEXT, allocator) TestLockContext(lock);
    THROW_ON_ALLOCATION_FAILURE(pointer);

    return SPtr(pointer);
}

void TestLockContext::Unlock()
{
    lock_->ReleaseWriteLock();
}

TestLockContext::TestLockContext(__in ReaderWriterAsyncLock & lock)
    : lock_(&lock)
{
}

TestLockContext::~TestLockContext()
{
}
