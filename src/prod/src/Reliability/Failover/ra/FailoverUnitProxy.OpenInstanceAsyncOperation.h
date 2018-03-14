// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent 
    {
        class FailoverUnitProxy::OpenInstanceAsyncOperation : public FailoverUnitProxy::UserApiInvokerAsyncOperationBase
        {
            DENY_COPY(OpenInstanceAsyncOperation);

        public:
            OpenInstanceAsyncOperation(
                Hosting2::IApplicationHost & applicationHost,
                FailoverUnitProxy & owner,
                Common::AsyncCallback const & callback, 
                Common::AsyncOperationSPtr const & parent) :
                UserApiInvokerAsyncOperationBase(owner, Common::ApiMonitoring::InterfaceName::IStatelessServiceInstance, Common::ApiMonitoring::ApiName::Open, callback, parent),
                applicationHost_(applicationHost)
            {
            }

            virtual ~OpenInstanceAsyncOperation() {}

            static ProxyErrorCode End(Common::AsyncOperationSPtr const & asyncOperation, __out std::wstring & serviceLocation);

        protected:
            void OnStart(Common::AsyncOperationSPtr const & thisSPtr) override;

        private:
            void OpenInstanceCompletedCallback(Common::AsyncOperationSPtr const & openAsyncOperation);
            void FinishOpenInstance(Common::AsyncOperationSPtr const & openAsyncOperation);
            void FinishWork(Common::AsyncOperationSPtr const & thisSPtr, Common::ErrorCode const & result);
            void OnGetFactoryAndCreateInstanceCompleted(Common::AsyncOperationSPtr const & createAsyncOperation, bool expectedCompletedSynchronously);

            Hosting2::IApplicationHost & applicationHost_;
            std::wstring serviceLocation_;
            Common::ComPointer<ComStatelessServicePartition> statelessServicePartition_;
            ComProxyStatelessServiceUPtr statelessService_;
        }; 
    }
}
