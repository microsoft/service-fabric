// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Client
{
    class FabricClientImpl;
    typedef std::shared_ptr<FabricClientImpl> FabricClientImplSPtr;

    class ClientHealthReporting;
    typedef std::shared_ptr<ClientHealthReporting> ClientHealthReportingSPtr;

    class ApplicationUpgradeProgressResult;
    typedef std::shared_ptr<ApplicationUpgradeProgressResult> ApplicationUpgradeProgressResultSPtr;

    class UpgradeProgressResult;
    typedef std::shared_ptr<UpgradeProgressResult> UpgradeProgressResultSPtr;

    class PropertyBatchResult;
    typedef std::shared_ptr<PropertyBatchResult> PropertyBatchResultSPtr;

    class ResolvedServicePartitionResult;
    typedef std::shared_ptr<ResolvedServicePartitionResult> ResolvedServicePartitionResultSPtr;

    class ServiceAddressTracker;
    typedef std::shared_ptr<ServiceAddressTracker> ServiceAddressTrackerSPtr;

    class ServiceAddressTrackerManager;
    typedef std::shared_ptr<ServiceAddressTrackerManager> ServiceAddressTrackerManagerSPtr;

    class ServiceAddressTrackerCallbacks;
    typedef std::shared_ptr<ServiceAddressTrackerCallbacks> ServiceAddressTrackerCallbacksSPtr;

    class FabricClientInternalSettings;
    typedef std::shared_ptr<FabricClientInternalSettings> FabricClientInternalSettingsSPtr;

    class HealthReportingTransport;

    class HealthReportingComponent;
    typedef std::shared_ptr<HealthReportingComponent> HealthReportingComponentSPtr;

    typedef std::unique_ptr<HealthReportingComponent> HealthReportingComponentUPtr;

    class FileTransferClient;
    typedef std::unique_ptr<FileTransferClient> FileTransferClientUPtr;

    class IpcHealthReportingClient;
    typedef std::shared_ptr<IpcHealthReportingClient> IpcHealthReportingClientSPtr;

    class InvokeDataLossProgressResult;
    typedef std::shared_ptr<InvokeDataLossProgressResult> InvokeDataLossProgressResultSPtr;

    class InvokeQuorumLossProgressResult;
    typedef std::shared_ptr<InvokeQuorumLossProgressResult> InvokeQuorumLossProgressResultSPtr;

    class RestartPartitionProgressResult;
    typedef std::shared_ptr<RestartPartitionProgressResult> RestartPartitionProgressResultSPtr;

    class NodeTransitionProgressResult;
    typedef std::shared_ptr<NodeTransitionProgressResult> NodeTransitionProgressResultSPtr;

    class ChaosReportResult;
    typedef std::shared_ptr<ChaosReportResult> ChaosReportResultSPtr;

    class ChaosEventsSegmentResult;
    typedef std::shared_ptr<ChaosEventsSegmentResult> ChaosEventsSegmentResultSPtr;

    class ChaosDescriptionResult;
    typedef std::shared_ptr<ChaosDescriptionResult> ChaosDescriptionResultSPtr;

    class ChaosScheduleDescriptionResult;
    typedef std::shared_ptr<ChaosScheduleDescriptionResult> ChaosScheduleDescriptionResultPtr;

    class CallSystemServiceResult;
    typedef std::shared_ptr<CallSystemServiceResult> CallSystemServiceResultSPtr;

    class RestartNodeResult;
    typedef std::shared_ptr<RestartNodeResult> RestartNodeResultSPtr;

    class StartNodeResult;
    typedef std::shared_ptr<StartNodeResult> StartNodeResultSPtr;

    class StopNodeResult;
    typedef std::shared_ptr<StopNodeResult> StopNodeResultSPtr;

    class RestartDeployedCodePackageResult;
    typedef std::shared_ptr<RestartDeployedCodePackageResult> RestartDeployedCodePackageResultSPtr;

    class MovePrimaryResult;
    typedef std::shared_ptr<MovePrimaryResult> MovePrimaryResultSPtr;

    class MoveSecondaryResult;
    typedef std::shared_ptr<MoveSecondaryResult> MoveSecondaryResultSPtr;

    class OrchestrationUpgradeStatusResult;
    typedef std::shared_ptr<OrchestrationUpgradeStatusResult> OrchestrationUpgradeStatusResultSPtr;

    class UpgradeOrchestrationServiceStateResult;
    typedef std::shared_ptr<UpgradeOrchestrationServiceStateResult> UpgradeOrchestrationServiceStateResultSPtr;
}
