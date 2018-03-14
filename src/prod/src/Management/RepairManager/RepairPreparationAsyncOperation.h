// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace RepairManager
    {
        class RepairPreparationAsyncOperation :
            public Common::AsyncOperation,
            public Store::ReplicaActivityTraceComponent<Common::TraceTaskCodes::RepairManager>
        {
        public:

            RepairPreparationAsyncOperation(
                RepairManagerServiceReplica & owner,
                RepairTaskContext & context,
                Store::ReplicaActivityId const & activityId,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);

        protected:

            void OnStart(Common::AsyncOperationSPtr const & thisSPtr) override;

            void SendDeactivationRequest(Common::AsyncOperationSPtr const & thisSPtr);

            void OnDeactivationRequestComplete(
                Common::AsyncOperationSPtr const & operation,
                bool expectedCompletedSynchronously);

            void GetDeactivationStatus(Common::AsyncOperationSPtr const & thisSPtr);

            void OnDeactivationStatusComplete(
                Common::AsyncOperationSPtr const & operation,
                bool expectedCompletedSynchronously);

            void ApproveTask(Common::AsyncOperationSPtr const & thisSPtr);

            void OnApprovalComplete(
                Common::AsyncOperationSPtr const & operation,
                bool expectedCompletedSynchronously);

            Common::ErrorCode CreateDeactivateNodesRequest(
                __out std::map<Federation::NodeId, Reliability::NodeDeactivationIntent::Enum> & request) const;

            Common::ErrorCode GetNodeId(std::wstring & nodeName, __out Federation::NodeId & nodeId) const;

            Common::ErrorCode GetDeactivationIntent(
                NodeImpactLevel::Enum const impactLevel,
                __out Reliability::NodeDeactivationIntent::Enum & intent) const;

            void UpdateHealthCheckStartInfo(__in Common::AsyncOperationSPtr const & thisSPtr);
            void UpdateHealthCheckEndInfo(__in Common::AsyncOperationSPtr const & thisSPtr);
            
            void OnUpdateHealthCheckStartInfoComplete(
                __in Common::AsyncOperationSPtr const & operation,
                __in bool expectedCompletedSynchronously);
            void OnHealthCheckEndComplete(
                __in Common::AsyncOperationSPtr const & operation,
                __in bool expectedCompletedSynchronously);

            void UpdateHealthCheckResultInStore(__in Common::AsyncOperationSPtr const & thisSPtr);

        private:

            RepairManagerServiceReplica & owner_;
            RepairTaskContext & context_;
        };
    }
}
