// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent 
    {
        class FailoverUnitProxy::ChangeReplicatorRoleAsyncOperation : public FailoverUnitProxy::UserApiInvokerAsyncOperationBase
        {
            DENY_COPY(ChangeReplicatorRoleAsyncOperation);

        public:
            ChangeReplicatorRoleAsyncOperation(
                FailoverUnitProxy & owner,
                Common::AsyncCallback const & callback, 
                Common::AsyncOperationSPtr const & parent) :
                previousRole_(ReplicaRole::Unknown),
                UserApiInvokerAsyncOperationBase(owner, Common::ApiMonitoring::InterfaceName::IReplicator, Common::ApiMonitoring::ApiName::ChangeRole, callback, parent)
            {
            }

            virtual ~ChangeReplicatorRoleAsyncOperation()
            {
            }

            static ProxyErrorCode End(Common::AsyncOperationSPtr const & asyncOperation);

        protected:
            void OnStart(Common::AsyncOperationSPtr const & thisSPtr) override;
                
        private:
            void ChangeRoleCompletedCallback(Common::AsyncOperationSPtr const & changeRoleAsyncOperation);
            void FinishChangeRole(Common::AsyncOperationSPtr const & changeRoleAsyncOperation);

            ReplicaRole::Enum previousRole_;
        }; 
    }
}
