// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon
{
    using System;
    using System.Collections.Generic;
    using System.Linq; 
    using Newtonsoft.Json;

    [JsonObject]
    internal abstract class AutoScaleClusterUpgradeStateBase : ClusterUpgradeStateBase
    {
        private static readonly TraceType TraceType = new TraceType("AutoScaleClusterUpgradeState");

        // This state does not need to be persisted since if there is no real upgrade required
        // the upgrade will be completed even before the state is persisted
        private bool skipSystemServicesReplicaScale;

        [JsonConstructor]
        protected AutoScaleClusterUpgradeStateBase()
        {
        }

        protected AutoScaleClusterUpgradeStateBase(
            IUserConfig targetCsmConfig,
            IAdminConfig targetWrpConfig,
            ClusterNodeConfig targetNodeConfig,
            ICluster clusterResource,
            ITraceLogger traceLogger)
        : base(targetCsmConfig, targetWrpConfig, targetNodeConfig, clusterResource, traceLogger)
        {
        }

        // This property does not need to be persisted
        internal Dictionary<string, ReplicaSetSize> TargetSystemServicesSize { get; set; }

        [JsonProperty]
        internal MultiphaseClusterUpgradeState SeedNodeUpgradeState { get; set; }

        public override bool CanInterruptUpgrade()
        {
            return false;
        }

        public override bool StartProcessing()
        {
            this.TraceLogger.WriteInfo(
                TraceType,
                "Start@StartProcessing: TargetNodeVersion: {0}, CurrentNodeVersion: {1}",
                this.TargetNodeConfig.Version,
                this.ClusterResource.Current.NodeConfig.Version);

            var disabledNodes = this.TargetNodeConfig.NodesStatus
                .Where(nodeStatus => nodeStatus.NodeDeactivationIntent == WrpNodeDeactivationIntent.RemoveNode)
                .Select(nodeStatus => nodeStatus.NodeName);

            var removedNodes = this.TargetNodeConfig.NodesStatus
                .Where(nodeStatus => nodeStatus.NodeState == NodeState.Removed)
                .Select(nodeStatus => nodeStatus.NodeName);

            MultiphaseClusterUpgradeState seedNodeUpgradeState;
            if (!this.ClusterResource.UpgradeFactory.TryCreateSeedNodeUpgradeState(
                this.TargetCsmConfig,
                this.TargetWrpConfig,
                this.TargetNodeConfig,
                new List<string>(disabledNodes),
                new List<string>(removedNodes),
                out seedNodeUpgradeState))
            {
                return false;
            }
            else
            {               
                this.SeedNodeUpgradeState = seedNodeUpgradeState;
                this.Validate();
                this.SeedNodeUpgradeState.ClusterUpgradeStarted();

                if (this.SeedNodeUpgradeState.ExternalState.Equals(this.SeedNodeUpgradeState.PreviousExternalState))
                {
                    // There is no seed node upgrade to do.
                    // Assume that there is no system service scale required either since they are currently tied together.                     
                    // Change logic if we have a condition where system services scale is required even when seed node change
                    // is not required.
                    this.skipSystemServicesReplicaScale = true;
                }

                this.ExternalState = this.SeedNodeUpgradeState.ExternalState;
                this.PreviousExternalState = this.SeedNodeUpgradeState.PreviousExternalState;
            }

            return true;
        }

        public override ClusterState ClusterUpgradeStarted()
        {
            var clusterState = this.SeedNodeUpgradeState.ClusterUpgradeStarted();
            if (clusterState != null)
            {
                this.ResetTargetConfigs();
                return new ClusterState(this.TargetCsmConfig, this.TargetWrpConfig, this.TargetNodeConfig, this.ExternalState);
            }

            this.OnAutoScaleUpgradeStarted();

            return null;
        }

        public override ClusterState ClusterUpgradeCompleted()
        {
            if (this.SeedNodeUpgradeState != null)
            {
                var clusterState = this.SeedNodeUpgradeState.ClusterUpgradeCompleted();
                if (clusterState == null)
                {
                    // SeedNode upgrade has not completed yet
                    this.ExternalState = this.SeedNodeUpgradeState.ExternalState;
                    this.PreviousExternalState = this.SeedNodeUpgradeState.PreviousExternalState;

                    return null;
                }
                else
                {
                    // SeedNode upgrade has completed
                    this.SeedNodeUpgradeState = null;
                }
            }

            if (!this.skipSystemServicesReplicaScale)
            {
                // Initiate system services replica set size adjustment
                // once SeedNode upgrade is done
                this.SetTargetSystemServicesReplicaSetSize();
            }

            if (this.skipSystemServicesReplicaScale || this.VerifyTargetSystemServicesReplicaSetSize())
            {
                this.TraceLogger.WriteInfo(
                    TraceType,
                    "Autoscale cluster upgrade completed. TargetCsmVersion: {0}, TargetWrpVersion: {1}, TargetNodeVersion",
                    this.TargetCsmConfig.Version,
                    this.TargetWrpConfig.Version,
                    this.TargetNodeConfig.Version);

                this.OnAutoScaleUpgradeCompleted();

                this.ResetTargetConfigs();

                return new ClusterState(this.TargetCsmConfig, this.TargetWrpConfig, this.TargetNodeConfig, this.ExternalState);
            }

            return null;
        }

        public override ClusterState ClusterUpgradeRolledbackOrFailed()
        {
            this.TraceLogger.WriteWarning(
                TraceType,
                "AutoScaleClusterUpgrade rolledback or failed. Fail the upgrade in its current state.");

            // Fail the upgrade in its current state
            this.ClusterResource.TargetCsmConfigUpgradeFailed = true;

            // Must set the external state to match the current true state of the cluster
            this.ClusterResource.Current.ExternalState = this.ClusterResource.Pending.PreviousExternalState;
            return this.ClusterResource.Current;
        }

        internal override IDictionary<string, ReplicaSetSize> GetSystemServiceReplicaSetSize(IDictionary<string, ReplicaSetSize> currentSystemServicesSize)
        {
            if (this.TargetSystemServicesSize == null)
            {
                return null;
            }

            if (currentSystemServicesSize == null)
            {
                return this.TargetSystemServicesSize;
            }

            Dictionary<string, ReplicaSetSize> updatedTargetSystemServicesSize = new Dictionary<string, ReplicaSetSize>(StringComparer.OrdinalIgnoreCase);
            foreach (var currentSystemServiceSize in currentSystemServicesSize)
            {
                var systemServiceName = currentSystemServiceSize.Key;
                var currentReplicaSetSize = currentSystemServiceSize.Value;

                ReplicaSetSize targetReplicaSetSize;
                if (this.TargetSystemServicesSize.TryGetValue(systemServiceName, out targetReplicaSetSize))
                {
                    if (!targetReplicaSetSize.Equals(currentReplicaSetSize))
                    {
                        // system service has not reached target. Send the target size in response
                        updatedTargetSystemServicesSize.Add(currentSystemServiceSize.Key, targetReplicaSetSize);
                    }
                    else
                    {
                        this.TraceLogger.WriteInfo(
                            TraceType,
                            "SystemService {0} has reached target size {1}",
                            systemServiceName,
                            targetReplicaSetSize);
                    }
                }
            }

            // The update target is the new target.
            // This also takes care of removing system services in TargetSystemServicesSize
            // but not present in the cluster. They should no longer be in target.
            this.TargetSystemServicesSize = updatedTargetSystemServicesSize;

            return updatedTargetSystemServicesSize.Any() ? updatedTargetSystemServicesSize : null;
        }

        protected virtual void Validate()
        {
        }

        protected abstract ClusterState OnAutoScaleUpgradeCompleted();

        protected abstract ClusterState OnAutoScaleUpgradeStarted();

        private void SetTargetSystemServicesReplicaSetSize()
        {
            if (this.TargetSystemServicesSize != null)
            {
                // The target is already set
                return;
            }

            Dictionary<string, ReplicaSetSize> systemServicesReplicaSetSize = new Dictionary<string, ReplicaSetSize>(StringComparer.OrdinalIgnoreCase);

            var systemServicesSupportedForScale = this.ClusterResource.ClusterManifestGeneratorSettings.SystemServicesForScale;
            ReplicaSetSize replicaSetSizeForReliability = this.TargetCsmConfig.ReliabilityLevel.GetReplicaSetSize();

            foreach (var systemServiceSuppotedForScale in systemServicesSupportedForScale)
            {
                systemServicesReplicaSetSize.Add(systemServiceSuppotedForScale.Value, replicaSetSizeForReliability);
            }

            // InfrastructureService name is not well known like other system services. Hence it is handled seaprately
            foreach (var nodeType in this.TargetCsmConfig.NodeTypes)
            {
                if (nodeType.DurabilityLevel != DurabilityLevel.Bronze)
                {
                    // IS is enabled. Hence adjust its replica set size
                    var infraSvcName = string.Format("fabric:/System/{0}/{1}", StringConstants.SectionName.InfrastructureServicePrefix, nodeType.Name);
                    systemServicesReplicaSetSize.Add(infraSvcName, replicaSetSizeForReliability);
                }
            }

            this.TargetSystemServicesSize = systemServicesReplicaSetSize;

            this.TraceLogger.WriteInfo(
                TraceType,
                "Initiating adjustment of system service replica set size.");
        }

        private bool VerifyTargetSystemServicesReplicaSetSize()
        {
            if (this.TargetSystemServicesSize == null || this.TargetSystemServicesSize.Any())
            {
                return false;
            }
            else
            {
                return true;
            }
        }
    }
}