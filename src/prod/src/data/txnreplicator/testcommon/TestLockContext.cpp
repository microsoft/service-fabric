// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data::Utilities;
using namespace TxnReplicator;
using namespace Data::StateManager;
using namespace TestCommon;

TestLockContext::SPtr TestLockContext::Create(
    __in AsyncLock & lock,
    __in KAllocator & allocator)
{
    TestLockContext * pointer = _new(TEST_Lock_Context_TAG, allocator) TestLockContext(lock);
    THROW_ON_ALLOCATION_FAILURE(pointer);

    return SPtr(pointer);
}

void TestLockContext::Unlock()
{
    lock_->ReleaseLock();
}

TestLockContext::TestLockContext(__in AsyncLock & lock)
    : lock_(&lock)
{
}

TestLockContext::~TestLockContext()
{
}
