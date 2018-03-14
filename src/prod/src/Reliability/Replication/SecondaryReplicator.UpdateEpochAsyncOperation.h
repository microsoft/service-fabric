// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicationComponent
    {
        class SecondaryReplicator::UpdateEpochAsyncOperation : public Common::AsyncOperation
        {
            DENY_COPY(UpdateEpochAsyncOperation)

        public:
            UpdateEpochAsyncOperation(
                __in SecondaryReplicator & parent,
                __in FABRIC_EPOCH const & epoch,
                Common::AsyncCallback const & callback, 
                Common::AsyncOperationSPtr const & state);
           
            virtual ~UpdateEpochAsyncOperation() {}

            static Common::ErrorCode End(
                Common::AsyncOperationSPtr const & asyncOperation);

        protected:
            void OnStart(Common::AsyncOperationSPtr const & thisSPtr);

        private:
            void WaitForReplicationQueueToDrain(Common::AsyncOperationSPtr const & thisSPtr);
            void WaitForReplicationQueueToDrainCallback(Common::AsyncOperationSPtr const & thisSPtr, bool completedSynchronously);

            SecondaryReplicator & parent_;
            ::FABRIC_EPOCH epoch_;
        };
    } // end namespace ReplicationComponent
} // end namespace Reliability
