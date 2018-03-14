// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent 
    {
        class FailoverUnitProxy::CloseInstanceAsyncOperation : public FailoverUnitProxy::UserApiInvokerAsyncOperationBase
        {
            DENY_COPY(CloseInstanceAsyncOperation);

        public:
            CloseInstanceAsyncOperation(
                FailoverUnitProxy & owner,
                Common::AsyncCallback const & callback, 
                Common::AsyncOperationSPtr const & parent) :                
                UserApiInvokerAsyncOperationBase(owner, Common::ApiMonitoring::InterfaceName::IStatelessServiceInstance, Common::ApiMonitoring::ApiName::Close, callback, parent) {}
            virtual ~CloseInstanceAsyncOperation() {}

            static ProxyErrorCode End(Common::AsyncOperationSPtr const & asyncOperation);

        protected:
            void OnStart(Common::AsyncOperationSPtr const & thisSPtr) override;

        private:
            void CloseInstanceCompletedCallback(Common::AsyncOperationSPtr const & closeAsyncOperation);
            void FinishCloseInstance(Common::AsyncOperationSPtr const & closeAsyncOperation);
        }; 
    }
}
