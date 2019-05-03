// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon
{
    using System.Collections.Generic;

    internal interface ICluster
    {
        /// <summary>
        /// The current state of the cluster.
        /// Can be null during initial provision before cluster manifest can be generated.
        /// </summary>
        ClusterState Current { get; set; }

        /// <summary>
        /// If not null, the state the cluster is transitioning to with the pending
        /// upgrade.  Null if there is no upgrade in progress.
        /// </summary>
        ClusterUpgradeStateBase Pending { get; set; }

        /// <summary>
        /// Null when no upgrade is pending and the current configuration is already the target.
        /// The value is set when there is a CSM update.
        /// </summary>
        IUserConfig TargetCsmConfig { get; set; }

        /// <summary>
        /// Null when no upgrade is pending and the current configuration is already the target.
        /// The value is set by the RolloutManager when it decides to upgrade the cluster.
        /// </summary>
        IAdminConfig TargetWrpConfig { get; set; }

        /// <summary>
        /// Null when no upgrade is pending and the current configuration is already the target.
        /// The value is set when NodeStatus is changed (as reported by UpgradeService)
        /// </summary>
        ClusterNodeConfig TargetNodeConfig { get; set; }

        IClusterTopology Topology { get; set; }

        string LastUpgradeFailureTimestampUtc { get; set; }

        /// <summary>
        /// Reset to false when TargetCSMConfig is updated.
        /// </summary>
        bool TargetCsmConfigUpgradeFailed { get; set; }

        /// <summary>
        /// Reset to false when TargetWRPConfig is updated.
        /// </summary>
        bool TargetWrpConfigUpgradeFailed { get; set; }

        ClusterUpgradeFailureReason WrpConfigUpgradeFailedReason { get; set; }

        ClusterProvisioningState ClusterState { get; set; }

        Dictionary<string, SortedList<string, NodeStatus>> NodeTypeNodeStatusList { get; set; }

        ClusterUpgradeStateFactory UpgradeFactory { get; set; }

        SeedNodeSelector SeedNodeSelector { get; set; }

        ClusterManifestGeneratorSettings ClusterManifestGeneratorSettings { get; set; }

        IFabricSettingsActivator FabricSettingsActivator { get; }

        int ClusterManifestVersion { get; set; }

        bool RunStateMachine();

        bool EnsureCurrentVersion(string clusterManifestVersion, string msiVersion);

        void ClusterUpgradeStarted();

        void ClusterUpgradeCompleted();

        void ClusterUpgradeRolledBackOrFailed(string upgradeFailureTimestampUtc, ClusterUpgradeFailureReason upgradeFailureReason);

        List<NodeStatus> UpdateNodesStatus(List<NodeStatus> nodesStatusUpdate);
    }
}