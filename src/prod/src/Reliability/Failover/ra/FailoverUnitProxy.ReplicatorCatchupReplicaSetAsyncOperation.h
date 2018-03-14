// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent 
    {
        class FailoverUnitProxy::ReplicatorCatchupReplicaSetAsyncOperation : public FailoverUnitProxy::UserApiInvokerAsyncOperationBase
        {
            DENY_COPY(ReplicatorCatchupReplicaSetAsyncOperation);

        public:
            ReplicatorCatchupReplicaSetAsyncOperation(
                CatchupType::Enum catchupType,
                FailoverUnitProxy & owner,
                Common::AsyncCallback const & callback, 
                Common::AsyncOperationSPtr const & parent) :
                catchupType_(catchupType),
                UserApiInvokerAsyncOperationBase(owner, Common::ApiMonitoring::InterfaceName::IReplicator, Common::ApiMonitoring::ApiName::CatchupReplicaSet, callback, parent)
            {
            }

            virtual ~ReplicatorCatchupReplicaSetAsyncOperation() {}

            static ProxyErrorCode End(Common::AsyncOperationSPtr const & asyncOperation);

        protected:
            void OnStart(Common::AsyncOperationSPtr const & thisSPtr) override;

        private:
            void ReplicatorCatchupReplicaSetCompletedCallback(Common::AsyncOperationSPtr const & catchupReplicaSetAsyncOperation);
            void FinishReplicatorCatchupReplicaSet(Common::AsyncOperationSPtr const & catchupReplicaSetAsyncOperation);

            FABRIC_REPLICA_SET_QUORUM_MODE GetCatchupModeCallerHoldsLock() const;

            CatchupType::Enum catchupType_;
        }; 
    }
}
