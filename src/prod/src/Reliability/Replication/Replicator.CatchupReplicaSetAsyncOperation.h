// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicationComponent
    {
        class Replicator::CatchupReplicaSetAsyncOperation : 
            public Common::AsyncOperation
        {
            DENY_COPY(CatchupReplicaSetAsyncOperation)

        public:
            CatchupReplicaSetAsyncOperation(
                __in Replicator & parent,
                FABRIC_REPLICA_SET_QUORUM_MODE catchUpMode,
                Common::AsyncCallback const & callback, 
                Common::AsyncOperationSPtr const & state);

            static Common::ErrorCode End(
                Common::AsyncOperationSPtr const & asyncOperation);

        protected:
            void OnStart(Common::AsyncOperationSPtr const & thisSPtr);

        private:

            void CatchupReplicaSetCallback(
                Common::AsyncOperationSPtr const & asyncOperation,
                PrimaryReplicatorSPtr const & primary);

            void FinishCatchupReplicaSet(
                Common::AsyncOperationSPtr const & asyncOperation,
                PrimaryReplicatorSPtr const & primary);

            Replicator & parent_;
            FABRIC_REPLICA_SET_QUORUM_MODE catchUpMode_;
        };
    } // end namespace ReplicationComponent
} // end namespace Reliability
