// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Management.ServiceModel;
    using System.Linq;

    using Newtonsoft.Json;

    [JsonObject]
    internal abstract class CertificateClusterUpgradeStateBase : MultiphaseClusterUpgradeState
    {
        private static readonly TraceType TraceType = new TraceType("CertificateClusterUpgradeStateBase");

        private HashSet<string> certificatesAdded;
        private HashSet<string> certificatesRemoved;

        [JsonConstructor]
        protected CertificateClusterUpgradeStateBase()
        {
        }

        protected CertificateClusterUpgradeStateBase(
            IUserConfig targetCsmConfig,
            IAdminConfig targetWrpConfig,
            ClusterNodeConfig targetNodeConfig,
            ICluster clusterResource,
            ITraceLogger traceLogger,
            HashSet<string> certsAdded,
            HashSet<string> certsRemoved)
        : base(targetCsmConfig, targetWrpConfig, targetNodeConfig, clusterResource, traceLogger)
        {
            this.certificatesAdded = certsAdded;
            this.certificatesRemoved = certsRemoved;
        }

        public override void ValidateSettingChanges()
        {
            base.ValidateSettingChanges();

            bool certificateAdded = this.certificatesAdded.Count > 0;
            bool certificateRemoved = this.certificatesRemoved.Count > 0;

            if (certificateAdded && this.ClusterResource.Current.CSMConfig.Security.CertificateInformation.ClusterCertificate == null
                && this.ClusterResource.Current.CSMConfig.Security.CertificateInformation.ClusterCertificateCommonNames == null)
            {
                throw new ClusterManagementException(ClusterManagementErrorCode.AuthenticationTypeCannotBeChangedByAddingCertificate);
            }

            if (certificateRemoved && this.TargetCsmConfig.Security.CertificateInformation.ClusterCertificate == null
                && this.TargetCsmConfig.Security.CertificateInformation.ClusterCertificateCommonNames == null)
            {
                throw new ClusterManagementException(ClusterManagementErrorCode.AuthenticationTypeCannotBeChangedByRemovingCertificate);
            }
        }

        protected abstract void OnCertificateUpgradeCompleted();

        protected abstract void OnCertificateUpgradeRolledbackOrFailed();

        protected abstract void OnCertificateUpgradeStarted();

        protected override void OnMultiphaseUpgradeCompleted()
        {
            this.OnCertificateUpgradeCompleted();
            this.TraceLogger.WriteInfo(TraceType, "CertificateClusterUpgrade has completed");
        }

        protected override void OnMultiphaseUpgradeRolledbackOrFailed()
        {
            this.OnCertificateUpgradeRolledbackOrFailed();
        }

        protected override bool OnMultiphaseUpgradeStarted()
        {
            this.OnCertificateUpgradeStarted();
            this.TraceLogger.WriteInfo(TraceType, "CertificateClusterUpgrade has started");

            return true;
        }

        protected override ClusterManifestType[] OnStartProcessingMultiphaseUpgrade()
        {
            var manifestBuilder = this.ClusterManifestBuilderActivator.CreateClusterManifestBuilder(
                                this.ClusterResource.Topology,
                                this.ClusterResource.SeedNodeSelector,
                                this.TargetCsmConfig,
                                this.TargetWrpConfig,
                                this.TargetNodeConfig,
                                //// this.ClusterResource.TargetDiagnosticsConfig,
                                this /*verionGenerator*/,
                                this.ClusterResource.FabricSettingsActivator,
                                this.ClusterResource.ClusterManifestGeneratorSettings,
                                this.TraceLogger);

            return manifestBuilder.UpdateCertificateInClusterManifest(
                       this.ClusterResource.Current.ExternalState.ClusterManifest,
                       this.ClusterResource.Current.CSMConfig.Security,
                       this.ClusterResource.Current.WRPConfig.GetFabricSettingsMetadata());
        }
    }
}