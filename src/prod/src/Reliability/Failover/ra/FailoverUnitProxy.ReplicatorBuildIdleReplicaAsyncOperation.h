// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent 
    {
        class FailoverUnitProxy::ReplicatorBuildIdleReplicaAsyncOperation : public Common::AsyncOperation
        {
            DENY_COPY(ReplicatorBuildIdleReplicaAsyncOperation);

        public:
            ReplicatorBuildIdleReplicaAsyncOperation(
                Reliability::ReplicaDescription const & idleReplicaDescription,
                FailoverUnitProxy & owner,
                Common::AsyncCallback const & callback, 
                Common::AsyncOperationSPtr const & parent) :
                owner_(owner),
                idleReplicaDescription_(idleReplicaDescription),
                AsyncOperation(callback, parent)
            {
            }

            virtual ~ReplicatorBuildIdleReplicaAsyncOperation() {}

            static Common::ErrorCode End(Common::AsyncOperationSPtr const & asyncOperation);
        
        protected:
            void OnStart(Common::AsyncOperationSPtr const & thisSPtr);

        private:
            void ReplicatorBuildIdleReplicaCompletedCallback(Common::AsyncOperationSPtr const & buildIdleReplicaAsyncOperation);
            void FinishReplicatorBuildIdleReplica(Common::AsyncOperationSPtr const & buildIdleReplicaAsyncOperation);

            Reliability::ReplicaDescription const & idleReplicaDescription_;
            FailoverUnitProxy & owner_;
        }; 
    }
}
