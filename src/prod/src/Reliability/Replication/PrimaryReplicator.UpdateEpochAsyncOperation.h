// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicationComponent
    {
        class PrimaryReplicator::UpdateEpochAsyncOperation : public Common::AsyncOperation
        {
            DENY_COPY(UpdateEpochAsyncOperation)

        public:
            UpdateEpochAsyncOperation(
                __in PrimaryReplicator & parent,
                __in FABRIC_EPOCH const & epoch,
                Common::AsyncCallback const & callback, 
                Common::AsyncOperationSPtr const & state);
           
            virtual ~UpdateEpochAsyncOperation() {}

            static Common::ErrorCode End(
                Common::AsyncOperationSPtr const & asyncOperation);

        protected:
            virtual void OnStart(Common::AsyncOperationSPtr const & thisSPtr);

        private:
            void FinishUpdateEpoch(
                Common::AsyncOperationSPtr const & asyncOperation,
                bool completedSynchronously,
                Common::ApiMonitoring::ApiCallDescriptionSPtr callDesc);

            PrimaryReplicator & parent_;
            FABRIC_EPOCH epoch_;
            Common::ApiMonitoring::ApiNameDescription apiNameDesc_;
        };
    } // end namespace ReplicationComponent
} // end namespace Reliability
