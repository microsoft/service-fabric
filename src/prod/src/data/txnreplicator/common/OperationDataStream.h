// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TxnReplicator
{
    //
    // Implements IOperationDataStream and additionally provides a helper method to convert the ktl async to ktl awaitable
    //
    class OperationDataStream
        : public KObject<OperationDataStream>
        , public KShared<OperationDataStream>
        , public IOperationDataStream
    {
        K_FORCE_SHARED_WITH_INHERITANCE(OperationDataStream)
        K_SHARED_INTERFACE_IMP(IOperationDataStream)
        K_SHARED_INTERFACE_IMP(IDisposable)

    public:

        HRESULT CreateAsyncGetNextContext(__out AsyncGetNextContext::SPtr & asyncContext) noexcept override;

        virtual ktl::Awaitable<NTSTATUS> GetNextAsync(
            __in ktl::CancellationToken const & cancellationToken,
            __out Data::Utilities::OperationData::CSPtr & result) noexcept = 0;

        virtual void Dispose() override;

    private:

        //
        // GetNextAsync operation
        //
        // Gets the next OperationData object from the IOperationDataStream
        // cancellationToken: Provides a mechanism to cancel the asynchronous operation.
        // Returns Awaitable of type OperationData
        // Returning null indicates to the system that the transfer is complete.
        //
        class AsyncGetNextContextImpl
            : public AsyncGetNextContext
        {
            friend OperationDataStream;

            K_FORCE_SHARED(AsyncGetNextContextImpl)

        public:

            HRESULT StartGetNext(
                __in_opt KAsyncContextBase * const parentAsyncContext,
                __in_opt KAsyncContextBase::CompletionCallback callback) noexcept override;

            HRESULT GetResult(Data::Utilities::OperationData::CSPtr & result) noexcept override;

        protected:

            void OnStart() noexcept;

        private:

            ktl::Task DoWork() noexcept;

            OperationDataStream::SPtr parent_;
            Data::Utilities::OperationData::CSPtr result_;
        };
    };
}

