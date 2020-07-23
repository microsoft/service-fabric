// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace NightWatchTXRService
{
    class TestRunnerBase
        : public ITestRunner
        , public KObject<TestRunnerBase>
        , public KShared<TestRunnerBase>
    {
        K_FORCE_SHARED_WITH_INHERITANCE(TestRunnerBase);
        K_SHARED_INTERFACE_IMP(ITestRunner);

    public:
        ktl::Awaitable<PerfResult::SPtr> Run(
            __in ULONG maxOutstandingOperations,
            __in ULONG numberOfOperations);

    protected:
        TestRunnerBase(
            __in TxnReplicator::ITransactionalReplicator & txnReplicator);

        virtual ktl::Task DoWork();
        virtual ktl::Awaitable<void> DoOperation() = 0;

        ktl::Task CheckTestCompletion();

        bool ShouldStop()
        {
            //if operation count is reached, stop processing
            ULONG operationsCount = (ULONG)operationsCount_.load();
            if (operationsCount > numberOfOperations_)
                return true;

            if (ExceptionThrown)
                return true;

            return false;
        }

        void IncOperation(int count)
        {
            operationsCount_ += count;
        }

    protected:
        TxnReplicator::ITransactionalReplicator::SPtr txnReplicator_;

        ktl::AwaitableCompletionSource<void>::SPtr testCompletionTcs_;
        Common::atomic_long runningThreadsCount_;
        Common::atomic_long operationsCount_;
    private:
        ULONG maxOutstandingOperations_;
        ULONG numberOfOperations_;
    };

    class TestRunner
        : public TestRunnerBase
    {
        K_FORCE_SHARED_WITH_INHERITANCE(TestRunner);
        K_SHARED_INTERFACE_IMP(ITestRunner);

    public:
        static TestRunner::SPtr Create(
            __in Common::ByteBufferUPtr const & testParametersJson,
            __in TxnReplicator::ITransactionalReplicator & txnReplicator,
            __in KAllocator & allocator);

        virtual ktl::Awaitable<PerfResult::SPtr> Run() override;

    protected:
        TestRunner(
            __in Common::ByteBufferUPtr const & testParametersJson,
            __in TxnReplicator::ITransactionalReplicator & txnReplicator);

        TestRunner(
            __in TxnReplicator::ITransactionalReplicator & txnReplicator);

        virtual ktl::Awaitable<void> DoOperation() override;
        virtual ktl::Awaitable<LONG64> AddTx(__in std::wstring const & key);
        virtual ktl::Awaitable<LONG64> UpdateTx(__in std::wstring const & key, __in LONG64 version);

    private:
        NightWatchTestParameters testParameters_;
    };
}