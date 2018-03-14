// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicationComponent
    {
        class SecondaryReplicator::DrainQueueAsyncOperation : public Common::AsyncOperation
        {
            DENY_COPY(DrainQueueAsyncOperation)

        public:
            DrainQueueAsyncOperation(
                __in SecondaryReplicator & parent,
                bool requireServiceAck,
                DispatchQueueSPtr const & dispatchQueue,
                bool isCopyQueue,
                std::wstring const & queueType,
                std::wstring const & drainScenario,
                Common::AsyncCallback const & callback, 
                Common::AsyncOperationSPtr const & state);
           
            virtual ~DrainQueueAsyncOperation() {}

            static Common::ErrorCode End(
                Common::AsyncOperationSPtr const & asyncOperation);

            void ResumeOutsideLock(Common::AsyncOperationSPtr const & thisSPtr);
            void CheckIfAllCopyOperationsAcked(Common::AsyncOperationSPtr const & asyncOperation);
            void CheckIfAllReplicationOperationsAcked(Common::AsyncOperationSPtr const & asyncOperation);
            void OnDrainStreamFaulted();

        protected:
            void OnStart(Common::AsyncOperationSPtr const & thisSPtr);
            void OnCancel();

        private:
            void FinishWaitForQueueToDrain(Common::AsyncOperationSPtr const & asyncOperation);
            void WaitForQueueToDrainCallback(Common::AsyncOperationSPtr const & asyncOperation);
            
            SecondaryReplicator & parent_;
            bool const requireServiceAck_;
            bool const isCopyQueue_;
            DispatchQueueSPtr const & dispatchQueue_;
            std::wstring const & queueType_;
            std::wstring const & drainScenario_;
            
            // lock protects the following 2 bools
            RWLOCK(RESecondaryReplicatorDrainAsyncOp, lock_);
            bool startedComplete_;
            bool hasQueueDrainCompletedSuccessfully_;
            bool isCancelRequested_;
            Common::atomic_bool isStreamFaulted_;
        };
    } // end namespace ReplicationComponent
} // end namespace Reliability
