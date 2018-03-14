// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace LoggingReplicatorTests
{
    /// <summary>
    /// Collection of named operation data
    /// </summary>
    class TestTransactionChangeHandler final
        : public KObject<TestTransactionChangeHandler>
        , public KShared<TestTransactionChangeHandler>
        , public TxnReplicator::ITransactionChangeHandler
    {
        K_FORCE_SHARED(TestTransactionChangeHandler)
        K_SHARED_INTERFACE_IMP(ITransactionChangeHandler)

    public:
        static SPtr Create(__in KAllocator & allocator);

    public: // Test APIs.
        __declspec(property(get = get_CommitCount)) ULONG CommitCount;
        ULONG get_CommitCount() const { return static_cast<ULONG>(commitNotificationCount_); }

    public: // ITransactionChangeHandler APIs.
        void OnTransactionCommitted(
            __in TxnReplicator::ITransactionalReplicator & source,
            __in TxnReplicator::ITransaction const & transaction) override;

    private:
        volatile LONG commitNotificationCount_ = 0;
    };
}
