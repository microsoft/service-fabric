// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicationComponent
    {
        class Replicator::ChangeRoleAsyncOperation : 
            public Common::AsyncOperation
        {
            DENY_COPY(ChangeRoleAsyncOperation)

        public:
            ChangeRoleAsyncOperation(
                __in Replicator & parent,
                FABRIC_EPOCH const & epoch,
                FABRIC_REPLICA_ROLE newRole,
                Common::AsyncCallback const & callback, 
                Common::AsyncOperationSPtr const & state);

            static Common::ErrorCode End(
                Common::AsyncOperationSPtr const & asyncOperation);

        protected:
            virtual void OnStart(Common::AsyncOperationSPtr const & thisSPtr);

        private:
            void CreateInitialPrimary(FABRIC_SEQUENCE_NUMBER cachedProgress, Common::AsyncOperationSPtr const & thisSPtr);
            Common::ErrorCode CreateInitialSecondary();
            void FinishUpdatePrimaryEpoch(Common::AsyncOperationSPtr const & asyncOperation, bool completedSynchronously);


            void PromoteIdleToActive(Common::AsyncOperationSPtr const & thisSPtr);
            void ClosePrimary(Common::AsyncOperationSPtr const & thisSPtr, bool createSecondary = false);
            void ClosePrimaryCallback(Common::AsyncOperationSPtr const & asyncOperation, bool createSecondary);
            void FinishClosePrimary(Common::AsyncOperationSPtr const & asyncOperation, bool createSecondary);

            void CloseSecondary(Common::AsyncOperationSPtr const & thisSPtr, bool createPrimary = false);
            void CloseSecondaryCallback(Common::AsyncOperationSPtr const & asyncOperation, bool createPrimary);
            void FinishCloseSecondary(Common::AsyncOperationSPtr const & asyncOperation, bool createPrimary);

            void ScheduleOpenPrimaryAndUpdateEpoch(Common::AsyncOperationSPtr const & thisSPtr);

            Replicator & parent_;
            FABRIC_EPOCH epoch_;
            FABRIC_REPLICA_ROLE newRole_;
            PrimaryReplicatorSPtr primaryCopy_;
            SecondaryReplicatorSPtr secondaryCopy_;
        };
    } // end namespace ReplicationComponent
} // end namespace Reliability
