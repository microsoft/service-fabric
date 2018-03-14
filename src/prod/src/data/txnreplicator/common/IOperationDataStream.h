// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TxnReplicator
{
    // encapsulates a stream of OperationData objects that are exchanged between Primary replica and Secondary replica.
    // Objects that implement IOperationDataStream are used during the copy process.
    // Both the copy context GetCopyContext method that is sent from the Secondary replica to the Primary replica 
    // and the copy state GetCopyState(LONG64, IOperationDataStream) method implement the IOperationDataStream interface.
    interface IOperationDataStream
        : public Data::Utilities::IDisposable
    {
        K_SHARED_INTERFACE(IOperationDataStream)

    public:

        //
        // GetNextAsync operation
        //
        // Gets the next OperationData object from the IOperationDataStream
        // cancellationToken: Provides a mechanism to cancel the asynchronous operation.
        // Returns Awaitable of type OperationData
        // Returning null indicates to the system that the transfer is complete.
        //
        class AsyncGetNextContext 
            : public Ktl::Com::FabricAsyncContextBase
        {
            K_FORCE_SHARED_WITH_INHERITANCE(AsyncGetNextContext)

        public:

            virtual HRESULT StartGetNext(
                __in_opt KAsyncContextBase * const parentAsyncContext,
                __in_opt KAsyncContextBase::CompletionCallback callback) noexcept = 0 ;

            virtual HRESULT GetResult(Data::Utilities::OperationData::CSPtr & result) noexcept = 0;
        };

        virtual HRESULT CreateAsyncGetNextContext(__out AsyncGetNextContext::SPtr & asyncContext) noexcept = 0;
    };
}
