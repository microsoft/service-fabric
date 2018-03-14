// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicationComponent
    {
        class Replicator::UpdateEpochAsyncOperation : 
            public Common::AsyncOperation
        {
            DENY_COPY(UpdateEpochAsyncOperation)

        public:
            UpdateEpochAsyncOperation(
                __in Replicator & parent,
                FABRIC_EPOCH const & epoch,
                Common::AsyncCallback const & callback, 
                Common::AsyncOperationSPtr const & state);

            static Common::ErrorCode End(
                Common::AsyncOperationSPtr const & asyncOperation);

        protected:
            virtual void OnStart(Common::AsyncOperationSPtr const & thisSPtr);

        private:
            void UpdateEpochCallback(Common::AsyncOperationSPtr const & asyncOperation);
            void FinishUpdateEpoch(Common::AsyncOperationSPtr const & asyncOperation);

            Replicator & parent_;
            FABRIC_EPOCH epoch_;
            PrimaryReplicatorSPtr primaryCopy_;
            SecondaryReplicatorSPtr secondaryCopy_;
        };
    } // end namespace ReplicationComponent
} // end namespace Reliability
