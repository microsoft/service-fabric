// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicationComponent
    {
        class SecondaryReplicator::CloseAsyncOperation2 : public Common::AsyncOperation
        {
            DENY_COPY(CloseAsyncOperation2)

        public:
            CloseAsyncOperation2(
                __in SecondaryReplicator & parent,
                bool waitForOperationsToBeDrained,
                Common::AsyncCallback const & callback, 
                Common::AsyncOperationSPtr const & state);
           
            virtual ~CloseAsyncOperation2() {}

            __declspec (property(get=get_IsPromoting)) bool IsPromoting;
            bool get_IsPromoting() const { return isPromoting_; }

            static Common::ErrorCode End(
                Common::AsyncOperationSPtr const & asyncOperation);

            void ResumeOutsideLock(Common::AsyncOperationSPtr const & thisSPtr);

        protected:
            void OnStart(Common::AsyncOperationSPtr const & thisSPtr);

        private:
            void WaitForCopyQueueToDrain(Common::AsyncOperationSPtr const & thisSPtr);
            void WaitForCopyQueueToDrainCallback(Common::AsyncOperationSPtr const & thisSPtr, bool completedSynchronously);
            void WaitForReplicationQueueToDrain(Common::AsyncOperationSPtr const & thisSPtr);
            void WaitForReplicationQueueToDrainCallback(Common::AsyncOperationSPtr const & thisSPtr, bool completedSynchronously);
            void WaitForPendingUpdateEpochToFinish(Common::AsyncOperationSPtr const & thisSPtr);
            void WaitForPendingUpdateEpochToFinishCallback(Common::AsyncOperationSPtr const & thisSPtr, bool completedSynchronously);
            void WaitForPendingGetCopyContextToFinish(Common::AsyncOperationSPtr const & thisSPtr);
            void WaitForPendingGetCopyContextToFinishCallback(Common::AsyncOperationSPtr const & thisSPtr, bool completedSynchronously);

            SecondaryReplicator & parent_;
            bool isPromoting_;
        };
    } // end namespace ReplicationComponent
} // end namespace Reliability
