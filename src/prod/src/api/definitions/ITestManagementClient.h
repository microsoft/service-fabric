// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    class ITestManagementClient
    {
    public:
        virtual ~ITestManagementClient() {};

        // Invoke Data Loss
        virtual Common::AsyncOperationSPtr BeginInvokeDataLoss(
            Management::FaultAnalysisService::InvokeDataLossDescription const &,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndInvokeDataLoss(
            Common::AsyncOperationSPtr const &) = 0;

        virtual Common::AsyncOperationSPtr BeginGetInvokeDataLossProgress(
            Common::Guid const & operationId,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndGetInvokeDataLossProgress(
            Common::AsyncOperationSPtr const &,
            __out IInvokeDataLossProgressResultPtr &result) = 0;

        // Invoke Quorum Loss
        virtual Common::AsyncOperationSPtr BeginInvokeQuorumLoss(
            Management::FaultAnalysisService::InvokeQuorumLossDescription const &,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndInvokeQuorumLoss(
            Common::AsyncOperationSPtr const &) = 0;

        virtual Common::AsyncOperationSPtr BeginGetInvokeQuorumLossProgress(
            Common::Guid const & operationId,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndGetInvokeQuorumLossProgress(
            Common::AsyncOperationSPtr const &,
            __out IInvokeQuorumLossProgressResultPtr &result) = 0;

        // Restart Partition
        virtual Common::AsyncOperationSPtr BeginRestartPartition(
            Management::FaultAnalysisService::RestartPartitionDescription const &,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndRestartPartition(
            Common::AsyncOperationSPtr const &) = 0;

        virtual Common::AsyncOperationSPtr BeginGetRestartPartitionProgress(
            Common::Guid const & operationId,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndGetRestartPartitionProgress(
            Common::AsyncOperationSPtr const &,
            __out IRestartPartitionProgressResultPtr &result) = 0;


        virtual Common::AsyncOperationSPtr BeginGetTestCommandStatusList(
            DWORD stateFilter,
            DWORD typeFilter,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) = 0;

        virtual Common::ErrorCode EndGetTestCommandStatusList(
            Common::AsyncOperationSPtr const &,
            __inout std::vector<ServiceModel::TestCommandListQueryResult> &result) = 0;

        virtual Common::AsyncOperationSPtr BeginCancelTestCommand(
            Management::FaultAnalysisService::CancelTestCommandDescription const &,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndCancelTestCommand(
            Common::AsyncOperationSPtr const &) = 0;

        // Chaos
        virtual Common::AsyncOperationSPtr BeginStartChaos(
            Management::FaultAnalysisService::StartChaosDescription &&,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndStartChaos(
            Common::AsyncOperationSPtr const &) = 0;

        virtual Common::AsyncOperationSPtr BeginStopChaos(
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndStopChaos(
            Common::AsyncOperationSPtr const &) = 0;

        virtual Common::AsyncOperationSPtr BeginGetChaos(
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndGetChaos(
            Common::AsyncOperationSPtr const &,
            __out IChaosDescriptionResultPtr &result) = 0;
        virtual Common::ErrorCode EndGetChaos(
            Common::AsyncOperationSPtr const &,
            __out ISystemServiceApiResultPtr &result) = 0;

        virtual Common::AsyncOperationSPtr BeginGetChaosReport(
            Management::FaultAnalysisService::GetChaosReportDescription const &,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndGetChaosReport(
            Common::AsyncOperationSPtr const &,
            __out IChaosReportResultPtr &result) = 0;

        virtual Common::AsyncOperationSPtr BeginGetChaosEvents(
            Management::FaultAnalysisService::GetChaosEventsDescription const &,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndGetChaosEvents(
            Common::AsyncOperationSPtr const &,
            __out IChaosEventsSegmentResultPtr &result) = 0;
        virtual Common::ErrorCode EndGetChaosEvents(
            Common::AsyncOperationSPtr const &,
            __out ISystemServiceApiResultPtr &result) = 0;

        virtual Common::AsyncOperationSPtr BeginStartNodeTransition(
            Management::FaultAnalysisService::StartNodeTransitionDescription const &,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndStartNodeTransition(
            Common::AsyncOperationSPtr const &) = 0;

        virtual Common::AsyncOperationSPtr BeginGetNodeTransitionProgress(
            Common::Guid const & operationId,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndGetNodeTransitionProgress(
            Common::AsyncOperationSPtr const &,
            __out INodeTransitionProgressResultPtr &result) = 0;

        // Chaos - Scheduler
	    virtual Common::AsyncOperationSPtr BeginGetChaosSchedule(
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndGetChaosSchedule(
            Common::AsyncOperationSPtr const &,
            __out IChaosScheduleDescriptionResultPtr &result) = 0;
        virtual Common::ErrorCode EndGetChaosSchedule(
            Common::AsyncOperationSPtr const &,
            __out ISystemServiceApiResultPtr &result) = 0;

        virtual Common::AsyncOperationSPtr BeginSetChaosSchedule(
            Management::FaultAnalysisService::SetChaosScheduleDescription const & setChaosScheduleDescription,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) = 0;
        virtual Common::AsyncOperationSPtr BeginSetChaosSchedule(
            std::wstring const & state,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndSetChaosSchedule(
            Common::AsyncOperationSPtr const &) = 0;
    };
}
