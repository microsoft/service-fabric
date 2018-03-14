// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace RepairManager
    {
        class CleanupAsyncOperation :
            public Common::AsyncOperation,
            public Store::ReplicaActivityTraceComponent<Common::TraceTaskCodes::RepairManager>
        {
        public:

            CleanupAsyncOperation(
                __in RepairManagerServiceReplica & owner,
                Store::ReplicaActivityId const & activityId,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);

        protected:

            void OnStart(Common::AsyncOperationSPtr const& thisSPtr) override;

        private:

            void RunCleanup(Common::AsyncOperationSPtr const & thisSPtr);
            void OnGetNodeListComplete(Common::AsyncOperationSPtr const & operation);
            void OnDeleteTaskComplete(Common::AsyncOperationSPtr const & operation);

            RepairManagerServiceReplica & owner_;
            Common::ActivityId currentNestedActivityId_;
        };
    }
}
