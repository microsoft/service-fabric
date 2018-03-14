// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TxnReplicator
{ 
    //
    // The loggingreplicator uses the V1 replicator to replicate state and this proxyclass converts the KTL based awaitables to com based API's of the V1 replicator
    //
    class ComProxyStateReplicator final
        : public KObject<ComProxyStateReplicator>
        , public KShared<ComProxyStateReplicator>
        , public Data::LoggingReplicator::IStateReplicator
        , public Data::Utilities::PartitionedReplicaTraceComponent<Common::TraceTaskCodes::TR>
    {
        K_FORCE_SHARED(ComProxyStateReplicator)
        K_SHARED_INTERFACE_IMP(IStateReplicator)

    public:
         
        static NTSTATUS Create(
            __in Data::Utilities::PartitionedReplicaId const & traceId,
            __in Common::ComPointer<IFabricStateReplicator> & v1StateReplicator,
            __in KAllocator & allocator,
            __out ComProxyStateReplicator::SPtr & comProxyStateReplicator);
         
        CompletionTask::SPtr ReplicateAsync(
            __in Data::Utilities::OperationData const & replicationData,
            __out LONG64 & logicalSequenceNumber) override;

        NTSTATUS GetCopyStream(__out Data::LoggingReplicator::IOperationStream::SPtr & result) override;
        
        NTSTATUS GetReplicationStream(__out Data::LoggingReplicator::IOperationStream::SPtr & result) override;
        
        int64 GetMaxReplicationMessageSize() override;

        void Test_SetTestHookContext(TestHooks::TestHookContext const &);

    private:

        ComProxyStateReplicator(
            __in Data::Utilities::PartitionedReplicaId const & traceId,
            __in Common::ComPointer<IFabricStateReplicator> & v1StateReplicator);

        class AsyncReplicateContextBase : 
            public Ktl::Com::FabricAsyncContextBase
        {
        };
 
        //
        // Replicate async operation
        //
        class AsyncReplicateContext
            : public Ktl::Com::AsyncCallOutAdapter<AsyncReplicateContextBase>
        {
            friend ComProxyStateReplicator;

            K_FORCE_SHARED(AsyncReplicateContext)

        public:

            CompletionTask::SPtr ReplicateAsync(
                Common::ComPointer<IFabricOperationData> comOperationData,
                __out LONG64 & logicalSequenceNumber);

            FABRIC_SEQUENCE_NUMBER GetResultLsn() const;

        protected:

            HRESULT OnEnd(__in IFabricAsyncOperationContext & context);

        private:

            FABRIC_SEQUENCE_NUMBER resultSequenceNumber_;
            ComProxyStateReplicator::SPtr parent_;
            CompletionTask::SPtr task_;
        };

        NTSTATUS GetOperationStream(
            __in bool isCopyStream,
            __out Data::LoggingReplicator::IOperationStream::SPtr & result);

        Common::ComPointer<IFabricStateReplicator> comStateReplicator_;

        TestHooks::TestHookContext testHookContext_;
    };
}
