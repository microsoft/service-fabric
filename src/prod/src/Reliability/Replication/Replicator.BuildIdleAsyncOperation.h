// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicationComponent
    {
        class Replicator::BuildIdleAsyncOperation : 
            public Common::AsyncOperation
        {
            DENY_COPY(BuildIdleAsyncOperation)

        public:
            BuildIdleAsyncOperation(
                __in Replicator & parent,
                ReplicaInformation const & replica,
                Common::AsyncCallback const & callback, 
                Common::AsyncOperationSPtr const & state);

            static Common::ErrorCode End(
                Common::AsyncOperationSPtr const & asyncOperation);

        protected:
            void OnStart(Common::AsyncOperationSPtr const & thisSPtr);

        private:

            void BuildIdleCallback(
                Common::AsyncOperationSPtr const & asyncOperation,
                PrimaryReplicatorSPtr const & primary);

            void FinishBuildIdle(
                Common::AsyncOperationSPtr const & asyncOperation,
                PrimaryReplicatorSPtr const & primary);

            Replicator & parent_;
            ReplicaInformation replica_;
        };
    } // end namespace ReplicationComponent
} // end namespace Reliability
