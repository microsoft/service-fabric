// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent 
    {
        class FailoverUnitProxy::ReplicatorOnDataLossAsyncOperation : public FailoverUnitProxy::UserApiInvokerAsyncOperationBase
        {
            DENY_COPY(ReplicatorOnDataLossAsyncOperation);

        public:
            ReplicatorOnDataLossAsyncOperation(
                FailoverUnitProxy & owner,
                Common::AsyncCallback const & callback, 
                Common::AsyncOperationSPtr const & parent) :
                lastLSN_(FABRIC_INVALID_SEQUENCE_NUMBER),
                UserApiInvokerAsyncOperationBase(owner, Common::ApiMonitoring::InterfaceName::IReplicator, Common::ApiMonitoring::ApiName::OnDataLoss, callback, parent) {}

            virtual ~ReplicatorOnDataLossAsyncOperation() {}

            static ProxyErrorCode End(Common::AsyncOperationSPtr const & asyncOperation, int64 & lastLSN);

        protected:
            void OnStart(Common::AsyncOperationSPtr const & thisSPtr) override;

        private:
            void ReplicatorOnDataLossCompletedCallback(Common::AsyncOperationSPtr const & onDataLossAsyncOperation);
            void FinishReplicatorOnDataLoss(Common::AsyncOperationSPtr const & onDataLossAsyncOperation);
            Common::ErrorCode HandleStateChangeOnDataLoss(bool isStateChanged);

            int64 lastLSN_;
        }; 
    }
}
