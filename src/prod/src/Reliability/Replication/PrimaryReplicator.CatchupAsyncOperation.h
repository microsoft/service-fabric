// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicationComponent
    {
        class PrimaryReplicator::CatchupAsyncOperation : public Common::AsyncOperation
        {
            DENY_COPY(CatchupAsyncOperation)

        public:
            CatchupAsyncOperation(
                __in PrimaryReplicator & parent,
                FABRIC_REPLICA_SET_QUORUM_MODE catchUpMode,
                Common::AsyncCallback const & callback, 
                Common::AsyncOperationSPtr const & state);
           
            __declspec(property(get=get_CatchupMode)) FABRIC_REPLICA_SET_QUORUM_MODE CatchupMode;
            FABRIC_REPLICA_SET_QUORUM_MODE get_CatchupMode() const
            {
                return catchUpMode_;
            }

            virtual ~CatchupAsyncOperation() {}

            static Common::ErrorCode End(
                Common::AsyncOperationSPtr const & asyncOperation);

            bool IsQuorumProgressAchieved(
                FABRIC_SEQUENCE_NUMBER committedSequenceNumber,
                FABRIC_SEQUENCE_NUMBER previousConfigCatchupLsn,
                FABRIC_SEQUENCE_NUMBER lowestLSNAmongstMustCatchupReplicas);

            bool IsAllProgressAchieved(
                FABRIC_SEQUENCE_NUMBER completedSequenceNumber,
                FABRIC_SEQUENCE_NUMBER latestSequenceNumber);
        protected:
            void OnStart(Common::AsyncOperationSPtr const & thisSPtr);

        private:            
            PrimaryReplicator & parent_;
            FABRIC_REPLICA_SET_QUORUM_MODE const catchUpMode_;
        };
    } // end namespace ReplicationComponent
} // end namespace Reliability
