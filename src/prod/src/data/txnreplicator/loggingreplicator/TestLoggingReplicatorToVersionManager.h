// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace LoggingReplicatorTests
{
    class TestLoggingReplicatorToVersionManager final
        : public TxnReplicator::ILoggingReplicatorToVersionManager
        , public KObject<TestLoggingReplicatorToVersionManager>
        , public KShared<TestLoggingReplicatorToVersionManager>
    {
        K_FORCE_SHARED(TestLoggingReplicatorToVersionManager)
        K_SHARED_INTERFACE_IMP(ILoggingReplicatorToVersionManager)

    public:

        static TestLoggingReplicatorToVersionManager::SPtr Create(__in KAllocator & allocator);

        void UpdateDispatchingBarrierTask(__in TxnReplicator::CompletionTask & barrierTask) override;
    };
}
