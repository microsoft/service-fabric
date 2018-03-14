// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TxnReplicator
{
    //
    // The Com Proxy that converts the com based v1 replicator operation stream to a ktl::awaitable operation stream
    //
    class ComProxyOperationStream final
        : public KObject<ComProxyOperationStream>
        , public KShared<ComProxyOperationStream>
        , public Data::LoggingReplicator::IOperationStream
        , public Data::Utilities::PartitionedReplicaTraceComponent<Common::TraceTaskCodes::TR>
    {
        K_FORCE_SHARED(ComProxyOperationStream)
        K_SHARED_INTERFACE_IMP(IOperationStream)

    public:

        static NTSTATUS Create(
            __in Data::Utilities::PartitionedReplicaId const & traceId,
            __in Common::ComPointer<IFabricOperationStream> comOperationStream,
            __in KAllocator & allocator,
            __out ComProxyOperationStream::SPtr & comProxyOperationStream);

        ktl::Awaitable<NTSTATUS> GetOperationAsync(__out Data::LoggingReplicator::IOperation::SPtr & result) override;

        void Test_SetTestHookContext(TestHooks::TestHookContext const &);

    private:

        ComProxyOperationStream(
            __in Data::Utilities::PartitionedReplicaId const & traceId,
            __in Common::ComPointer<IFabricOperationStream> comOperationStream);

        class AsyncGetOperationContextBase : 
            public Ktl::Com::FabricAsyncContextBase
        {
        };
 
        //
        // GetOperation async 
        //
        class AsyncGetOperationContext
            : public Ktl::Com::AsyncCallOutAdapter<AsyncGetOperationContextBase>
        {
            friend ComProxyOperationStream;

            K_FORCE_SHARED(AsyncGetOperationContext)

        public:

            ktl::Awaitable<NTSTATUS> GetNextOperationAsync(__out ComProxyOperation::SPtr & operation);
            
            ComProxyOperation::SPtr GetResult();

        protected:

            HRESULT OnEnd(__in IFabricAsyncOperationContext & context);

        private:
            
            ComProxyOperationStream::SPtr parent_;
            ComProxyOperation::SPtr result_;
            ktl::AwaitableCompletionSource<NTSTATUS>::SPtr tcs_;
        };

        Common::ComPointer<IFabricOperationStream> comOperationStream_;

        TestHooks::TestHookContext testHookContext_;
    };
}
