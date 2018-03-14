// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TxnReplicator
{
    namespace TestCommon
    {
        // 
        // A Test Lock Context for releasing the lock on "Unlock"
        //
        class TestLockContext
            : public LockContext
        {
            K_FORCE_SHARED(TestLockContext)

        public:
            static SPtr Create(
                __in Data::Utilities::AsyncLock & lock,
                __in KAllocator & allocator);

            void Unlock() override;

        private:
            TestLockContext(__in Data::Utilities::AsyncLock & lock);

            Data::Utilities::AsyncLock::SPtr lock_;
        };
    }
}
