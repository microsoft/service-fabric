// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon
{
    using System.Fabric.Management.ServiceModel;

    using Newtonsoft.Json;

    [JsonObject]
    internal abstract class MultiphaseClusterUpgradeState : ClusterUpgradeStateBase
    {
        private static readonly TraceType TraceType = new TraceType("MultiphaseClusterUpgradeState");

        [JsonConstructor]
        protected MultiphaseClusterUpgradeState()
        {
        }

        protected MultiphaseClusterUpgradeState(
            IUserConfig targetCsmConfig,
            IAdminConfig targetWrpConfig,
            ClusterNodeConfig targetNodeConfig,
            ICluster clusterResource,
            ITraceLogger traceLogger)
        : base(targetCsmConfig, targetWrpConfig, targetNodeConfig, clusterResource, traceLogger)
        {
        }

        [JsonProperty]
        protected internal ClusterManifestType[] ClusterManifestList { get; set; }

        [JsonProperty]
        protected internal int CurrentListIndex { get; set; }

        /// <summary>
        /// true means the upgrade sequence is in the rollback direction
        /// </summary>
        [JsonProperty]
        protected internal bool UpgradeUnsuccessful { get; set; }

        public override bool CanInterruptUpgrade()
        {
            // The first upgrade can be interrupted
            return this.CurrentListIndex == 0;
        }

        public override bool StartProcessing()
        {
            this.TraceLogger.WriteInfo(TraceType, "MultiphaseClusterUpgradeState: Enter StartProcessing.");

            ClusterManifestType[] clusterManifestList = this.OnStartProcessingMultiphaseUpgrade();
            if (clusterManifestList == null || clusterManifestList.Length == 0)
            {
                this.TraceLogger.WriteInfo(TraceType, "MultiphaseClusterUpgradeState: No cluster manifest to process.");
                return false;
            }

            this.ClusterManifestList = clusterManifestList;

            this.ExternalState = new ClusterExternalState(
                this.ClusterManifestList[0],
                this.ClusterResource.Current.ExternalState.MsiVersion);
            this.PreviousExternalState = this.ClusterResource.Current.ExternalState;

            return true;
        }

        public override ClusterState ClusterUpgradeStarted()
        {
            this.TraceLogger.WriteInfo(TraceType, "MultiphaseClusterUpgradeState: Enter ClusterUpgradeStarted.");
            if (!this.OnMultiphaseUpgradeStarted())
            {
                this.ResetTargetConfigs();

                return new ClusterState(this.TargetCsmConfig, this.TargetWrpConfig, this.TargetNodeConfig, this.ClusterResource.Current.ExternalState);
            }

            return null;
        }

        public override ClusterState ClusterUpgradeCompleted()
        {
            this.TraceLogger.WriteInfo(TraceType, "MultiphaseClusterUpgradeState: Enter ClusterUpgradeCompleted. The current phase is {0}", this.CurrentListIndex);

            if (this.UpgradeUnsuccessful)
            {
                if (this.CurrentListIndex == 1)
                {
                    this.TraceLogger.WriteInfo(TraceType, "MultiphaseClusterUpgradeState: Rolling back. All phases have been completed.");
                    this.OnMultiphaseUpgradeRolledbackOrFailedCompleted();
                    return this.ClusterResource.Current;
                }
                else
                {
                    --this.CurrentListIndex;
                    this.TraceLogger.WriteInfo(TraceType, "MultiphaseClusterUpgradeState: Rolling back. The next phase is {0}.", this.CurrentListIndex);
                }
            }
            else
            {
                if (this.ClusterManifestList.Length == this.CurrentListIndex + 1)
                {
                    this.TraceLogger.WriteInfo(TraceType, "MultiphaseClusterUpgradeState: Rolling forward. All phases have been completed.");

                    //// All items in the ClusterManifest has been consumed.
                    this.OnMultiphaseUpgradeCompleted();

                    this.ResetTargetConfigs();

                    return new ClusterState(this.TargetCsmConfig, this.TargetWrpConfig, this.TargetNodeConfig, this.ExternalState);
                }
                else
                {
                    ++this.CurrentListIndex;
                    this.TraceLogger.WriteInfo(TraceType, "MultiphaseClusterUpgradeState: Rolling forward. The next phase is {0}.", this.CurrentListIndex);
                }
            }

            this.PreviousExternalState = this.ExternalState;
            this.ExternalState = new ClusterExternalState(
                this.ClusterManifestList[this.CurrentListIndex],
                this.ExternalState.MsiVersion);

            return null;
        }

        public override ClusterState ClusterUpgradeRolledbackOrFailed()
        {
            this.TraceLogger.WriteInfo(TraceType, "MultiphaseClusterUpgradeState: Enter ClusterUpgradeRolledbackOrFailed.");

            if (this.CurrentListIndex == 0)
            {
                this.OnMultiphaseUpgradeRolledbackOrFailedCompleted();
                return this.ClusterResource.Current;
            }
            else if (this.UpgradeUnsuccessful)
            {
                // retry
                this.TraceLogger.WriteInfo(TraceType, "MultiphaseClusterUpgradeState: Rolling back. The current phase is {0}. Retry started.", this.CurrentListIndex);
                return null;
            }
            else
            {
                this.UpgradeUnsuccessful = true;
                this.TraceLogger.WriteInfo(TraceType, "MultiphaseClusterUpgradeState: Rolling back. The current phase is {0}.", this.CurrentListIndex);

                this.PreviousExternalState = this.ExternalState;
                if (this.CurrentListIndex == 1)
                {
                    this.ExternalState = new ClusterExternalState(
                        this.ClusterResource.Current.ExternalState.ClusterManifest,
                        this.ClusterResource.Current.ExternalState.MsiVersion);
                }
                else
                {
                    this.ExternalState = new ClusterExternalState(
                        this.ClusterManifestList[this.CurrentListIndex - 2],
                        this.ClusterResource.Current.ExternalState.MsiVersion);
                }

                return null;
            }
        }

        internal void OnMultiphaseUpgradeRolledbackOrFailedCompleted()
        {
            if (this.IsAdminInitiatedUpgrade())
            {
                this.ClusterResource.TargetWrpConfigUpgradeFailed = true;
            }
            else
            {
                this.ClusterResource.TargetCsmConfigUpgradeFailed = true;
            }

            this.OnMultiphaseUpgradeRolledbackOrFailed();
        }

        protected abstract ClusterManifestType[] OnStartProcessingMultiphaseUpgrade();

        protected abstract bool OnMultiphaseUpgradeStarted();

        protected abstract void OnMultiphaseUpgradeCompleted();

        protected abstract void OnMultiphaseUpgradeRolledbackOrFailed();
    }
}