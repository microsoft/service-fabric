// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicationComponent
    {
        class OperationStream::GetOperationAsyncOperation : public Common::AsyncOperation
        {
            DENY_COPY(GetOperationAsyncOperation)

        public:
            GetOperationAsyncOperation(
                __in OperationStream & parent,
                Common::AsyncCallback const & callback, 
                Common::AsyncOperationSPtr const & state);
           
            virtual ~GetOperationAsyncOperation() {}

            static Common::ErrorCode End(
                Common::AsyncOperationSPtr const & asyncOperation,
                __out IFabricOperation * & operation);

            void ResumeOutsideLock(Common::AsyncOperationSPtr const & thisSPtr);

        protected:
            void OnStart(Common::AsyncOperationSPtr const & thisSPtr);

        private:       
            void StartDequeue(Common::AsyncOperationSPtr const & thisSPtr);
            void FinishDequeue(Common::AsyncOperationSPtr const & asyncOperation);
            void DequeueCallback(Common::AsyncOperationSPtr const & asyncOperation);
            void UpdateSecondaryEpoch(
                FABRIC_EPOCH const & epoch,
                FABRIC_SEQUENCE_NUMBER previousEpochLastSequenceNumber,
                Common::AsyncOperationSPtr const & thisSPtr);
            void FinishUpdateSecondaryEpoch(
                Common::AsyncOperationSPtr const & asyncOperation,
                bool completedSynchronously);

            OperationStream & parent_;
            IFabricOperation *operation_;
        };
    } // end namespace ReplicationComponent
} // end namespace Reliability
