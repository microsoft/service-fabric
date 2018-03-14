// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicationComponent
    {
        class PrimaryReplicator::BuildIdleAsyncOperation : public Common::AsyncOperation
        {
            DENY_COPY(BuildIdleAsyncOperation)

        public:
            BuildIdleAsyncOperation(
                __in PrimaryReplicator & parent,
                ReplicaInformation const & replica,
                Common::AsyncCallback const & callback, 
                Common::AsyncOperationSPtr const & state);
           
            virtual ~BuildIdleAsyncOperation() {}

            void FinishEstablishCopy(
                Common::AsyncOperationSPtr const & asyncOperation,
                FABRIC_SEQUENCE_NUMBER replicationStartSeq,
                bool completedSynchronously);

            static Common::ErrorCode End(
                Common::AsyncOperationSPtr const & asyncOperation);

        protected:
            virtual void OnStart(Common::AsyncOperationSPtr const & thisSPtr);
            virtual void OnCancel();

        private:

            static void CleanupCCReceiver(
                ReplicationSessionSPtr const & session,
                Common::ErrorCode const & errorCode);

            static void CleanupCCReceiverAndCompleteOperation(
                ReplicationSessionSPtr const & session,
                Common::AsyncOperationSPtr const & thisSPtr,
                Common::ErrorCode const & errorCode);

            inline bool IsCancelledLockedRead();

            PrimaryReplicator & parent_;
            ReplicaInformation replica_;
            ReplicationSessionSPtr session_;
            bool isCancelled_;
            BatchedHealthReporterSPtr const healthReporter_;
            MUTABLE_RWLOCK(REPrimaryReplicatorBuildIdleAsyncOperationIsCancelled, isCancelledLock_);
        };
    } // end namespace ReplicationComponent
} // end namespace Reliability
