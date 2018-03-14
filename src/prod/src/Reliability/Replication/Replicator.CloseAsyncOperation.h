// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicationComponent
    {
        class Replicator::CloseAsyncOperation : 
            public Common::AsyncOperation
        {
            DENY_COPY(CloseAsyncOperation)

        public:
            CloseAsyncOperation(
                __in Replicator & parent,
                Common::AsyncCallback const & callback, 
                Common::AsyncOperationSPtr const & state);

            static Common::ErrorCode End(
                Common::AsyncOperationSPtr const & asyncOperation);

        protected:
            virtual void OnStart(Common::AsyncOperationSPtr const & thisSPtr);

        private:

            void ClosePrimary(Common::AsyncOperationSPtr const & thisSPtr);
            void ClosePrimaryCallback(Common::AsyncOperationSPtr const & asyncOperation);
            void FinishClosePrimary(Common::AsyncOperationSPtr const & asyncOperation);

            void CloseSecondary(Common::AsyncOperationSPtr const & thisSPtr);
            void CloseSecondaryCallback(Common::AsyncOperationSPtr const & asyncOperation);
            void FinishCloseSecondary(Common::AsyncOperationSPtr const & asyncOperation);

            Replicator & parent_;
            PrimaryReplicatorSPtr primaryCopy_;
            SecondaryReplicatorSPtr secondaryCopy_;
        };
    } // end namespace ReplicationComponent
} // end namespace Reliability
