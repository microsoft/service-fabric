// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Api
{
    class IRepairManagementClient
    {
    public:
        virtual ~IRepairManagementClient() {};

        virtual Common::AsyncOperationSPtr BeginCreateRepairTask(
            Management::RepairManager::RepairTask const & repairTask,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;
        virtual Common::ErrorCode EndCreateRepairTask(
            Common::AsyncOperationSPtr const & operation,
            __out int64 & commitVersion) = 0;

        virtual Common::AsyncOperationSPtr BeginCancelRepairTask(
            Management::RepairManager::CancelRepairTaskMessageBody const & requestBody,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;
        virtual Common::ErrorCode EndCancelRepairTask(
            Common::AsyncOperationSPtr const & operation,
            __out int64 & commitVersion) = 0;

        virtual Common::AsyncOperationSPtr BeginForceApproveRepairTask(
            Management::RepairManager::ApproveRepairTaskMessageBody const & requestBody,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;
        virtual Common::ErrorCode EndForceApproveRepairTask(
            Common::AsyncOperationSPtr const & operation,
            __out int64 & commitVersion) = 0;

        virtual Common::AsyncOperationSPtr BeginDeleteRepairTask(
            Management::RepairManager::DeleteRepairTaskMessageBody const & requestBody,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;
        virtual Common::ErrorCode EndDeleteRepairTask(
            Common::AsyncOperationSPtr const & operation) = 0;

        virtual Common::AsyncOperationSPtr BeginUpdateRepairExecutionState(
            Management::RepairManager::RepairTask const & repairTask,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;
        virtual Common::ErrorCode EndUpdateRepairExecutionState(
            Common::AsyncOperationSPtr const & operation,
            __out int64 & commitVersion) = 0;

        virtual Common::AsyncOperationSPtr BeginGetRepairTaskList(
            Management::RepairManager::RepairScopeIdentifierBase & scope,
            std::wstring const & taskIdFilter,
            Management::RepairManager::RepairTaskState::Enum stateFilter,
            std::wstring const & executorFilter,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;
        virtual Common::ErrorCode EndGetRepairTaskList(
            Common::AsyncOperationSPtr const & operation,
            __out std::vector<Management::RepairManager::RepairTask> & result) = 0;

        virtual Common::AsyncOperationSPtr BeginUpdateRepairTaskHealthPolicy(
            Management::RepairManager::UpdateRepairTaskHealthPolicyMessageBody const & requestBody,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;
        virtual Common::ErrorCode EndUpdateRepairTaskHealthPolicy(
            Common::AsyncOperationSPtr const & operation,
            __out int64 & commitVersion) = 0;        
    };
}
