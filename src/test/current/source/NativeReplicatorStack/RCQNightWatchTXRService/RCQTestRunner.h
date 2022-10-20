//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

#pragma once

namespace RCQNightWatchTXRService
{
    class RCQTestRunner
        : public NightWatchTXRService::TestRunner
    {
        //K_FORCE_SHARED(RCQTestRunner)
        K_FORCE_SHARED_WITH_INHERITANCE(RCQTestRunner)
        K_SHARED_INTERFACE_IMP(ITestRunner);

    public:
        static RCQTestRunner::SPtr Create(
            __in Common::ByteBufferUPtr const & testParametersJson,
            __in TxnReplicator::ITransactionalReplicator & txnReplicator,
            __in KAllocator & allocator);

        //ktl::Awaitable<NightWatchTXRService::PerfResult::SPtr> Run() override;

    protected:
        virtual ktl::Awaitable<void> DoOperation() override;

    private:
        RCQTestRunner(
            __in Common::ByteBufferUPtr const & testParametersJson,
            __in TxnReplicator::ITransactionalReplicator & txnReplicator);

        KSharedPtr<Data::Collections::IReliableConcurrentQueue<KBuffer::SPtr>> rcqSPtr_;
        NightWatchTXRService::NightWatchTestParameters testParameters_;
    };
}