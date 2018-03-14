// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace RepairManager
    {
        class ProcessRepairTaskContextsAsyncOperation :
            public Common::AsyncOperation,
            public Store::ReplicaActivityTraceComponent<Common::TraceTaskCodes::RepairManager>
        {
        public:

            ProcessRepairTaskContextsAsyncOperation(
                __in RepairManagerServiceReplica & owner,
                std::vector<std::shared_ptr<RepairTaskContext>> && tasks,
                Store::ReplicaActivityId const & activityId,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);

        protected:

            void OnStart(Common::AsyncOperationSPtr const& thisSPtr) override;

        private:

            void ScheduleProcessNextTask(Common::AsyncOperationSPtr const & thisSPtr);
            void ProcessNextTask(Common::AsyncOperationSPtr const & thisSPtr);
            void OnProcessNextTaskComplete(Common::AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously);

            RepairManagerServiceReplica & owner_;

            std::vector<std::shared_ptr<RepairTaskContext>> tasks_;
            size_t currentTaskIndex_;
            Common::ActivityId currentNestedActivityId_;

            Common::TimerSPtr timer_;
        };
    }
}
