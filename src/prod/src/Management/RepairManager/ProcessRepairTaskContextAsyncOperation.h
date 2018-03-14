// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace RepairManager
    {
        class ProcessRepairTaskContextAsyncOperation :
            public Common::AsyncOperation,
            public Store::ReplicaActivityTraceComponent<Common::TraceTaskCodes::RepairManager>
        {
        public:

            ProcessRepairTaskContextAsyncOperation(
                RepairManagerServiceReplica & owner,
                RepairTaskContext & context,
                Store::ReplicaActivityId const & activityId,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);

        protected:

            void OnStart(Common::AsyncOperationSPtr const& thisSPtr) override;

        private:

            void ProcessTask(Common::AsyncOperationSPtr const & thisSPtr);
            void FinishProcessTask(Common::AsyncOperationSPtr const & thisSPtr, Common::ErrorCode const & error);

            void PrepareTask(Common::AsyncOperationSPtr const & thisSPtr);
            void OnPrepareTaskComplete(Common::AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously);

            void RestoreTask(Common::AsyncOperationSPtr const & thisSPtr);
            void OnRestoreTaskComplete(Common::AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously);

            RepairManagerServiceReplica & owner_;
            RepairTaskContext & context_;
        };
    }
}
