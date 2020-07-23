// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace RobustTXRService
{
    class RobustTestRunner
        : public NightWatchTXRService::TestRunner
    {
        K_FORCE_SHARED_WITH_INHERITANCE(RobustTestRunner);
        K_SHARED_INTERFACE_IMP(ITestRunner);

    public:
        static RobustTestRunner::SPtr Create(
            __in Common::ByteBufferUPtr const & testParametersJson,
            __in TxnReplicator::ITransactionalReplicator & txnReplicator,
            __in KAllocator & allocator);

        ktl::Awaitable<NightWatchTXRService::PerfResult::SPtr> Run() override;

    protected:
        RobustTestRunner(
            __in Common::ByteBufferUPtr const & testParametersJson,
            __in TxnReplicator::ITransactionalReplicator & txnReplicator);

        virtual ktl::Awaitable<void> DoOperation() override;

        TestParameters testParameters_;

    private:
        Common::Random random_;

        double getRandomNumber();
        int getRandomNumber(int max);
    };
}