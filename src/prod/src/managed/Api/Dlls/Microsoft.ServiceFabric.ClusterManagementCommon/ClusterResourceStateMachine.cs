// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Linq;

    using Newtonsoft.Json;

    internal class ClusterResourceStateMachine
    {
        private static readonly TraceType TraceType = new TraceType("ClusterResourceStateMachine");

        [JsonProperty(IsReference = true)]
        private ICluster clusterResource;
        private ITraceLogger traceLogger;

        public ClusterResourceStateMachine(ICluster clusterResource, ClusterUpgradeStateFactory upgradeFactory, ITraceLogger traceLogger)
        {
            clusterResource.MustNotBeNull("clusterResource");
            upgradeFactory.MustNotBeNull("upgradeFactory");
            traceLogger.MustNotBeNull("traceLogger");

            this.clusterResource = clusterResource;
            this.UpgradeFactory = upgradeFactory;
            this.traceLogger = traceLogger;
        }

        [JsonConstructor]
        private ClusterResourceStateMachine(ICluster clusterResource, ClusterUpgradeStateFactory upgradeFactory)
        {
            this.clusterResource = clusterResource;
            this.UpgradeFactory = upgradeFactory;
        }

        public ClusterUpgradeStateFactory UpgradeFactory
        {
            get;
            private set;
        }

        /// <summary>
        /// The caller should get a copy of the Cluster object and run it on the copy.
        /// If false is returned, nothing has changed and the caller can just discard
        /// the copy.
        /// If true is returned, the caller should persist the cluster object and replace
        /// the existing one.  And if Pending is not null, start the upgrade.
        /// </summary>
        public bool RunStateMachine()
        {
            this.traceLogger.WriteInfo(TraceType, "RunStateMachine: Enter RunStateMachine.");

            // Initital provision, can apply both CSM & WRP at the same time
            if (this.clusterResource.Current == null)
            {
                this.traceLogger.WriteInfo(TraceType, "RunStateMachine: TryConfigureBaselineUpgrade.");
                return this.TryConfigureBaselineUpgrade();
            }

            if (this.clusterResource.Pending != null)
            {
                this.traceLogger.WriteInfo(TraceType, "RunStateMachine: TryInterruptPendingUpgrade.");
                return this.TryInterruptPendingUpgrade();
            }

            this.traceLogger.WriteInfo(TraceType, "RunStateMachine: TryProcessUpgrade.");
            return this.TryProcessUpgrade();
        }

        public bool EnsureCurrentVersion(string clusterManifestVersion, string msiVersion)
        {
            if (this.clusterResource.Current != null)
            {
                // System.Fabric.Common.ReleaseAssert.AssertIf(this.clusterResource.Pending != null, string.Format("EnsureCurrentVersion: Pending should be null for Resource {0}", this.clusterResource.Id));

                // If a current version is set for the custer and there are no pending upgrades
                // then ensure that the cluster version matches the current version
                if (!this.clusterResource.Current.ExternalState.ClusterManifest.Version.Equals(clusterManifestVersion, StringComparison.OrdinalIgnoreCase) ||
                    !this.clusterResource.Current.ExternalState.MsiVersion.Equals(msiVersion, StringComparison.OrdinalIgnoreCase))
                {
                    this.traceLogger.WriteWarning(
                        TraceType,
                        "The version reported by the cluster does not match Current or Pending version. ClusterReportedVersion: {0}, CurrentVersion: {1}",
                        string.Format("{0}:{1}", msiVersion, clusterManifestVersion),
                        string.Format("{0}:{1}", this.clusterResource.Current.ExternalState.MsiVersion, this.clusterResource.Current.ExternalState.ClusterManifest.Version));

                    this.clusterResource.Pending = this.clusterResource.UpgradeFactory.CreateGatekeepingUpgradeState();

                    return true;
                }
            }

            return false;
        }

        public void ClusterUpgradeCompleted()
        {
            var currentClusterState = this.clusterResource.Pending.ClusterUpgradeCompleted();
            if (currentClusterState == null)
            {
                // An Upgrade is scheduled
                this.traceLogger.WriteInfo(TraceType, "ClusterUpgradeCompleted: More upgrades have been scheduled.");
                return;
            }

            this.clusterResource.Current = currentClusterState;
            this.clusterResource.Pending = null;

            // Run the state machine again to check whether a new upgrade is needed.
            this.RunStateMachine();
        }

        public void ClusterUpgradeRolledBackOrFailed(string upgradeFailureTimestampUtc, ClusterUpgradeFailureReason upgradeFailureReason)
        {
            this.traceLogger.WriteWarning(TraceType, "Enter ClusterUpgradeRolledBackOrFailed.");

            var currentClusterState = this.clusterResource.Pending.ClusterUpgradeRolledbackOrFailed();
            if (currentClusterState == null)
            {
                // An Upgrade is scheduled
                return;
            }

            if (this.clusterResource.Pending.IsAdminInitiatedUpgrade())
            {
                this.clusterResource.WrpConfigUpgradeFailedReason = upgradeFailureReason;
            }

            this.clusterResource.Current = currentClusterState;
            this.clusterResource.Pending = null;
            this.clusterResource.LastUpgradeFailureTimestampUtc = upgradeFailureTimestampUtc;

            // if (this.clusterResource.IsUpgradeServiceDown)
            // {
            //    this.clusterResource.ClusterState = ClusterProvisioningState.UpgradeServiceUnreachable;
            // }

            // Run the state machine again to check whether a new upgrade is needed.
            this.clusterResource.RunStateMachine();
        }

        public List<NodeStatus> UpdateNodesStatus(List<NodeStatus> nodesStatusUpdate)
        {
            List<NodeStatus> processedNodeStatus = new List<NodeStatus>();
            if (nodesStatusUpdate != null && nodesStatusUpdate.Any())
            {
                foreach (var nodeStatusUpdate in nodesStatusUpdate)
                {
                    SortedList<string, NodeStatus> existingNodeType;
                    if (!this.clusterResource.NodeTypeNodeStatusList.TryGetValue(nodeStatusUpdate.NodeType, out existingNodeType))
                    {
                        this.traceLogger.WriteWarning(TraceType, "NodeType {0} is not present in NodeTypeNodeStatusList", nodeStatusUpdate.NodeType);
                        continue;
                    }

                    existingNodeType[nodeStatusUpdate.NodeName] = nodeStatusUpdate;
                    processedNodeStatus.Add(nodeStatusUpdate);
                }

                var nodeStatus = GetNodeStatusList(this.clusterResource);
                long currentLatestNodeVersion = this.clusterResource.TargetNodeConfig != null ? this.clusterResource.TargetNodeConfig.Version : this.clusterResource.Current.NodeConfig.Version;
                this.clusterResource.TargetNodeConfig = new ClusterNodeConfig(nodeStatus, ++currentLatestNodeVersion);
            }

            if (this.clusterResource.Pending == null)
            {
                // RunStateMachine only when there is no pending upgrade
                this.RunStateMachine();
            }

            return processedNodeStatus;
        }

        private static List<NodeStatus> GetNodeStatusList(ICluster cluster)
        {
            List<NodeStatus> nodeStatusList = new List<NodeStatus>();
            foreach (var nodeType in cluster.NodeTypeNodeStatusList)
            {
                if (nodeType.Value == null)
                {
                    continue;
                }

                nodeStatusList.AddRange(nodeType.Value.Select(item => item.Value));
            }

            return nodeStatusList;
        }

        private bool TryConfigureBaselineUpgrade()
        {
            // Create the TargetNodeConfig based on current node status
            List<NodeStatus> nodeStatus = GetNodeStatusList(this.clusterResource);
            this.clusterResource.TargetNodeConfig = new ClusterNodeConfig(nodeStatus, DateTime.UtcNow.Ticks);

            ClusterUpgradeStateBase pendingUpgrade;
            if (!this.UpgradeFactory.TryCreateUpgradeState(
                    this.clusterResource.TargetCsmConfig,
                    this.clusterResource.TargetWrpConfig,
                    this.clusterResource.TargetNodeConfig,
                    out pendingUpgrade))
            {
                // The cluster manifest is not get generate and the Cluster is
                // "WaitingForNodes". Accept the new request and complete the PUT
                // request in this.clusterResource condition

                // If we do not set ProvisionState to Succeeded, ARM will keep retrying PUT on this cluster resource
                // which will block PUT on VM extension. Hence if VM extension fails and ARM retries the full deployment,
                // we will deadlock since new nodes have to come up for ClusterResource to get out of "WaitingForNodes"
                return false;
            }

            if (!(pendingUpgrade is BaselineClusterUpgradeStateBase))
            {
                throw new ValidationException(
                    ClusterManagementErrorCode.OperationInconsistentWithStateMachine, 
                    string.Format("Baseline upgrade has to be generated when Current is null. Actual: {0}", pendingUpgrade.GetType()));
            }

            var clusterState = pendingUpgrade.ClusterUpgradeStarted();
            
            if (clusterState != null)
            {
                throw new ValidationException(
                    ClusterManagementErrorCode.OperationInconsistentWithStateMachine, 
                    "Baseline ClusterUpgradeStarted should return null clusterState");
            }

            this.clusterResource.Pending = pendingUpgrade;

            this.clusterResource.Current = new ClusterState(pendingUpgrade.TargetCsmConfig, pendingUpgrade.TargetWrpConfig, pendingUpgrade.TargetNodeConfig, pendingUpgrade.ExternalState);

            return true;
        }

        private bool TryInterruptPendingUpgrade()
        {
            if (!this.clusterResource.Pending.CanInterruptUpgrade())
            {
                this.traceLogger.WriteInfo(TraceType, "TryInterruptPendingUpgrade: Can not interrupt pending upgrade since Pending.CanInterruptUpgrade is false");
                return false;
            }

            var pendingCsmConfig = this.clusterResource.Pending.TargetCsmConfig;
            var pendingWrpConfig = this.clusterResource.Pending.TargetWrpConfig;
            var pendingNodeConfig = this.clusterResource.Pending.TargetNodeConfig;

            if (this.clusterResource.Pending.IsUserInitiatedUpgrade())
            {
                if (this.clusterResource.TargetCsmConfig == null)
                {
                    throw new ValidationException(
                        ClusterManagementErrorCode.OperationInconsistentWithStateMachine, 
                        "this.clusterResource.TargetCsmConfig cannot be null");
                }

                if (!this.clusterResource.TargetCsmConfig.Version.Equals(pendingCsmConfig.Version) &&
                    !this.clusterResource.TargetCsmConfigUpgradeFailed)
                {
                    pendingCsmConfig = this.clusterResource.TargetCsmConfig;
                    if (this.clusterResource.TargetNodeConfig != null && this.clusterResource.TargetNodeConfig.IsUserSet)
                    {
                        // If the current target node config is set by user
                        // then set it as part of the pending upgrade
                        pendingNodeConfig = this.clusterResource.TargetNodeConfig;
                    }
                }

                if (this.clusterResource.TargetWrpConfig != null && this.clusterResource.TargetWrpConfig.IsUserSet)
                {
                    // If the current target wrp config is set by user
                    // then set it as part of the pending upgrade
                    pendingWrpConfig = this.clusterResource.TargetWrpConfig;
                }
                else if (pendingWrpConfig.IsUserSet)
                {
                    // If the upgrade to be interrupted contains user initiated wrp config, then
                    // use wrp config of current since all of user's upgrade has to be interrupted
                    pendingWrpConfig = this.clusterResource.Current.WRPConfig;
                }
            }
            else if (this.clusterResource.Pending.IsAdminInitiatedUpgrade())
            {
                if (this.clusterResource.TargetWrpConfig == null)
                {
                    throw new ValidationException(
                        ClusterManagementErrorCode.OperationInconsistentWithStateMachine, 
                        "this.clusterResource.TargetWrpConfig cannot be null");
                }

                // Current upgrade is driven from WRP
                if (!this.clusterResource.TargetWrpConfig.Version.Equals(pendingWrpConfig.Version) &&
                    !this.clusterResource.TargetWrpConfigUpgradeFailed)
                {
                    pendingWrpConfig = this.clusterResource.TargetWrpConfig;
                    if (this.clusterResource.TargetWrpConfig.IsUserSet)
                    {
                        ReleaseAssert.AssertIfNull(this.clusterResource.TargetCsmConfig, "TargetCsmConfig should not be null when TargetWrpConfig is user set.");
                        
                        // If the interrupting wrp config is set by user
                        // then set the target csm config as well
                        pendingCsmConfig = this.clusterResource.TargetCsmConfig;
                    }
                }
            }
            else if (this.clusterResource.Pending.IsAutoScaleUpgrade())
            {
                if (this.clusterResource.TargetNodeConfig == null)
                {
                    throw new ValidationException(
                        ClusterManagementErrorCode.OperationInconsistentWithStateMachine, 
                        "this.clusterResource.TargetNodeConfig cannot be null");
                }

                // Current upgrade is auto-scale upgrade
                if (!this.clusterResource.TargetNodeConfig.Version.Equals(pendingNodeConfig.Version))
                {
                    pendingNodeConfig = this.clusterResource.TargetNodeConfig;
                }
            }

            if (pendingCsmConfig.Version.Equals(this.clusterResource.Pending.TargetCsmConfig.Version) &&
                pendingWrpConfig.Version.Equals(this.clusterResource.Pending.TargetWrpConfig.Version) &&
                pendingNodeConfig.Version.Equals(this.clusterResource.Pending.TargetNodeConfig.Version))
            {
                this.traceLogger.WriteInfo(TraceType, "TryInterruptPendingUpgrade: No need to interrupt pending upgrade since the target config versions are the same");
                return false;
            }

            ClusterUpgradeStateBase pendingUpgrade;
            if (!this.UpgradeFactory.TryCreateUpgradeState(
                    pendingCsmConfig,
                    pendingWrpConfig,
                    pendingNodeConfig,
                    out pendingUpgrade))
            {
                this.traceLogger.WriteInfo(TraceType, "TryInterruptPendingUpgrade: Tried to interrupt but fails since UpgradeFactory.TryCreateUpgradeState returns false");
                return false;
            }

            var clusterState = pendingUpgrade.ClusterUpgradeStarted();

            if (clusterState != null)
            {
                throw new ValidationException(
                    ClusterManagementErrorCode.OperationInconsistentWithStateMachine, 
                    "ClusterUpgradeStarted should return null clusterState when there is a PendingUpgrade");
            }

            this.traceLogger.WriteInfo(TraceType, "TryInterruptPendingUpgrade: Interrupt successfully");

            this.clusterResource.Pending = pendingUpgrade;
            return true;
        }

        private bool TryProcessUpgrade()
        {
            bool updated = false;
            while (this.clusterResource.Pending == null)
            {
                IUserConfig csmConfig = null;
                IAdminConfig wrpConfig = null;
                ClusterNodeConfig nodeConfig = null;
                if (this.clusterResource.TargetCsmConfig != null
                    && !this.clusterResource.TargetCsmConfigUpgradeFailed)
                {
                    csmConfig = this.clusterResource.TargetCsmConfig;
                    if (this.clusterResource.TargetWrpConfig != null && this.clusterResource.TargetWrpConfig.IsUserSet)
                    {
                        // If the current target wrp config is set by user
                        // then the set it as part of the pending upgrade
                        wrpConfig = this.clusterResource.TargetWrpConfig;
                    }

                    if (this.clusterResource.TargetNodeConfig != null && this.clusterResource.TargetNodeConfig.IsUserSet)
                    {
                        // If the current target node config is set by user
                        // then set it as part of the pending upgrade
                        nodeConfig = this.clusterResource.TargetNodeConfig;
                    }
                }
                else
                {
                    csmConfig = this.clusterResource.Current.CSMConfig;
                }

                if (wrpConfig == null)
                {
                    if (csmConfig == this.clusterResource.Current.CSMConfig &&
                    this.clusterResource.TargetWrpConfig != null &&
                    !this.clusterResource.TargetWrpConfigUpgradeFailed)
                    {
                        wrpConfig = this.clusterResource.TargetWrpConfig;
                    }
                    else
                    {
                        wrpConfig = this.clusterResource.Current.WRPConfig;
                    }
                }

                if (nodeConfig == null)
                {
                    if (csmConfig == this.clusterResource.Current.CSMConfig &&
                    wrpConfig == this.clusterResource.Current.WRPConfig &&
                    !this.clusterResource.TargetCsmConfigUpgradeFailed &&
                    this.clusterResource.TargetNodeConfig != null)
                    {
                        nodeConfig = this.clusterResource.TargetNodeConfig;
                    }
                    else
                    {
                        nodeConfig = this.clusterResource.Current.NodeConfig;
                    }
                }
                
                if (csmConfig == this.clusterResource.Current.CSMConfig &&
                    wrpConfig == this.clusterResource.Current.WRPConfig &&
                    nodeConfig == this.clusterResource.Current.NodeConfig)
                {
                    // No new target selected
                    this.traceLogger.WriteInfo(
                        TraceType,
                        "TryProcessUpgrade: No new target selected. CsmConfig changed: {0}. WrpConfig changed: {1}. NodeConfig changed: {2}, CsmUpgradeFailed: {3}. WrpUpgradeFailed: {4}",
                        csmConfig != this.clusterResource.TargetCsmConfig,
                        wrpConfig != this.clusterResource.TargetWrpConfig,
                        nodeConfig != this.clusterResource.TargetNodeConfig,
                        this.clusterResource.TargetCsmConfigUpgradeFailed,
                        this.clusterResource.TargetWrpConfigUpgradeFailed);
                    return updated;
                }

                ClusterUpgradeStateBase pendingUpgrade;
                if (!this.UpgradeFactory.TryCreateUpgradeState(csmConfig, wrpConfig, nodeConfig, out pendingUpgrade))
                {
                    this.traceLogger.WriteInfo(TraceType, "TryProcessUpgrade: Fail to create upgrade state");
                    return updated;
                }

                updated = true;

                var clusterState = pendingUpgrade.ClusterUpgradeStarted();
                if (clusterState != null)
                {
                    this.clusterResource.Current = clusterState;
                    this.clusterResource.Pending = null;
                }
                else
                {
                    this.clusterResource.Pending = pendingUpgrade;
                }
            }

            this.traceLogger.WriteInfo(TraceType, "TryProcessUpgrade: Succeed to create upgrade state: {0}", this.clusterResource.Pending.GetType().ToString());
            return updated;
        }
    }
}