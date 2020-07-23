// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    class IClusterManagementClient
    {
    public:
        virtual ~IClusterManagementClient() {};

        virtual Common::AsyncOperationSPtr BeginDeactivateNode(
            std::wstring const &nodeName,
            FABRIC_NODE_DEACTIVATION_INTENT const deactivationIntent,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) = 0;
        virtual Common::ErrorCode EndDeactivateNode(
            Common::AsyncOperationSPtr const &parent) = 0;

        virtual Common::AsyncOperationSPtr BeginActivateNode(
            std::wstring const &nodeName,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) = 0;
        virtual Common::ErrorCode EndActivateNode(
            Common::AsyncOperationSPtr const &operation) = 0;

        virtual Common::AsyncOperationSPtr BeginDeactivateNodesBatch(
            std::map<Federation::NodeId, Reliability::NodeDeactivationIntent::Enum> const &,
            std::wstring const & batchId,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) = 0;
        virtual Common::ErrorCode EndDeactivateNodesBatch(
            Common::AsyncOperationSPtr const &parent) = 0;

        virtual Common::AsyncOperationSPtr BeginRemoveNodeDeactivations(
            std::wstring const & batchId,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) = 0;
        virtual Common::ErrorCode EndRemoveNodeDeactivations(
            Common::AsyncOperationSPtr const &parent) = 0;

        virtual Common::AsyncOperationSPtr BeginGetNodeDeactivationStatus(
            std::wstring const & batchId,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) = 0;
        virtual Common::ErrorCode EndGetNodeDeactivationStatus(
            Common::AsyncOperationSPtr const &parent,
            __out Reliability::NodeDeactivationStatus::Enum &) = 0;

        virtual Common::AsyncOperationSPtr BeginNodeStateRemoved(
            std::wstring const &nodeName,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) = 0;
        virtual Common::ErrorCode EndNodeStateRemoved(
            Common::AsyncOperationSPtr const &operation) = 0;

        virtual Common::AsyncOperationSPtr BeginRecoverPartitions(
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) = 0;
        virtual Common::ErrorCode EndRecoverPartitions(
            Common::AsyncOperationSPtr const &operation) = 0;

        virtual Common::AsyncOperationSPtr BeginRecoverPartition(
            Common::Guid partitionId,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) = 0;
        virtual Common::ErrorCode EndRecoverPartition(
            Common::AsyncOperationSPtr const &operation) = 0;

        virtual Common::AsyncOperationSPtr BeginRecoverServicePartitions(
            std::wstring const& serviceName,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) = 0;
        virtual Common::ErrorCode EndRecoverServicePartitions(
            Common::AsyncOperationSPtr const &operation) = 0;

        virtual Common::AsyncOperationSPtr BeginRecoverSystemPartitions(
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) = 0;
        virtual Common::ErrorCode EndRecoverSystemPartitions(
            Common::AsyncOperationSPtr const &operation) = 0;

        virtual Common::AsyncOperationSPtr BeginProvisionFabric(
            std::wstring const &codeFilepath,
            std::wstring const &clusterManifestFilepath,
            Common::TimeSpan const timeoutMilliseconds,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) = 0;
        virtual Common::ErrorCode EndProvisionFabric(
            Common::AsyncOperationSPtr const &operation) = 0;

        virtual Common::AsyncOperationSPtr BeginUpgradeFabric(
            ServiceModel::FabricUpgradeDescriptionWrapper const &upgradeDescription,
            Common::TimeSpan const timeoutMilliseconds,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) = 0;
        virtual Common::ErrorCode EndUpgradeFabric(
            Common::AsyncOperationSPtr const &operation) = 0;

        virtual Common::AsyncOperationSPtr BeginUpdateFabricUpgrade(
            Management::ClusterManager::FabricUpgradeUpdateDescription const &,
            Common::TimeSpan const timeoutMilliseconds,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) = 0;
        virtual Common::ErrorCode EndUpdateFabricUpgrade(
            Common::AsyncOperationSPtr const &operation) = 0;

        virtual Common::AsyncOperationSPtr BeginRollbackFabricUpgrade(
            Common::TimeSpan const timeoutMilliseconds,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) = 0;
        virtual Common::ErrorCode EndRollbackFabricUpgrade(
            Common::AsyncOperationSPtr const &operation) = 0;

        virtual Common::AsyncOperationSPtr BeginGetFabricUpgradeProgress(
            Common::TimeSpan const timeoutMilliseconds,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) = 0;
        virtual Common::ErrorCode EndGetFabricUpgradeProgress(
            Common::AsyncOperationSPtr const &operation,
            __inout IUpgradeProgressResultPtr &result) = 0;

        virtual Common::AsyncOperationSPtr BeginMoveNextFabricUpgradeDomain(
            IUpgradeProgressResultPtr const& progress,
            Common::TimeSpan const timeoutMilliseconds,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) = 0;
        virtual Common::ErrorCode EndMoveNextFabricUpgradeDomain(
            Common::AsyncOperationSPtr const &operation) = 0;

        virtual Common::AsyncOperationSPtr BeginMoveNextFabricUpgradeDomain2(
            std::wstring const &nextDomain,
            Common::TimeSpan const timeoutMilliseconds,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) = 0;
        virtual Common::ErrorCode EndMoveNextFabricUpgradeDomain2(
            Common::AsyncOperationSPtr const &operation) = 0;

       virtual Common::AsyncOperationSPtr BeginUnprovisionFabric(
            Common::FabricCodeVersion const &codeVersion,
            Common::FabricConfigVersion const &configVersion,
            Common::TimeSpan const timeoutMilliseconds,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) = 0;
        virtual Common::ErrorCode EndUnprovisionFabric(
            Common::AsyncOperationSPtr const &operation) = 0;

        virtual Common::AsyncOperationSPtr BeginGetClusterManifest(
            Management::ClusterManager::ClusterManifestQueryDescription const &,
            Common::TimeSpan const timeoutMilliseconds,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) = 0;
        virtual Common::ErrorCode EndGetClusterManifest(
            Common::AsyncOperationSPtr const &operation,
            __inout std::wstring &result) = 0;

        virtual Common::AsyncOperationSPtr BeginGetClusterVersion(
            Common::TimeSpan const timeoutMilliseconds,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) = 0;
        virtual Common::ErrorCode EndGetClusterVersion(
            Common::AsyncOperationSPtr const &operation,
            __inout std::wstring &result) = 0;

        virtual Common::AsyncOperationSPtr BeginStartInfrastructureTask(
            Management::ClusterManager::InfrastructureTaskDescription const&,
            Common::TimeSpan const timeoutMilliseconds,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) = 0;
        virtual Common::ErrorCode EndStartInfrastructureTask(
            Common::AsyncOperationSPtr const &operation,
            __out bool & isComplete) = 0;

        virtual Common::AsyncOperationSPtr BeginFinishInfrastructureTask(
            Management::ClusterManager::TaskInstance const&,
            Common::TimeSpan const timeoutMilliseconds,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) = 0;
        virtual Common::ErrorCode EndFinishInfrastructureTask(
            Common::AsyncOperationSPtr const &operation,
            __out bool & isComplete) = 0;

        virtual Common::AsyncOperationSPtr BeginAddUnreliableTransportBehavior(
            std::wstring const & nodeName,
            std::wstring const & name,
            std::wstring const & behaviorString,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) = 0;

        virtual Common::ErrorCode EndAddUnreliableTransportBehavior(
            Common::AsyncOperationSPtr const &) = 0;

        virtual Common::AsyncOperationSPtr BeginRemoveUnreliableTransportBehavior(
            std::wstring const & nodeName,
            std::wstring const & name,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) = 0;

        virtual Common::ErrorCode EndRemoveUnreliableTransportBehavior(
            Common::AsyncOperationSPtr const &) = 0;

        virtual Common::AsyncOperationSPtr BeginGetTransportBehaviorList(
            std::wstring const & nodeName,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) = 0;

        virtual Common::ErrorCode EndGetTransportBehaviorList(
            Common::AsyncOperationSPtr const &,
            __inout std::vector<std::wstring>& result) = 0;

        virtual Common::AsyncOperationSPtr BeginStopNode(
            std::wstring const & nodeName,
            std::wstring const & instanceId,
            bool restart,
            bool createFabricDump,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) = 0;

        virtual Common::ErrorCode EndStopNode(
            Common::AsyncOperationSPtr const &) = 0;

        virtual Common::AsyncOperationSPtr BeginRestartDeployedCodePackage(
            std::wstring const & nodeName,
            Common::NamingUri const & applicationName,
            std::wstring const & serviceManifestName,
            std::wstring const & codePackageName,
            std::wstring const & instanceId,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) = 0;

        virtual Common::ErrorCode EndRestartDeployedCodePackage(Common::AsyncOperationSPtr const &) = 0;

        virtual Common::AsyncOperationSPtr BeginStartNode(
            std::wstring const & nodeName,
            uint64 instanceId,
            std::wstring const & ipAddressOrFQDN,
            ULONG clusterConnectionPort,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) = 0;

        virtual Common::ErrorCode EndStartNode(
            Common::AsyncOperationSPtr const &) = 0;

        virtual Common::AsyncOperationSPtr BeginResetPartitionLoad(
            Common::Guid partitionId,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) = 0;
        virtual Common::ErrorCode EndResetPartitionLoad(
            Common::AsyncOperationSPtr const &operation) = 0;

        virtual Common::AsyncOperationSPtr BeginToggleVerboseServicePlacementHealthReporting(
            bool enabled,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) = 0;
        virtual Common::ErrorCode EndToggleVerboseServicePlacementHealthReporting(
            Common::AsyncOperationSPtr const &operation) = 0;

        // StartClusterConfigurationUpgrade
        virtual Common::AsyncOperationSPtr BeginUpgradeConfiguration(
            Management::UpgradeOrchestrationService::StartUpgradeDescription const &,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndUpgradeConfiguration(
            Common::AsyncOperationSPtr const &) = 0;

        // GetClusterConfigurationUpgradeStatus
        virtual Common::AsyncOperationSPtr BeginGetClusterConfigurationUpgradeStatus(
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndGetClusterConfigurationUpgradeStatus(
            Common::AsyncOperationSPtr const &,
            __out IFabricOrchestrationUpgradeStatusResultPtr &result) = 0;

        // GetClusterConfiguration
        virtual Common::AsyncOperationSPtr BeginGetClusterConfiguration(
            std::wstring const & apiVersion,
            Common::TimeSpan const timeoutMilliseconds,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) = 0;
        virtual Common::ErrorCode EndGetClusterConfiguration(
            Common::AsyncOperationSPtr const &operation,
            __inout std::wstring &result) = 0;

        // GetUpgradesPendingApproval
        virtual Common::AsyncOperationSPtr BeginGetUpgradesPendingApproval(
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndGetUpgradesPendingApproval(
            Common::AsyncOperationSPtr const &) = 0;

        // StartApprovedUpgrades
        virtual Common::AsyncOperationSPtr BeginStartApprovedUpgrades(
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndStartApprovedUpgrades(
            Common::AsyncOperationSPtr const &) = 0;

        // GetUpgradeOrchestrationServiceState
        virtual Common::AsyncOperationSPtr BeginGetUpgradeOrchestrationServiceState(
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;
        virtual Common::ErrorCode EndGetUpgradeOrchestrationServiceState(
            Common::AsyncOperationSPtr const &operation,
            __inout std::wstring &result) = 0;

		virtual Common::AsyncOperationSPtr BeginSetUpgradeOrchestrationServiceState(
			std::wstring const & state,
			Common::TimeSpan const timeout,
			Common::AsyncCallback const & callback,
			Common::AsyncOperationSPtr const & parent) = 0;

		virtual Common::ErrorCode EndSetUpgradeOrchestrationServiceState(
			Common::AsyncOperationSPtr const & operation,
			__inout Api::IFabricUpgradeOrchestrationServiceStateResultPtr & result) = 0;
    };
}
