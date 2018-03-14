// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent 
    {
        class FailoverUnitProxy::ReplicatorUpdateEpochAsyncOperation : public FailoverUnitProxy::UserApiInvokerAsyncOperationBase
        {
            DENY_COPY(ReplicatorUpdateEpochAsyncOperation);

        public:
            ReplicatorUpdateEpochAsyncOperation(
                FailoverUnitProxy & owner,
                Common::AsyncCallback const & callback, 
                Common::AsyncOperationSPtr const & parent) :
                UserApiInvokerAsyncOperationBase(owner, Common::ApiMonitoring::InterfaceName::IReplicator, Common::ApiMonitoring::ApiName::UpdateEpoch, callback, parent)
            {
            }

            virtual ~ReplicatorUpdateEpochAsyncOperation() {}

            static ProxyErrorCode End(Common::AsyncOperationSPtr const & asyncOperation);

        protected:
            void OnStart(Common::AsyncOperationSPtr const & thisSPtr) override;
                
        private:
            void ReplicatorUpdateEpochCompletedCallback(Common::AsyncOperationSPtr const & replicatorUpdateEpochAsyncOperation);
            void FinishReplicatorUpdateEpoch(Common::AsyncOperationSPtr const & replicatorUpdateEpochAsyncOperation);
        }; 
    }
}
