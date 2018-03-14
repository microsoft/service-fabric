// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent 
    {
        class FailoverUnitProxy::CloseAsyncOperation : public Common::AsyncOperation
        {
            DENY_COPY(CloseAsyncOperation);

        public:
            CloseAsyncOperation(
                FailoverUnitProxy & owner,
                Common::AsyncCallback const & callback, 
                Common::AsyncOperationSPtr const & parent) :
                owner_(owner),
                AsyncOperation(callback, parent) {}
            virtual ~CloseAsyncOperation() {}

            static Common::ErrorCode End(Common::AsyncOperationSPtr const & asyncOperation);

        protected:
            void OnStart(Common::AsyncOperationSPtr const & thisSPtr);

        private:
            enum Actions
            {
                CloseInstance,
                CloseReplicator,
                CloseReplica,
                Done
            };

            void ProcessActions(Common::AsyncOperationSPtr const & thisSPtr, int nextStep);

            void DoCloseInstance(Common::AsyncOperationSPtr const & thisSPtr, int const nextStep, __out bool & isCompletedSynchronously);
            void CloseInstanceCompletedCallback(int const nextStep, Common::AsyncOperationSPtr const & closeInstanceAsyncOperation);
            void FinishCloseInstance(Common::AsyncOperationSPtr const & closeInstanceAsyncOperation);

            void DoCloseReplica(Common::AsyncOperationSPtr const & thisSPtr, int const nextStep, __out bool & isCompletedSynchronously);
            void CloseReplicaCompletedCallback(int const nextStep, Common::AsyncOperationSPtr const & closeReplicaAsyncOperation);
            void FinishCloseReplica(Common::AsyncOperationSPtr const & closeReplicaAsyncOperation);

            void DoAbortReplicator(Common::AsyncOperationSPtr const & thisSPtr, int const nextStep, __out bool & isCompletedSynchronously);
            
            void DoCloseReplicator(Common::AsyncOperationSPtr const & thisSPtr, int const nextStep, __out bool & isCompletedSynchronously);
            void CloseReplicatorCompletedCallback(int const nextStep, Common::AsyncOperationSPtr const & closeReplicatorAsyncOperation);
            void FinishCloseReplicator(Common::AsyncOperationSPtr const & closeReplicatorAsyncOperation);

            FailoverUnitProxy & owner_;
        }; 
    }
}
