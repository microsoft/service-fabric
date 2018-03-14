// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicationComponent
    {
        class Replicator::OnDataLossAsyncOperation : public Common::AsyncOperation
        {
            DENY_COPY(OnDataLossAsyncOperation)

        public:
            OnDataLossAsyncOperation(
                __in Replicator & parent,
                Common::AsyncCallback const & callback, 
                Common::AsyncOperationSPtr const & state);
           
            static Common::ErrorCode End(
                Common::AsyncOperationSPtr const & asyncOperation, 
                __out BOOLEAN & isStateChanged);

        protected:
            virtual void OnStart(Common::AsyncOperationSPtr const & thisSPtr);

        private:
            void OnDataLossCallback(Common::AsyncOperationSPtr const & asyncOperation, Common::ApiMonitoring::ApiCallDescriptionSPtr callDesc);
            void FinishOnDataLoss(Common::AsyncOperationSPtr const & asyncOperation, Common::ApiMonitoring::ApiCallDescriptionSPtr callDesc);

            Replicator & parent_;
            BOOLEAN isStateChanged_;
            Common::ApiMonitoring::ApiNameDescription apiNameDesc_;
        };
    } // end namespace ReplicationComponent
} // end namespace Reliability
