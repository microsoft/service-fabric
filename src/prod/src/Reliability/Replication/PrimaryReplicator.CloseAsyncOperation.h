// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicationComponent
    {
        class PrimaryReplicator::CloseAsyncOperation : public Common::AsyncOperation
        {
            DENY_COPY(CloseAsyncOperation)

        public:
            CloseAsyncOperation(
                __in PrimaryReplicator & parent,
                bool isCreatingSecondary,
                Common::TimeSpan const & waitForQuorumTimeout,
                Common::AsyncCallback const & callback, 
                Common::AsyncOperationSPtr const & state);
           
            virtual ~CloseAsyncOperation();

            static Common::ErrorCode End(
                Common::AsyncOperationSPtr const & asyncOperation);

        protected:
            virtual void OnStart(Common::AsyncOperationSPtr const & thisSPtr);

        private:
            void PerformClose(Common::AsyncOperationSPtr const & thisSPtr);
            
            void FinishBeginWaitForReplicateOperationsToComplete(Common::AsyncOperationSPtr const & innerAsyncOp);

            void FinishBeginWaitForReplicateOperationsCallbacksToComplete(Common::AsyncOperationSPtr const & innerAsyncOp);

            Common::TimeSpan const waitForQuorumTimeout_;
            PrimaryReplicator & parent_;
            bool const isCreatingSecondary_;
        };
    } // end namespace ReplicationComponent
} // end namespace Reliability
