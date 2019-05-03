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

    using SectionName = StringConstants.SectionName;

    [JsonObject]
    internal abstract class ClusterUpgradeStateBase : IManifestVersionGenerator
    {
        private static readonly TraceType TraceType = new TraceType("ClusterUpgradeStateBase");

        private static readonly string[] AddOnServiceNames = new string[]
        {
            SectionName.RepairManager,
            SectionName.DnsService,
            SectionName.BackupRestoreService,
            SectionName.ResourceMonitorService,
            SectionName.SFVolumeDiskService,
            SectionName.EventStoreService,
            SectionName.GatewayResourceManager
        };

        private static ClusterUpgradePolicy forceUpgradePolicy = new ClusterUpgradePolicy
        {
            DeltaHealthPolicy = new ClusterUpgradeDeltaHealthPolicy() { MaxPercentDeltaUnhealthyApplications = 100, MaxPercentDeltaUnhealthyNodes = 100, MaxPercentUpgradeDomainDeltaUnhealthyNodes = 100 },
            HealthPolicy = new ClusterUpgradeHealthPolicy() { MaxPercentUnhealthyApplications = 100, MaxPercentUnhealthyNodes = 100 },
            UpgradeDomainTimeout = TimeSpan.FromHours(5),
            UpgradeTimeout = TimeSpan.FromHours(25),
            UpgradeReplicaSetCheckTimeout = TimeSpan.FromSeconds(60),
            HealthCheckWaitDuration = TimeSpan.FromSeconds(2),
            HealthCheckRetryTimeout = TimeSpan.FromMinutes(5),
            HealthCheckStableDuration = TimeSpan.FromSeconds(2),
        };
        
        [JsonConstructor]
        protected ClusterUpgradeStateBase()
        {
        }

        protected ClusterUpgradeStateBase(
            IUserConfig targetCsmConfig,
            IAdminConfig targetWrpConfig,
            ClusterNodeConfig targetNodeConfig,
            ICluster clusterResource,
            ITraceLogger traceLogger)
        {
            targetCsmConfig.MustNotBeNull("csmConfig");
            targetWrpConfig.MustNotBeNull("wrpConfig");
            targetNodeConfig.MustNotBeNull("nodeConfig");
            clusterResource.MustNotBeNull("clusterResource");

            this.TargetCsmConfig = targetCsmConfig;
            this.TargetWrpConfig = targetWrpConfig;
            this.TargetNodeConfig = targetNodeConfig;
            this.ClusterResource = clusterResource;
            this.TraceLogger = traceLogger;
        }

        [JsonProperty]
        public IUserConfig TargetCsmConfig { get; private set; }

        [JsonProperty]
        public IAdminConfig TargetWrpConfig { get; private set; }

        [JsonProperty]
        public ClusterNodeConfig TargetNodeConfig { get; set; }

        [JsonProperty]
        public ClusterExternalState ExternalState { get; protected set; }

        [JsonProperty]
        public ClusterExternalState PreviousExternalState { get; set; }

        [JsonProperty(IsReference = true)]
        public ICluster ClusterResource { get; set; }

        // Represents the number of times UpgradeService polls SFRP
        // without initiating upgrade after the pending state is set
        // Continous failure indicates some error and should fail the 
        // upgrade
        [JsonProperty]
        public int PollCountWithoutUpgrade { get; private set; }

        protected static ClusterUpgradePolicy ForceUpgradePolicy
        {
            get { return ClusterUpgradeStateBase.forceUpgradePolicy; }
            set { ClusterUpgradeStateBase.forceUpgradePolicy = value; }
        }

        protected abstract IClusterManifestBuilderActivator ClusterManifestBuilderActivator
        {
            get;
        }

        [JsonProperty]
        protected ITraceLogger TraceLogger { get; private set; }

        public bool IsBaselineUpgrade()
        {
            return this is BaselineClusterUpgradeStateBase;
        }

        public bool IsUserInitiatedUpgrade()
        {
            var currentState = this.ClusterResource.Current;
            if (currentState == null)
            {
                return false;
            }

            return !this.TargetCsmConfig.Version.Equals(this.ClusterResource.Current.CSMConfig.Version);
        }

        public bool IsAdminInitiatedUpgrade()
        {
            var currentState = this.ClusterResource.Current;
            if (currentState == null)
            {
                return false;
            }

            return !this.TargetWrpConfig.Equals(this.ClusterResource.Current.WRPConfig) && !this.TargetWrpConfig.IsUserSet;
        }

        public bool IsAutoScaleUpgrade()
        {
            var currentState = this.ClusterResource.Current;
            if (currentState == null)
            {
                return false;
            }

            return !this.TargetNodeConfig.Equals(this.ClusterResource.Current.NodeConfig);
        }

        public bool ShouldFailOnPollWithoutUpgrade()
        {
            return ++this.PollCountWithoutUpgrade > 15;
        }        

        // Determines if a new upgrade can interrupt the current
        // pending upgrade
        public virtual bool CanInterruptUpgrade()
        {
            return true;
        }

        public virtual void ValidateSettingChanges()
        {
        }
        
        public virtual ClusterUpgradePolicy GetUpgradePolicy()
        {
            ClusterUpgradePolicy result = ClusterUpgradeStateBase.ForceUpgradePolicy;
            if (!result.ForceRestart && this.IsForceRestartRequired())
            {
                result = result.Clone();
                result.ForceRestart = true;
            }

            return result;
        }

        public abstract bool StartProcessing();

        public abstract ClusterState ClusterUpgradeStarted();

        // This method is called by the state machine when the 
        // Cluster upgrade completes
        // The return ClusterState indicates if the target has been
        // reached
        public abstract ClusterState ClusterUpgradeCompleted();

        // This method is called by the state machine when the 
        // Cluster upgrade rolledback or failed
        // The return ClusterState indicates if the target rollback has been
        // reached
        public abstract ClusterState ClusterUpgradeRolledbackOrFailed();

        public abstract string GetNextClusterManifestVersion();

        internal static IEnumerable<string> GetFabricSectionNames(ClusterExternalState state)
        {
            if (state == null || state.ClusterManifest == null || state.ClusterManifest.FabricSettings == null)
            {
                return Enumerable.Empty<string>();
            }

            return state.ClusterManifest.FabricSettings.Select(p => p.Name);
        }

        internal static IEnumerable<string> GetChangedAddOnServices(
            IEnumerable<string> currentSectionNames,
            IEnumerable<string> previousSectionNames)
        {
            List<string> result = new List<string>();

            foreach (string svcName in ClusterUpgradeStateBase.AddOnServiceNames)
            {
                bool currentPresent = currentSectionNames.Any(p => p.Equals(svcName, StringComparison.OrdinalIgnoreCase));
                bool previousPresent = previousSectionNames.Any(p => p.Equals(svcName, StringComparison.OrdinalIgnoreCase));
                if (currentPresent != previousPresent)
                {
                    result.Add(svcName);
                }
            }

            return result;
        }

        internal virtual IEnumerable<NodeStatus> GetNodesToDisable(
            List<NodeStatus> disabledNodes)
        {
            if (disabledNodes != null)
            {
                this.TraceLogger.WriteError(TraceType, "Disabled nodes request is not null, disabled node count from upgrade service : {0}", disabledNodes.Count);
            }

            return null;
        }

        internal virtual IEnumerable<NodeStatus> GetNodesToEnable(
            List<NodeStatus> enabledNodes)
        {
            if (enabledNodes != null)
            {
                this.TraceLogger.WriteError(TraceType, "Enabled nodes request is not null, enabled node count from upgrade service : {0}", enabledNodes.Count);
            }

            return null;
        }

        internal virtual IDictionary<string, ReplicaSetSize> GetSystemServiceReplicaSetSize(IDictionary<string, ReplicaSetSize> currentSystemServicesSize)
        {
            return null;
        }

        internal bool IsForceRestartRequired()
        {
            var currentSectionNames = ClusterUpgradeStateBase.GetFabricSectionNames(this.ExternalState);
            var previousSectionNames = ClusterUpgradeStateBase.GetFabricSectionNames(this.PreviousExternalState);

            IEnumerable<string> changedAddOnServices = ClusterUpgradeStateBase.GetChangedAddOnServices(currentSectionNames, previousSectionNames);
            if (changedAddOnServices.Any())
            {
                this.TraceLogger.WriteInfo(
                    TraceType,
                    "Upgrade requires fabric restart. Changed add-on services: {0}",
                    string.Join(",", changedAddOnServices));

                return true;
            }

            return false;
        }

        protected void ResetTargetConfigs()
        {
            if (this.ClusterResource.TargetCsmConfig != null && 
                this.ClusterResource.TargetCsmConfig.Version.Equals(this.TargetCsmConfig.Version))
            {
                this.ClusterResource.TargetCsmConfig = null;
            }

            if (this.ClusterResource.TargetWrpConfig != null &&
                this.ClusterResource.TargetWrpConfig.Version.Equals(this.TargetWrpConfig.Version))
            {
                this.ClusterResource.TargetWrpConfig = null;
            }

            if (this.ClusterResource.TargetNodeConfig != null &&
                this.ClusterResource.TargetNodeConfig.Version.Equals(this.TargetNodeConfig.Version))
            {
                this.ClusterResource.TargetNodeConfig = null;
            }

            this.TraceLogger.WriteInfo(TraceType, "target config has been reset");
        }
    }
}
