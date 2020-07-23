// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon
{
    using System.Fabric.Common;
    using System.Fabric.Management.ServiceModel;
    using Newtonsoft.Json;

    [JsonObject]
    internal abstract class SimpleClusterUpgradeState : ClusterUpgradeStateBase
    {
        private static readonly TraceType TraceType = new TraceType("SimpleClusterUpgradeState");

        protected SimpleClusterUpgradeState()
        {
        }

        protected SimpleClusterUpgradeState(
            IUserConfig targetCsmConfig,
            IAdminConfig targetWrpConfig,
            ClusterNodeConfig targetNodeConfig,
            ICluster clusterResource,
            ITraceLogger traceLogger)
        : base(targetCsmConfig, targetWrpConfig, targetNodeConfig, clusterResource, traceLogger)
        {
        }

        public override bool StartProcessing()
        {
            this.TraceLogger.WriteInfo(
                TraceType,
                "Start@StartProcessing: TargetCsmVersion: {0}, TargetWrpVersion: {1}, CurrentCsmVersion: {2}, CurrentWrpVersion: {3}",
                this.TargetCsmConfig.Version,
                this.TargetWrpConfig.Version,
                this.ClusterResource.Current.CSMConfig.Version,
                this.ClusterResource.Current.WRPConfig.Version);

            var manifestBuilder = this.ClusterManifestBuilderActivator.CreateClusterManifestBuilder(
                                this.ClusterResource.Topology,
                                this.ClusterResource.SeedNodeSelector,
                                this.TargetCsmConfig,
                                this.TargetWrpConfig,
                                this.TargetNodeConfig,
                                this /*versionGenerator*/,
                                this.ClusterResource.FabricSettingsActivator,
                                this.ClusterResource.ClusterManifestGeneratorSettings,
                                this.TraceLogger);

            var clusterManifest = manifestBuilder.UpdateClusterManifest(
                                      this.ClusterResource.Current.ExternalState.ClusterManifest,
                                      this.ClusterResource.Current.WRPConfig.GetFabricSettingsMetadata());

            this.ExternalState = new ClusterExternalState(clusterManifest, this.TargetWrpConfig.Version.MsiVersion);
            this.PreviousExternalState = this.ClusterResource.Current.ExternalState;

            this.TraceLogger.WriteInfo(
                TraceType,
                "Finish@StartProcessing: TargetCsmVersion: {0}, TargetWrpVersion: {1}, CurrentCsmVersion: {2}, CurrentWrpVersion: {3}",
                this.TargetCsmConfig.Version,
                this.TargetWrpConfig.Version,
                this.ClusterResource.Current.CSMConfig.Version,
                this.ClusterResource.Current.WRPConfig.Version);

            return true;
        }

        public override ClusterState ClusterUpgradeStarted()
        {
            this.TraceLogger.WriteInfo(
                TraceType,
                "ClusterUpgradeStarted called for {0} initiated upgrade. TargetCsmVersion: {1}, TargetWrpVersion: {2}",
                this.IsAdminInitiatedUpgrade() ? "Admin" : "User",
                this.TargetCsmConfig.Version,
                this.TargetWrpConfig.Version);

            // If there is no pending upgrade, check if the ClusterManifest result of the upgrade results in any change
            // If there is not change, then there is no need to start an upgrade
            if (this.ClusterResource.Pending == null)
            {
                bool isClusterManifestEqual = IsClusterManfiestEqual(this.ExternalState.ClusterManifest, this.ClusterResource.Current.ExternalState.ClusterManifest);
                bool isMsiEqual = EqualityUtility.Equals(this.TargetWrpConfig.Version.MsiVersion, this.ClusterResource.Current.ExternalState.MsiVersion);

                if (isClusterManifestEqual && isMsiEqual)
                {
                    this.TraceLogger.WriteInfo(
                        TraceType,
                        "The target cluster manifest and msi version is same as current. FabricUpgrade is not required.");

                    return new ClusterState(this.TargetCsmConfig, this.TargetWrpConfig, this.TargetNodeConfig, this.ClusterResource.Current.ExternalState);
                }
            }

            this.ClusterResource.ClusterState = this.IsAdminInitiatedUpgrade() ? ClusterProvisioningState.UpdatingInfrastructure : ClusterProvisioningState.UpdatingUserConfiguration;
            this.OnSimpleClusterUpgradeStarted();
            return null;
        }

        public override ClusterState ClusterUpgradeCompleted()
        {
            this.TraceLogger.WriteInfo(
                TraceType,
                "ClusterUpgradeCompleted called for {0} initiated upgrade. TargetCsmVersion: {1}, TargetWrpVersion: {2}",
                this.IsAdminInitiatedUpgrade() ? "Admin" : "User",
                this.TargetCsmConfig.Version,
                this.TargetWrpConfig.Version);

            this.ClusterResource.ClusterState = ClusterProvisioningState.Ready;

            this.ResetTargetConfigs();

            this.OnSimpleClusterUpgradeCompleted();

            return new ClusterState(this.TargetCsmConfig, this.TargetWrpConfig, this.TargetNodeConfig, this.ExternalState);
        }

        public override ClusterState ClusterUpgradeRolledbackOrFailed()
        {
            this.TraceLogger.WriteWarning(
                TraceType,
                "ClusterUpgradeRolledbackOrFailed called for {0} initiated upgrade. TargetCsmVersion: {1}, TargetWrpVersion: {2}",
                this.IsAdminInitiatedUpgrade() ? "Admin" : "User",
                this.TargetCsmConfig.Version.Version,
                this.TargetWrpConfig.Version);

            this.ClusterResource.ClusterState = ClusterProvisioningState.Ready;

            if (this.IsAdminInitiatedUpgrade())
            {
                this.ClusterResource.TargetWrpConfigUpgradeFailed = true;
            }
            else
            {
                this.ClusterResource.TargetCsmConfigUpgradeFailed = true;

                if (this.ClusterResource.TargetWrpConfig != null &&
                    this.ClusterResource.TargetWrpConfig.IsUserSet)
                {
                    ReleaseAssert.AssertIf(
                        !this.ClusterResource.TargetWrpConfig.Equals(this.TargetWrpConfig),
                        "For UserInitiatedUpgrade TargetWrpConfig should be used if it is user set");

                    this.ClusterResource.TargetWrpConfigUpgradeFailed = true;
                }
            }

            this.OnSimpleClusterUpgradeRolledbackOrFailed();

            return this.ClusterResource.Current;
        }

        protected virtual void OnSimpleClusterUpgradeStarted()
        {
        }

        protected virtual void OnSimpleClusterUpgradeCompleted()
        {
        }

        protected virtual void OnSimpleClusterUpgradeRolledbackOrFailed()
        {
        }

        private static bool IsClusterManfiestEqual(
            ClusterManifestType updatedClusterManifestType,
            ClusterManifestType existingClusterManifestType)
        {
            var updatedClusterManifestVersion = updatedClusterManifestType.Version;
            var existingClusterManifestVersion = existingClusterManifestType.Version;

            updatedClusterManifestType.Version = existingClusterManifestType.Version = "*";

            var isEqual = EqualityUtility.IsEqual(updatedClusterManifestType, existingClusterManifestType);

            updatedClusterManifestType.Version = updatedClusterManifestVersion;
            existingClusterManifestType.Version = existingClusterManifestVersion;

            return isEqual;
        }
    }
}