// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace RepairManager
    {
        /// <summary>
        /// Class that does the actual health check. This is done when there are active repair tasks 
        /// that require health checks.
        /// </summary>
        class HealthCheckAsyncOperation :
            public Common::AsyncOperation,
            public Store::ReplicaActivityTraceComponent<Common::TraceTaskCodes::RepairManager>
        {
        public:

            HealthCheckAsyncOperation(
                RepairManagerServiceReplica & owner,                
                Store::ReplicaActivityId const & activityId,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);

        protected:

            void OnStart(Common::AsyncOperationSPtr const& thisSPtr) override;

        private:

            void PerformHealthCheck(__in Common::AsyncOperationSPtr const & thisSPtr);            
            void OnPerformHealthCheckComplete(
                __in Common::AsyncOperationSPtr const & operation,
                __in bool expectedCompletedSynchronously);

            bool HealthCheckAsyncOperation::ModifyHealthCheckStoreData(
                __in Common::ErrorCode clusterHealthError,
                __in bool isClusterHealthy,
                __inout HealthCheckStoreData & healthCheckStoreData);

            void OnCommitComplete(
                __in Common::AsyncOperationSPtr const & operation,
                __in bool expectedCompletedSynchronously);

            void UpdateStore(__in Common::AsyncOperationSPtr const & thisSPtr, __in Common::ErrorCode clusterHealthError, __in bool isClusterHealthy);

            bool IsHealthCheckNeeded();

            RepairManagerServiceReplica & owner_;
        };
    }
}
