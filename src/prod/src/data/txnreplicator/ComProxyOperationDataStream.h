// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TxnReplicator
{
    //
    // The Com Proxy that converts the com based v1 replicator operation data stream to a ktl::awaitable operation data stream
    //
    class ComProxyOperationDataStream final
        : public OperationDataStream
        , public Data::Utilities::PartitionedReplicaTraceComponent<Common::TraceTaskCodes::TR>
    {
        K_FORCE_SHARED(ComProxyOperationDataStream)

    public:

        static NTSTATUS Create(
            __in Data::Utilities::PartitionedReplicaId const & traceId,
            __in Common::ComPointer<IFabricOperationDataStream> comOperationDataStream,
            __in KAllocator & allocator,
            __out ComProxyOperationDataStream::SPtr & comProxyOperationDataStream) noexcept;

        ktl::Awaitable<NTSTATUS> GetNextAsync(
            __in ktl::CancellationToken const & cancellationToken,
            __out Data::Utilities::OperationData::CSPtr & result) noexcept override;

    private:

        ComProxyOperationDataStream(
            __in Data::Utilities::PartitionedReplicaId const & traceId,
            __in Common::ComPointer<IFabricOperationDataStream> comOperationStream);

        class AsyncGetNextContextBase : 
            public Ktl::Com::FabricAsyncContextBase
        {
        };
 
        //
        // GetNext async 
        //
        class AsyncGetNextContext
            : public Ktl::Com::AsyncCallOutAdapter<AsyncGetNextContextBase>
        {
            friend ComProxyOperationDataStream;

            K_FORCE_SHARED(AsyncGetNextContext)

        public:

            ktl::Awaitable<NTSTATUS> GetNextAsync(__out Data::Utilities::OperationData::CSPtr & operationData) noexcept;
            
            Data::Utilities::OperationData::CSPtr GetResult() noexcept;

        protected:

            HRESULT OnEnd(__in IFabricAsyncOperationContext & context) noexcept;

        private:
            
            ComProxyOperationDataStream::SPtr parent_;
            Data::Utilities::OperationData::CSPtr result_;
            ktl::AwaitableCompletionSource<NTSTATUS>::SPtr tcs_;
        };

        Common::ComPointer<IFabricOperationDataStream> comOperationDataStream_;
    };
}
