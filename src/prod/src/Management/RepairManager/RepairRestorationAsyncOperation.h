// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace RepairManager
    {
        class RepairRestorationAsyncOperation :
            public Common::AsyncOperation,
            public Store::ReplicaActivityTraceComponent<Common::TraceTaskCodes::RepairManager>
        {
        public:
    
            RepairRestorationAsyncOperation(
                RepairManagerServiceReplica & owner,
                RepairTaskContext & context,
                Store::ReplicaActivityId const & activityId,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);

        protected:

            void OnStart(Common::AsyncOperationSPtr const & thisSPtr) override;

            void CancelPreparation(Common::AsyncOperationSPtr const & thisSPtr);

            void OnCancelPreparationComplete(
                Common::AsyncOperationSPtr const & operation,
                bool expectedCompletedSynchronously);

            void StartRestoration(Common::AsyncOperationSPtr const & thisSPtr);

            void SendRemoveDeactivationRequest(Common::AsyncOperationSPtr const & thisSPtr);

            void OnRemoveDeactivationRequestComplete(
                Common::AsyncOperationSPtr const & operation,
                bool expectedCompletedSynchronously);

            void CompleteTask(Common::AsyncOperationSPtr const & thisSPtr);

            void OnCompleteTaskComplete(
                Common::AsyncOperationSPtr const & operation,
                bool expectedCompletedSynchronously);

            void UpdateHealthCheckStartInfo(__in Common::AsyncOperationSPtr const & thisSPtr);
            void UpdateHealthCheckEndInfo(__in Common::AsyncOperationSPtr const & thisSPtr);

            void OnUpdateHealthCheckStartInfoComplete(
                __in Common::AsyncOperationSPtr const & operation,
                __in bool expectedCompletedSynchronously);
            void OnHealthCheckEndComplete(
                __in Common::AsyncOperationSPtr const & operation,
                __in bool expectedCompletedSynchronously);

            void UpdateHealthCheckResultInStore(__in Common::AsyncOperationSPtr const & thisSPtr);
            Common::ErrorCode PerformRepairTaskHealthCheck();

        private:

            RepairManagerServiceReplica & owner_;
            RepairTaskContext & context_;
        };
    }
}
