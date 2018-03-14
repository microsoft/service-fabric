// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent 
    {
        class FailoverUnitProxy::ChangeReplicaRoleAsyncOperation : public FailoverUnitProxy::UserApiInvokerAsyncOperationBase
        {
            DENY_COPY(ChangeReplicaRoleAsyncOperation);

        public:
            ChangeReplicaRoleAsyncOperation(
                FailoverUnitProxy & owner,
                Common::AsyncCallback const & callback, 
                Common::AsyncOperationSPtr const & parent) :                                
                previousRole_(ReplicaRole::Unknown),
                UserApiInvokerAsyncOperationBase(owner, Common::ApiMonitoring::InterfaceName::IStatefulServiceReplica, Common::ApiMonitoring::ApiName::ChangeRole, callback, parent)
            {
            }

            ~ChangeReplicaRoleAsyncOperation()
            {
            }

            static ProxyErrorCode End(Common::AsyncOperationSPtr const & asyncOperation, __out std::wstring & serviceLocation);
        
        protected:
            void OnStart(Common::AsyncOperationSPtr const & thisSPtr) override;
                
        private:
            void ChangeRoleCompletedCallback(Common::AsyncOperationSPtr const & changeRoleAsyncOperation);
            void FinishChangeRole(Common::AsyncOperationSPtr const & changeRoleAsyncOperation);

            std::wstring serviceLocation_;
            ReplicaRole::Enum previousRole_;
        }; 
    }
}
