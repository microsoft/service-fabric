// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace LoggingReplicator
    {
        // Interface for the V1 replicator to talk to the logging Replicator
        interface IStateProvider
            : public Utilities::IDisposable
        {
            K_SHARED_INTERFACE(IStateProvider)

        public:

            //
            // OnDataLoss async operation
            //
            class AsyncOnDataLossContext 
                : public Ktl::Com::FabricAsyncContextBase
            {
                K_FORCE_SHARED_WITH_INHERITANCE(AsyncOnDataLossContext)

            public:

                virtual HRESULT
                    StartOnDataLoss(
                    __in_opt KAsyncContextBase * const parentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback callback) = 0;

                virtual HRESULT
                    GetResult(
                    __out BOOLEAN & isStateChanged) = 0;
            };

            virtual HRESULT CreateAsyncOnDataLossContext(__out AsyncOnDataLossContext::SPtr & asyncContext) = 0;

            //
            // UpdateEpoch async operation
            //
            class AsyncUpdateEpochContext 
                : public Ktl::Com::FabricAsyncContextBase
            {
                K_FORCE_SHARED_WITH_INHERITANCE(AsyncUpdateEpochContext)

            public:

                virtual HRESULT StartUpdateEpoch(
                    __in FABRIC_EPOCH const & epoch,
                    __in FABRIC_SEQUENCE_NUMBER previousEpochLastSequenceNumber,
                    __in_opt KAsyncContextBase * const parentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback callback) = 0;
            };

            virtual HRESULT CreateAsyncUpdateEpochContext(__out AsyncUpdateEpochContext::SPtr & asyncContext) = 0;

            virtual HRESULT GetCopyContext(__out TxnReplicator::IOperationDataStream::SPtr & copyContextStream) = 0;

            virtual HRESULT GetCopyState(
                __in FABRIC_SEQUENCE_NUMBER uptoSequenceNumber,
                __in TxnReplicator::OperationDataStream & copyContextStream,
                __out TxnReplicator::IOperationDataStream::SPtr & copyStateStream) = 0;

            virtual HRESULT GetLastCommittedSequenceNumber(FABRIC_SEQUENCE_NUMBER * lsn) = 0;

            virtual void Test_SetTestHookContext(TestHooks::TestHookContext const &)
            {
                CODING_ASSERT("Test_SetTestHookContext not implemented");
            }
        };
    }
}
