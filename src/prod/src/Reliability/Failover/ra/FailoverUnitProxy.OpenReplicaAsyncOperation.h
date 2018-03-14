// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent 
    {
        class FailoverUnitProxy::OpenReplicaAsyncOperation : public FailoverUnitProxy::UserApiInvokerAsyncOperationBase
        {
            DENY_COPY(OpenReplicaAsyncOperation);

        public:
            OpenReplicaAsyncOperation(
                Hosting2::IApplicationHost & applicationHost,
                FailoverUnitProxy & owner,
                Common::AsyncCallback const & callback, 
                Common::AsyncOperationSPtr const & parent) :
                applicationHost_(applicationHost),
                UserApiInvokerAsyncOperationBase(owner, Common::ApiMonitoring::InterfaceName::IStatefulServiceReplica, Common::ApiMonitoring::ApiName::Open, callback, parent)
            {
            }

            virtual ~OpenReplicaAsyncOperation() {}

            static ProxyErrorCode End(Common::AsyncOperationSPtr const & asyncOperation);

        protected:
            void OnStart(Common::AsyncOperationSPtr const & thisSPtr) override;

        private:
            void OpenReplicaCompletedCallback(Common::AsyncOperationSPtr const & openAsyncOperation);
            void FinishOpenReplica(Common::AsyncOperationSPtr const & openAsyncOperation);
            void FinishWork(Common::AsyncOperationSPtr const & thisSPtr, Common::ErrorCode const & result);
            void OnGetFactoryAndCreateReplicaCompleted(Common::AsyncOperationSPtr const & createAsyncOperation, bool expectedCompletedSynchronously);

            Hosting2::IApplicationHost & applicationHost_;
            Common::ComPointer<ComStatefulServicePartition> statefulServicePartition_;
            ComProxyStatefulServiceUPtr statefulService_;
        }; 
    }
}
