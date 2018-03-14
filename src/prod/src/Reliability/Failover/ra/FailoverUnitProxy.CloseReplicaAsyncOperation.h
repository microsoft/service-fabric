// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent 
    {
        class FailoverUnitProxy::CloseReplicaAsyncOperation : public FailoverUnitProxy::UserApiInvokerAsyncOperationBase
        {
            DENY_COPY(CloseReplicaAsyncOperation);

        public:
            CloseReplicaAsyncOperation(
                FailoverUnitProxy & owner,
                Common::AsyncCallback const & callback, 
                Common::AsyncOperationSPtr const & parent) :                
                UserApiInvokerAsyncOperationBase(owner, Common::ApiMonitoring::InterfaceName::IStatefulServiceReplica, Common::ApiMonitoring::ApiName::Close, callback, parent) {}
            virtual ~CloseReplicaAsyncOperation() {}

            static ProxyErrorCode End(Common::AsyncOperationSPtr const & asyncOperation);

        protected:
            void OnStart(Common::AsyncOperationSPtr const & thisSPtr) override;

        private:
            void CloseReplicaCompletedCallback(Common::AsyncOperationSPtr const & closeAsyncOperation);
            void FinishCloseReplica(Common::AsyncOperationSPtr const & closeAsyncOperation);
        }; 
    }
}
