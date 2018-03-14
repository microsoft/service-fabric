// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace LoggingReplicator
    {
        //
        // State Provider proxy class that the V1 replicator can use to talk to the logging replicator
        // ComStateProvider is what the V1 replicator talks to, which in turn converts the com API's to the ktl based API's here
        //
        class StateProvider final
            : public IStateProvider
            , public KObject<StateProvider>
            , public KShared<StateProvider>
            , public Utilities::PartitionedReplicaTraceComponent<Common::TraceTaskCodes::LR>
        {
            K_FORCE_SHARED(StateProvider)
            K_SHARED_INTERFACE_IMP(IStateProvider)
            K_SHARED_INTERFACE_IMP(IDisposable)

        public:

            //
            // NOTE: Cyclic reference possibility
            //
            // V1 replicator has a reference to ComStateProvider to talk to it.
            // The ComStateProvider needs to keep a reference to this class to convert com api's to ktl api's
            // This class must reference the loggingreplicatorimpl to do useful work
            // loggingreplicatorimpl needs to reference v1replicator back again to replicate operations
            //
            // This forms a cycle and leads to leaks
            //
            // The way the cycle is broken is via the Dispose method that is invoked in the destructor of the TransactionalReplicator as this class is not part of the chain
            // and is kept alive by the partition keeping the ComTransactionalReplicator object alive
            //
            // ComTransactionalReplicator.Test.cpp test case for create and delete found this bug
            // 
            static IStateProvider::SPtr Create(
                __in LoggingReplicatorImpl & loggingReplicator,
                __in KAllocator & allocator);
            
            void Dispose() override;

            HRESULT CreateAsyncOnDataLossContext(__out AsyncOnDataLossContext::SPtr & asyncContext) override;

            HRESULT CreateAsyncUpdateEpochContext(__out AsyncUpdateEpochContext::SPtr & asyncContext) override;

            HRESULT GetCopyContext(__out TxnReplicator::IOperationDataStream::SPtr & copyContextStream) override;

            HRESULT GetCopyState(
                __in FABRIC_SEQUENCE_NUMBER uptoSequenceNumber,
                __in TxnReplicator::OperationDataStream & copyContextStream,
                __out TxnReplicator::IOperationDataStream::SPtr & copyStateStream) override;

            HRESULT GetLastCommittedSequenceNumber(FABRIC_SEQUENCE_NUMBER * lsn) override;

            void Test_SetTestHookContext(TestHooks::TestHookContext const &) override;

        private:

            StateProvider(__in LoggingReplicatorImpl & loggingReplicator);

            ktl::Awaitable<NTSTATUS> OnDataLossAsync(__out bool & result);

            ktl::Awaitable<NTSTATUS> UpdateEpochAsync(
                __in FABRIC_EPOCH const * epoch,
                __in LONG64 lastLsnInPreviousEpoch);

            HRESULT Test_GetInjectedFault(std::wstring const & tag);

            class AsyncUpdateEpochContextImpl 
                : public AsyncUpdateEpochContext
            {
                friend StateProvider;

                K_FORCE_SHARED(AsyncUpdateEpochContextImpl)

            public:

                HRESULT StartUpdateEpoch(
                    __in FABRIC_EPOCH const & epoch,
                    __in FABRIC_SEQUENCE_NUMBER previousEpochLastSequenceNumber,
                    __in_opt KAsyncContextBase * const parentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback callback) override;

            protected:

                void OnStart();

            private:

                ktl::Task DoWork();

                StateProvider::SPtr parent_;

                FABRIC_EPOCH epoch_;
                FABRIC_SEQUENCE_NUMBER previousEpochLastSequenceNumber_;
            };


            class AsyncOnDataLossContextImpl 
                : public AsyncOnDataLossContext
            {
                friend StateProvider;

                K_FORCE_SHARED(AsyncOnDataLossContextImpl)

            public:

                HRESULT StartOnDataLoss(
                    __in_opt KAsyncContextBase * const parentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback callback)override;

                HRESULT GetResult(
                    __out BOOLEAN & isStateChanged)override;

            protected:

                void OnStart();

            private:

                ktl::Task DoWork();

                StateProvider::SPtr parent_;
                BOOLEAN isStateChanged_;
            };

            LoggingReplicatorImpl::SPtr loggingReplicatorStateProvider_;

            // Used for fault injection from FabricTest
            //
            TestHooks::TestHookContext testHookContext_;
        };
    }
}
