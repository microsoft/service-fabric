// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent 
    {
        class FailoverUnitProxy::CloseReplicatorAsyncOperation : public FailoverUnitProxy::UserApiInvokerAsyncOperationBase
        {
            DENY_COPY(CloseReplicatorAsyncOperation);

        public:
            CloseReplicatorAsyncOperation(
                FailoverUnitProxy & owner,
                Common::AsyncCallback const & callback, 
                Common::AsyncOperationSPtr const & parent) :
                UserApiInvokerAsyncOperationBase(owner, Common::ApiMonitoring::InterfaceName::IReplicator, Common::ApiMonitoring::ApiName::Close, callback, parent) {}
            virtual ~CloseReplicatorAsyncOperation() {}

            static ProxyErrorCode End(Common::AsyncOperationSPtr const & asyncOperation);
 
        protected:
            void OnStart(Common::AsyncOperationSPtr const & thisSPtr) override;

        private:
            void CloseReplicatorCompletedCallback(Common::AsyncOperationSPtr const & closeAsyncOperation);
            void FinishCloseReplicator(Common::AsyncOperationSPtr const & closeAsyncOperation);
        }; 
    }
}
