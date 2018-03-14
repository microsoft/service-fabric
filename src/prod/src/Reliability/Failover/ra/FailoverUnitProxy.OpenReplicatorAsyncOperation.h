// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent 
    {
        class FailoverUnitProxy::OpenReplicatorAsyncOperation : public FailoverUnitProxy::UserApiInvokerAsyncOperationBase
        {
            DENY_COPY(OpenReplicatorAsyncOperation);

        public:
            OpenReplicatorAsyncOperation(
                FailoverUnitProxy & owner,
                Common::AsyncCallback const & callback, 
                Common::AsyncOperationSPtr const & parent) :
                UserApiInvokerAsyncOperationBase(owner, Common::ApiMonitoring::InterfaceName::IReplicator, Common::ApiMonitoring::ApiName::Open, callback, parent)
            {
            }

            virtual ~OpenReplicatorAsyncOperation() {}

            static ProxyErrorCode End(Common::AsyncOperationSPtr const & asyncOperation, __out std::wstring & replicationEndpoint);
        
        protected:
            void OnStart(Common::AsyncOperationSPtr const & thisSPtr) override;

        private:
            void OpenReplicatorCompletedCallback(Common::AsyncOperationSPtr const & operation);
            void FinishOpenReplicator(Common::AsyncOperationSPtr const & operation);

            std::wstring replicationEndpoint_;
        }; 
    }
}
