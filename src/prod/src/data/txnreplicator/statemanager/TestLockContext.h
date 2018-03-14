// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace StateManagerTests
{
    // 
    // A Test Lock Context for releasing the lock on "Unlock"
    //
    class TestLockContext
        : public TxnReplicator::LockContext
    {
        K_FORCE_SHARED(TestLockContext)

    public:
        static SPtr Create(
            __in Data::Utilities::ReaderWriterAsyncLock & lock,
            __in KAllocator & allocator);

        void Unlock() override;

    private:
        TestLockContext(__in Data::Utilities::ReaderWriterAsyncLock & lock);

        Data::Utilities::ReaderWriterAsyncLock::SPtr lock_;
    };
}
